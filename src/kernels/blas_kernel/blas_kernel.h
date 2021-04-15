#ifndef _BLAS_KERNEL_
#define _BLAS_KERNEL_

#include "../common.h"
#include <CL/sycl.hpp>
#include "oneapi/mkl.hpp"
//#include "mkl_sycl.hpp" /* To be included in version <beta0.9 */

constexpr oneapi::mkl::transpose transA = oneapi::mkl::transpose::trans;
constexpr oneapi::mkl::transpose transB = oneapi::mkl::transpose::nontrans;

void blas_W_mult_H(queue q, C_REAL *WH, C_REAL *W, C_REAL *Htras, int N, int M, int K);
void blas_accum(queue q, C_REAL *acc, C_REAL *X, int N, int M);
void blas_Wt_mult_WH(queue q, C_REAL *Haux, C_REAL *W, C_REAL *WH, int N, int M, int K);
void blas_WH_mult_Ht(queue q, C_REAL *Waux, C_REAL *WH, C_REAL *Htras, int N, int M, int K);

#endif
