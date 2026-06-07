#include "network/fetch/network_service.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <memory>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace
{

constexpr std::size_t kMaxResponseBytes = 2U * 1024U * 1024U;
constexpr std::string_view kAboutBlankDocument =
    "<!doctype html><html><head><title></title></head><body></body></html>";

struct ParsedHttpUrl final
{
  std::string host;
  std::string port;
  std::string target;
};

class ScopedSocket final
{
public:
  ScopedSocket() = default;
  explicit ScopedSocket(int fd)
      : fd_(fd)
  {}

  ScopedSocket(const ScopedSocket&) = delete;
  ScopedSocket& operator=(const ScopedSocket&) = delete;

  ScopedSocket(ScopedSocket&& other) noexcept
      : fd_(std::exchange(other.fd_, -1))
  {}

  ScopedSocket& operator=(ScopedSocket&& other) noexcept
  {
    if (this != &other)
    {
      Reset();
      fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
  }

  ~ScopedSocket()
  {
    Reset();
  }

  [[nodiscard]] bool valid() const
  {
    return fd_ >= 0;
  }

  [[nodiscard]] int get() const
  {
    return fd_;
  }

private:
  void Reset()
  {
    if (fd_ >= 0)
    {
      (void)::close(fd_);
      fd_ = -1;
    }
  }

  int fd_{-1};
};

struct SslContextDeleter final
{
  void operator()(SSL_CTX* context) const
  {
    SSL_CTX_free(context);
  }
};

struct SslDeleter final
{
  void operator()(SSL* ssl) const
  {
    SSL_free(ssl);
  }
};

using ScopedSslContext = std::unique_ptr<SSL_CTX, SslContextDeleter>;
using ScopedSsl = std::unique_ptr<SSL, SslDeleter>;

[[nodiscard]] std::string ToLowerAscii(std::string_view value)
{
  std::string lowered;
  lowered.reserve(value.size());
  for (const char character : value)
  {
    const unsigned char byte = static_cast<unsigned char>(character);
    if (byte >= static_cast<unsigned char>('A') && byte <= static_cast<unsigned char>('Z'))
    {
      lowered.push_back(static_cast<char>(byte - static_cast<unsigned char>('A') +
                                          static_cast<unsigned char>('a')));
      continue;
    }
    lowered.push_back(character);
  }
  return lowered;
}

[[nodiscard]] bool ContainsUnsafeHttpByte(std::string_view value)
{
  return std::ranges::any_of(value,
                             [](char character)
                             {
                               const unsigned char byte = static_cast<unsigned char>(character);
                               return byte <= static_cast<unsigned char>(' ') || byte == 127U;
                             });
}

[[nodiscard]] std::string SchemeForUrl(std::string_view url)
{
  const std::size_t colon = url.find(':');
  if (colon == std::string_view::npos)
  {
    return {};
  }
  return ToLowerAscii(url.substr(0, colon));
}

[[nodiscard]] std::optional<ParsedHttpUrl>
ParseNetworkUrl(std::string_view url, std::string_view scheme, std::string_view default_port)
{
  const std::string prefix = std::string(scheme) + "://";
  if (ToLowerAscii(url.substr(0, prefix.size())) != prefix)
  {
    return std::nullopt;
  }

  std::string_view remainder = url.substr(prefix.size());
  const std::size_t path_start = remainder.find_first_of("/?#");
  std::string_view authority = remainder.substr(0, path_start);
  std::string target = "/";
  if (path_start != std::string_view::npos)
  {
    target = remainder[path_start] == '?' ? "/" + std::string(remainder.substr(path_start))
                                          : std::string(remainder.substr(path_start));
  }

  const std::size_t fragment_start = target.find('#');
  if (fragment_start != std::string::npos)
  {
    target.erase(fragment_start);
  }
  if (target.empty())
  {
    target = "/";
  }

  if (authority.empty() || authority.find('@') != std::string_view::npos ||
      authority.front() == '[' || ContainsUnsafeHttpByte(authority) ||
      ContainsUnsafeHttpByte(target))
  {
    return std::nullopt;
  }

  std::string host(authority);
  std::string port(default_port);
  const std::size_t port_separator = host.rfind(':');
  if (port_separator != std::string::npos)
  {
    port = host.substr(port_separator + 1);
    host.erase(port_separator);
  }

  if (host.empty() || port.empty() || ContainsUnsafeHttpByte(host) || ContainsUnsafeHttpByte(port))
  {
    return std::nullopt;
  }

  return ParsedHttpUrl{
      .host = std::move(host),
      .port = std::move(port),
      .target = std::move(target),
  };
}

[[nodiscard]] std::optional<ParsedHttpUrl> ParseHttpUrl(std::string_view url)
{
  return ParseNetworkUrl(url, "http", "80");
}

[[nodiscard]] std::optional<ParsedHttpUrl> ParseHttpsUrl(std::string_view url)
{
  return ParseNetworkUrl(url, "https", "443");
}

[[nodiscard]] std::string ErrnoMessage(std::string_view prefix)
{
  return std::string(prefix) + ": " + std::strerror(errno);
}

[[nodiscard]] std::string OpenSslErrorMessage(std::string_view prefix)
{
  const unsigned long error_code = ERR_get_error();
  if (error_code == 0)
  {
    return std::string(prefix);
  }

  std::array<char, 256> buffer{};
  ERR_error_string_n(error_code, buffer.data(), buffer.size());
  return std::string(prefix) + ": " + buffer.data();
}

[[nodiscard]] speed::network::FetchResult SendAll(int socket_fd, std::string_view bytes)
{
  while (!bytes.empty())
  {
    const ssize_t sent = ::send(socket_fd, bytes.data(), bytes.size(), 0);
    if (sent < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      return speed::network::FetchResult::Failure(
          ErrnoMessage("http fetch failed while sending request"));
    }
    if (sent == 0)
    {
      return speed::network::FetchResult::Failure(
          "http fetch connection closed while sending request");
    }

    bytes.remove_prefix(static_cast<std::size_t>(sent));
  }

  return speed::network::FetchResult::Success("sent");
}

[[nodiscard]] speed::network::FetchResult SendAll(SSL& ssl, std::string_view bytes)
{
  while (!bytes.empty())
  {
    const std::size_t write_size =
        std::min(bytes.size(), static_cast<std::size_t>(std::numeric_limits<int>::max()));
    const int written = SSL_write(&ssl, bytes.data(), static_cast<int>(write_size));
    if (written <= 0)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed while sending request"));
    }

    bytes.remove_prefix(static_cast<std::size_t>(written));
  }

  return speed::network::FetchResult::Success("sent");
}

[[nodiscard]] speed::network::FetchResult ReadAll(int socket_fd)
{
  std::string response;
  std::array<char, 4096> buffer{};

  for (;;)
  {
    const ssize_t read_count = ::recv(socket_fd, buffer.data(), buffer.size(), 0);
    if (read_count < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      return speed::network::FetchResult::Failure(
          ErrnoMessage("http fetch failed while reading response"));
    }
    if (read_count == 0)
    {
      break;
    }

    const std::size_t chunk_size = static_cast<std::size_t>(read_count);
    if (response.size() + chunk_size > kMaxResponseBytes)
    {
      return speed::network::FetchResult::Failure(
          "http fetch response exceeded the MVP size limit");
    }
    response.append(buffer.data(), chunk_size);
  }

  return speed::network::FetchResult::Success(std::move(response));
}

[[nodiscard]] speed::network::FetchResult ReadAll(SSL& ssl)
{
  std::string response;
  std::array<char, 4096> buffer{};

  for (;;)
  {
    const int read_count = SSL_read(&ssl, buffer.data(), static_cast<int>(buffer.size()));
    if (read_count <= 0)
    {
      const int ssl_error = SSL_get_error(&ssl, read_count);
      if (ssl_error == SSL_ERROR_ZERO_RETURN)
      {
        break;
      }

      if (ssl_error == SSL_ERROR_SYSCALL && ERR_peek_error() == 0)
      {
        break;
      }

      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed while reading response"));
    }

    const std::size_t chunk_size = static_cast<std::size_t>(read_count);
    if (response.size() + chunk_size > kMaxResponseBytes)
    {
      return speed::network::FetchResult::Failure(
          "https fetch response exceeded the MVP size limit");
    }
    response.append(buffer.data(), chunk_size);
  }

  return speed::network::FetchResult::Success(std::move(response));
}

[[nodiscard]] std::string TrimTrailingCarriageReturn(std::string value)
{
  if (!value.empty() && value.back() == '\r')
  {
    value.pop_back();
  }
  return value;
}

[[nodiscard]] bool HasChunkedTransferEncoding(std::string_view headers)
{
  const std::string lowered = ToLowerAscii(headers);
  const std::size_t transfer_encoding = lowered.find("transfer-encoding:");
  if (transfer_encoding == std::string::npos)
  {
    return false;
  }

  const std::size_t line_end = lowered.find('\n', transfer_encoding);
  const std::string_view line =
      line_end == std::string::npos
          ? std::string_view(lowered).substr(transfer_encoding)
          : std::string_view(lowered).substr(transfer_encoding, line_end - transfer_encoding);
  return line.find("chunked") != std::string_view::npos;
}

[[nodiscard]] speed::network::FetchResult ParseHttpResponse(std::string response)
{
  std::size_t header_end = response.find("\r\n\r\n");
  std::size_t separator_size = 4;
  if (header_end == std::string::npos)
  {
    header_end = response.find("\n\n");
    separator_size = 2;
  }
  if (header_end == std::string::npos)
  {
    return speed::network::FetchResult::Failure("malformed HTTP response");
  }

  const std::string headers = response.substr(0, header_end);
  std::string body = response.substr(header_end + separator_size);

  const std::size_t status_line_end = headers.find('\n');
  const std::string status_line = TrimTrailingCarriageReturn(
      status_line_end == std::string::npos ? headers : headers.substr(0, status_line_end));

  std::istringstream status_stream(status_line);
  std::string http_version;
  int status_code = 0;
  status_stream >> http_version >> status_code;
  if (http_version.rfind("HTTP/", 0) != 0 || status_code == 0)
  {
    return speed::network::FetchResult::Failure("malformed HTTP status line");
  }

  if (status_code < 200 || status_code >= 300)
  {
    return speed::network::FetchResult::Failure("http fetch returned status " +
                                                std::to_string(status_code));
  }

  if (HasChunkedTransferEncoding(headers))
  {
    return speed::network::FetchResult::Failure(
        "chunked HTTP responses are not supported by the MVP fetch adapter");
  }

  if (body.empty())
  {
    return speed::network::FetchResult::Failure("http fetch returned an empty body");
  }

  return speed::network::FetchResult::Success(std::move(body));
}

class DefaultFetchAdapter final : public speed::network::FetchAdapter
{
public:
  [[nodiscard]] speed::network::FetchResult
  Fetch(const speed::network::FetchRequest& request) const override
  {
    const std::string scheme = SchemeForUrl(request.url);
    if (scheme == "about")
    {
      if (ToLowerAscii(request.url) == "about:blank")
      {
        return speed::network::FetchResult::Success(std::string(kAboutBlankDocument));
      }
      return speed::network::FetchResult::Failure(
          "only about:blank is supported by the MVP fetch adapter");
    }

    if (scheme != "http" && scheme != "https")
    {
      return speed::network::FetchResult::Failure(
          "unsupported URL scheme for the MVP fetch adapter");
    }

    const std::optional<ParsedHttpUrl> parsed_url =
        scheme == "https" ? ParseHttpsUrl(request.url) : ParseHttpUrl(request.url);
    if (!parsed_url)
    {
      return speed::network::FetchResult::Failure("invalid " + scheme + " URL");
    }

    if (scheme == "https")
    {
      return FetchHttps(*parsed_url);
    }

    return FetchHttp(*parsed_url);
  }

private:
  [[nodiscard]] speed::network::FetchResult ConnectTcp(const ParsedHttpUrl& url,
                                                       ScopedSocket& socket) const
  {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* raw_addresses = nullptr;
    const int result = ::getaddrinfo(url.host.c_str(), url.port.c_str(), &hints, &raw_addresses);
    if (result != 0)
    {
      return speed::network::FetchResult::Failure(std::string("fetch failed to resolve host: ") +
                                                  ::gai_strerror(result));
    }

    std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> addresses(raw_addresses, &::freeaddrinfo);
    std::string last_connect_error = "no resolved address";
    for (addrinfo* address = addresses.get(); address != nullptr; address = address->ai_next)
    {
      ScopedSocket candidate(
          ::socket(address->ai_family, address->ai_socktype, address->ai_protocol));
      if (!candidate.valid())
      {
        last_connect_error = ErrnoMessage("socket creation failed");
        continue;
      }

      if (::connect(candidate.get(), address->ai_addr, address->ai_addrlen) == 0)
      {
        socket = std::move(candidate);
        break;
      }

      last_connect_error = ErrnoMessage("connect failed");
    }

    if (!socket.valid())
    {
      return speed::network::FetchResult::Failure("fetch failed to connect: " + last_connect_error);
    }

    return speed::network::FetchResult::Success("connected");
  }

  [[nodiscard]] static std::string BuildHttpRequest(const ParsedHttpUrl& url)
  {
    const std::string host_header = url.port == "80" ? url.host : url.host + ":" + url.port;
    return "GET " + url.target + " HTTP/1.0\r\nHost: " + host_header +
           "\r\nUser-Agent: Speed/0.1\r\nAccept: text/html,*/*;q=0.1"
           "\r\nConnection: close\r\n\r\n";
  }

  [[nodiscard]] speed::network::FetchResult FetchHttp(const ParsedHttpUrl& url) const
  {
    ScopedSocket socket;
    speed::network::FetchResult connect_result = ConnectTcp(url, socket);
    if (!connect_result.ok())
    {
      return connect_result;
    }

    const std::string request = BuildHttpRequest(url);
    const speed::network::FetchResult send_result = SendAll(socket.get(), request);
    if (!send_result.ok())
    {
      return send_result;
    }

    speed::network::FetchResult read_result = ReadAll(socket.get());
    if (!read_result.ok())
    {
      return read_result;
    }

    return ParseHttpResponse(std::move(read_result.body));
  }

  [[nodiscard]] speed::network::FetchResult FetchHttps(const ParsedHttpUrl& url) const
  {
    ScopedSocket socket;
    speed::network::FetchResult connect_result = ConnectTcp(url, socket);
    if (!connect_result.ok())
    {
      return connect_result;
    }

    ScopedSslContext context(SSL_CTX_new(TLS_client_method()));
    if (!context)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed to create TLS context"));
    }

    SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER, nullptr);
    if (SSL_CTX_set_default_verify_paths(context.get()) != 1)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed to load default certificate paths"));
    }

    ScopedSsl ssl(SSL_new(context.get()));
    if (!ssl)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed to create TLS session"));
    }

    if (SSL_set_fd(ssl.get(), socket.get()) != 1)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed to attach socket"));
    }

    if (SSL_ctrl(ssl.get(),
                 SSL_CTRL_SET_TLSEXT_HOSTNAME,
                 TLSEXT_NAMETYPE_host_name,
                 static_cast<void*>(const_cast<char*>(url.host.c_str()))) != 1)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed to set SNI host"));
    }

    if (SSL_set1_host(ssl.get(), url.host.c_str()) != 1)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch failed to set verification host"));
    }

    if (SSL_connect(ssl.get()) != 1)
    {
      return speed::network::FetchResult::Failure(
          OpenSslErrorMessage("https fetch TLS handshake failed"));
    }

    const long verify_result = SSL_get_verify_result(ssl.get());
    if (verify_result != X509_V_OK)
    {
      return speed::network::FetchResult::Failure(
          std::string("https fetch certificate verification failed: ") +
          X509_verify_cert_error_string(verify_result));
    }

    const std::string request = BuildHttpRequest(url);
    const speed::network::FetchResult send_result = SendAll(*ssl, request);
    if (!send_result.ok())
    {
      return send_result;
    }

    speed::network::FetchResult read_result = ReadAll(*ssl);
    if (!read_result.ok())
    {
      return read_result;
    }

    return ParseHttpResponse(std::move(read_result.body));
  }
};

} // namespace

