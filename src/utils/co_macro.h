#ifndef UTILS__CO_MACRO_H
#define UTILS__CO_MACRO_H

#define co_stringify_(x) #x
#define co_stringify(x) co_stringify_(x)

#define co_sizeof_array(a) sizeof(a)

#define container_of(ptr, type, member) ((type *)((void *)ptr - (void *)&((type *)0)->member))

#define __co_list_c(...) __co_cat_2(__co_list_c_, __co_narg(__VA_ARGS__))(__VA_ARGS__)
#define __co_list_c_0()
#define __co_list_c_1(_1) _1
#define __co_list_c_2(_1, _2) _1, _2
#define __co_list_c_3(_1, _2, _3) _1, _2, _3
#define __co_list_c_4(_1, _2, _3, _4) _1, _2, _3, _4
#define __co_list_c_5(_1, _2, _3, _4, _5) _1, _2, _3, _4, _5
#define __co_list_c_6(_1, _2, _3, _4, _5, _6) _1, _2, _3, _4, _5, _6
#define __co_list_c_7(_1, _2, _3, _4, _5, _6, _7) _1, _2, _3, _4, _5, _6, _7
#define __co_list_c_8(_1, _2, _3, _4, _5, _6, _7, _8) _1, _2, _3, _4, _5, _6, _7, _8

#define __co_list(delim, ...) __co_cat_2(__co_list_, __co_narg(__VA_ARGS__))(delim, ##__VA_ARGS__)
#define __co_list_0(d)
#define __co_list_1(d, _1) _1
#define __co_list_2(d, _1, _2) _1 d _2
#define __co_list_3(d, _1, _2, _3) _1 d _2 d _3
#define __co_list_4(d, _1, _2, _3, _4) _1 d _2 d _3 d _4
#define __co_list_5(d, _1, _2, _3, _4, _5) _1 d _2 d _3 d _4 d _5
#define __co_list_6(d, _1, _2, _3, _4, _5, _6) _1 d _2 d _3 d _4 d _5 d _6
#define __co_list_7(d, _1, _2, _3, _4, _5, _6, _7) _1 d _2 d _3 d _4 d _5 d _6 d _7
#define __co_list_8(d, _1, _2, _3, _4, _5, _6, _7, _8) _1 d _2 d _3 d _4 d _5 d _6 d _7 d _8

#define __co_pairs(delim, ...) __co_cat_2(__co_pairs_, __co_narg(__VA_ARGS__))(delim, ##__VA_ARGS__)
#define __co_pairs_0(d)
#define __co_pairs_2(d, _1, _2) _1 _2
#define __co_pairs_4(d, _1, _2, _3, _4) _1 _2 d _3 _4
#define __co_pairs_6(d, _1, _2, _3, _4, _5, _6) _1 _2 d _3 _4 d _5 _6
#define __co_pairs_8(d, _1, _2, _3, _4, _5, _6, _7, _8) _1 _2 d _3 _4 d _5 _6 d _7 _8

#define __co_narg(...) __co_narg_(dum, ##__VA_ARGS__, __co_n_list())
#define __co_narg_(...) __co_arg_n(__VA_ARGS__)

#define __co_arg_n(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21,     \
				   _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, \
				   _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, \
				   _62, _63, N, ...)                                                                                   \
	N

#define __co_n_list()                                                                                                  \
	62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35,    \
		34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7,   \
		6, 5, 4, 3, 2, 1, 0, -1

#define __co_cat_2(x, y) __co_cat_2_(x, y)
#define __co_cat_2_(x, y) x##y

#define __co_nop(...)                                                                                                  \
	do {                                                                                                               \
	} while (0)

#endif /*UTILS__CO_MACRO_H*/