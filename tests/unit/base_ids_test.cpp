#include "base/ids/id_types.h"

#include <cassert>

int main()
{
  const speed::base::TabId tab_id = speed::base::TabId::FromRaw(7);
  const speed::base::RequestId request_id = speed::base::RequestId::FromRaw(7);

  assert(tab_id);
  assert(request_id);
  assert(tab_id.value() == 7);
  assert(request_id.value() == 7);

  return 0;
}
