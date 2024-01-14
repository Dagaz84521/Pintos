#ifndef __THREAD_FIXED_POINT_H
#define __THREAD_FIXED_POINT_H

/* Basic definitions of fixed point. */
typedef int fixed_t;
/* 精确位数是小数点后16位 */
#define FP_SHIFT_AMOUNT 16
/* 把浮点数A的小数部分变为整数 */
#define FP_CONST(A) ((fixed_t)(A << FP_SHIFT_AMOUNT))
/* 两个小数部分加法 */
#define FP_ADD(A,B) (A + B)
/* 混合加法浮点数A和整数B */
#define FP_ADD_MIX(A,B) (A + (B << FP_SHIFT_AMOUNT))
/* 减法 */
#define FP_SUB(A,B) (A - B)
/* 混合减法 */
#define FP_SUB_MIX(A,B) (A - (B << FP_SHIFT_AMOUNT))
/* 乘法A是浮点数B是整数 */
#define FP_MULT_MIX(A,B) (A * B)
/* 除法A是浮点数B是整数 */
#define FP_DIV_MIX(A,B) (A / B)
/* 两个浮点数的乘法 */
#define FP_MULT(A,B) ((fixed_t)(((int64_t) A) * B >> FP_SHIFT_AMOUNT))
/* 两个浮点数的除法 */
#define FP_DIV(A,B) ((fixed_t)((((int64_t) A) << FP_SHIFT_AMOUNT) / B))
/* 得到浮点数A的整数部分 */
#define FP_INT_PART(A) (A >> FP_SHIFT_AMOUNT)
/* 得到浮点数A的小数部分 */
#define FP_ROUND(A) (A >= 0 ? ((A + (1 << (FP_SHIFT_AMOUNT - 1))) >> FP_SHIFT_AMOUNT) \
        : ((A - (1 << (FP_SHIFT_AMOUNT - 1))) >> FP_SHIFT_AMOUNT))

#endif /* thread/fixed_point.h */