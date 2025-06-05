#pragma once

/** Boost header inclusion should use this header and the
 * wrapper functions.
 */

// 'This' fun boost thing...global namespace for _1.
// It seems ptree still wants it.  We define it here
// to force it before any boost header is included.
// #define BOOST_BIND_NO_PLACEHOLDERS
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#if defined(__clang__)
# if __clang_major__ >= 17
#  define BOOST_WRAP_PUSH \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wenum-constexpr-conversion\"")
# else
#  define BOOST_WRAP_PUSH _Pragma("clang diagnostic push")
# endif
# define BOOST_WRAP_POP   _Pragma("clang diagnostic pop")

#elif defined(__GNUC__) && !defined(__clang__)
# define BOOST_WRAP_PUSH _Pragma("GCC diagnostic push")
# define BOOST_WRAP_POP  _Pragma("GCC diagnostic pop")

#else // if defined(__clang__)
# define BOOST_WRAP_PUSH
# define BOOST_WRAP_POP
#endif // if defined(__clang__)