namespace speed::network
{

std::unique_ptr<FetchAdapter> CreateDefaultFetchAdapter()
{
  return std::make_unique<DefaultFetchAdapter>();
}

NetworkService::NetworkService(aegis::RequestClassifier classifier)
    : NetworkService(std::move(classifier), CreateDefaultFetchAdapter())
{}

NetworkService::NetworkService(aegis::RequestClassifier classifier,
                               std::unique_ptr<FetchAdapter> fetch_adapter)
    : classifier_(std::move(classifier)),
      fetch_adapter_(std::move(fetch_adapter))
{
  if (!fetch_adapter_)
  {
    fetch_adapter_ = CreateDefaultFetchAdapter();
  }
}

NetworkService::NetworkService(NetworkService&&) noexcept = default;
NetworkService& NetworkService::operator=(NetworkService&&) noexcept = default;
NetworkService::~NetworkService() = default;

NetworkResult NetworkService::PrepareRequest(const NetworkRequest& request) const
{
  const aegis::Classification classification = classifier_.ClassifyUrl(request.url);
  return {
      .would_dispatch = classification.kind == aegis::DecisionKind::kAllow,
      .aegis_decision = classification,
  };
}

ipc::navigation::NavigateResponse
NetworkService::FetchNavigation(const ipc::navigation::NavigateRequest& request) const
{
  if (!ipc::navigation::IsValidNavigateRequest(request))
  {
    return {
        .request_id = request.request_id,
        .status = ipc::navigation::NavigateStatus::kFailed,
        .aegis_reason = {},
        .error_message = "invalid navigation request",
        .document_body = {},
    };
  }

  const NetworkResult prepared = PrepareRequest({
      .request_id = request.request_id,
      .url = request.url,
  });

  if (!prepared.would_dispatch)
  {
    return {
        .request_id = request.request_id,
        .status = ipc::navigation::NavigateStatus::kBlocked,
        .aegis_reason = prepared.aegis_decision.reason,
        .error_message = {},
        .document_body = {},
    };
  }

  const FetchResult fetched = fetch_adapter_->Fetch({
      .request_id = request.request_id,
      .url = request.url,
  });

  if (!fetched.ok())
  {
    return {
        .request_id = request.request_id,
        .status = ipc::navigation::NavigateStatus::kFailed,
        .aegis_reason = prepared.aegis_decision.reason,
        .error_message = fetched.error_message.empty() ? "fetch failed" : fetched.error_message,
        .document_body = {},
    };
  }

  if (fetched.body.empty())
  {
    return {
        .request_id = request.request_id,
        .status = ipc::navigation::NavigateStatus::kFailed,
        .aegis_reason = prepared.aegis_decision.reason,
        .error_message = "fetch adapter returned an empty body",
        .document_body = {},
    };
  }

  return {
      .request_id = request.request_id,
      .status = ipc::navigation::NavigateStatus::kAllowed,
      .aegis_reason = prepared.aegis_decision.reason,
      .error_message = {},
      .document_body = fetched.body,
  };
}

} // namespace speed::network
