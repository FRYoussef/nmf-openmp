#ifndef _COMMON_
#define _COMMON_

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <CL/sycl.hpp>

using namespace cl::sycl;

// CUDA device selector
class CUDASelector : public device_selector {
    public:
        int operator()(const device &Device) const override {
            const std::string DriverVersion = Device.get_info<info::device::driver_version>();

            if (Device.is_gpu() && (DriverVersion.find("CUDA") != std::string::npos)) {
                std::cout << std::endl << "Running on  CUDA GPU" << std::endl << std::endl;
                return 1;
            }

            return 0;
        }
};

// Intel iGPU
class NEOGPUDeviceSelector : public device_selector {
    public:
        int operator()(const device &Device) const override {
            const std::string DeviceName = Device.get_info<info::device::name>();

            if (Device.is_gpu() && (DeviceName.find("HD Graphics NEO") != std::string::npos)) {
                std::cout << std::endl << "Running on HD Graphics NEO GPU" << std::endl << std::endl;
                return 1;
            }

            return 0;
        }
};

#define RANDOM
//#define DEBUG
const bool verbose = false;
const char PAD = 32;
//static int HW_SPECIFIC_ADVICE_RO = 0;

#ifdef REAL_S
#define C_REAL float
#else
#define C_REAL double
#endif

#ifdef BLAS_KERNEL
#define W_mult_H blas_W_mult_H
#define accum blas_accum
#define Wt_mult_WH blas_Wt_mult_WH
#define WH_mult_Ht blas_WH_mult_Ht
#else
#define W_mult_H bare_W_mult_H
#define accum bare_accum
#define Wt_mult_WH bare_Wt_mult_WH
#define WH_mult_Ht bare_WH_mult_Ht
#endif

/* Number of iterations before testing convergence (can be adjusted) */
const int NITER_TEST_CONV = 10;

/* Spacing of floating point numbers. */
const C_REAL eps = 2.2204e-16;

void adjust_WH(queue q, C_REAL *W, C_REAL *Ht, int N, int M, int K);
void V_div_WH(queue q, C_REAL *V, C_REAL *WH, int N, int M);
void mult_M_div_vect(queue q, C_REAL *Mat, C_REAL *Maux, C_REAL *acc, int M, int K);

#endif