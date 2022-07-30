#pragma once

#if defined ARCH_INTERNAL_ASSERTIONS || defined ARCH_ASSERTIONS
#include <cassert>
#endif

#if defined ARCH_INTERNAL_ASSERTIONS
#define arch_assert_internal(x) do {if(!( x )) { assert( x ); } }while(0)
#else
#define arch_assert_internal(x) void(x)
#endif

#if defined ARCH_INTERNAL_ASSERTIONS || defined ARCH_ASSERTIONS
#define arch_assert_external(x) do {if(!( x )) { assert( x ); } }while(0)
#else
#define arch_assert_external(x) void(x)
#endif

#if defined ARCH_SAFE_PTR_INIT
#define arch_ptr_init = nullptr
#else
#define arch_ptr_init
#endif

#define arch_fwd(val) std::forward<decltype(val)>(val)

#if defined __clang__ | defined __GNUC__
#define arch_restrict __restrict__
#elif _MSC_VER & !__INTEL_COMPILER
#define arch_restrict __restrict
#endif