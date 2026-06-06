include(CheckCXXCompilerFlag)

function(speed_add_supported_cxx_option target_name option)
  string(MAKE_C_IDENTIFIER "SPEED_HAS_CXX_OPTION_${option}" option_var)
  check_cxx_compiler_flag("${option}" "${option_var}")

  if(${option_var})
    target_compile_options(${target_name} PRIVATE "${option}")
  endif()
endfunction()

function(speed_add_supported_cxx_options target_name)
  foreach(option IN LISTS ARGN)
    speed_add_supported_cxx_option(${target_name} "${option}")
  endforeach()
endfunction()

function(speed_configure_target target_name)
  target_compile_features(${target_name} PUBLIC cxx_std_23)

  if(SPEED_ENABLE_CLANG_TIDY AND SPEED_CLANG_TIDY_COMMAND)
    set_target_properties(${target_name} PROPERTIES CXX_CLANG_TIDY "${SPEED_CLANG_TIDY_COMMAND}")
  endif()

  if(SPEED_WARNINGS_AS_ERRORS)
    set_target_properties(${target_name} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
  endif()

  if(MSVC)
    target_compile_options(
      ${target_name}
      PRIVATE
        /W4
        /permissive-
        /EHsc
        /Zc:__cplusplus
        /Zc:inline
        /Zc:preprocessor
        /Zc:throwingNew
    )

    if(SPEED_ENABLE_STRICT_WARNINGS)
      target_compile_options(
        ${target_name}
        PRIVATE
          /w14242
          /w14254
          /w14263
          /w14265
          /w14287
          /w14289
          /w14296
          /w14311
          /w14545
          /w14546
          /w14547
          /w14549
          /w14555
          /w14619
          /w14640
          /w14826
          /w14905
          /w14906
          /w14928
      )
    endif()
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)

    if(SPEED_ENABLE_STRICT_WARNINGS)
      speed_add_supported_cxx_options(
        ${target_name}
        -Wcast-align
        -Wcast-qual
        -Wconversion
        -Wdate-time
        -Wdouble-promotion
        -Wextra-semi
        -Wfloat-equal
        -Wformat=2
        -Wimplicit-fallthrough
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wnon-virtual-dtor
        -Wnull-dereference
        -Wold-style-cast
        -Woverloaded-virtual
        -Wredundant-decls
        -Wshadow
        -Wsign-conversion
        -Wswitch-enum
        -Wundef
        -Wunreachable-code
        -Wunused
        -Wzero-as-null-pointer-constant
        -Werror=return-type
      )

      if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        speed_add_supported_cxx_options(
          ${target_name}
          -Wduplicated-branches
          -Wduplicated-cond
          -Wlogical-op
          -Wsuggest-override
          -Wuseless-cast
        )
      elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        speed_add_supported_cxx_options(
          ${target_name}
          -Wcomma
          -Wdeprecated
          -Wdocumentation
          -Wextra-semi-stmt
          -Wheader-hygiene
          -Wnewline-eof
          -Wrange-loop-analysis
          -Wreserved-identifier
          -Wshadow-all
          -Wshorten-64-to-32
          -Wthread-safety
          -Wundefined-reinterpret-cast
          -Wunreachable-code-aggressive
          -Wweak-vtables
        )
      endif()
    endif()
  endif()
endfunction()
