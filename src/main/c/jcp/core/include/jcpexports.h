#ifndef Jcp_EXPORTS_H
#define Jcp_EXPORTS_H

#ifndef __has_attribute
  #define __has_attribute(x) 0  // Compatibility with non-clang compilers.
#endif
#if (defined(__GNUC__) && (__GNUC__ >= 4)) ||\
    (defined(__clang__) && __has_attribute(visibility))
    #define Jcp_IMPORTED_SYMBOL __attribute__ ((visibility ("default")))
    #define Jcp_EXPORTED_SYMBOL __attribute__ ((visibility ("default")))
    #define Jcp_LOCAL_SYMBOL  __attribute__ ((visibility ("hidden")))
#else
    #define Jcp_IMPORTED_SYMBOL
    #define Jcp_EXPORTED_SYMBOL
    #define Jcp_LOCAL_SYMBOL
#endif

#endif /* Jcp_EXPORTS_H */
