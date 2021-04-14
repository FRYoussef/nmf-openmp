#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#ifdef BLAS_KERNEL
#include "./kernels/blas_kernel/blas_kernel.h"
#else
#include "./kernels/bare_kernel/bare_kernel.h" //default kernels
#endif

double gettime() {
	double final_time;
	struct timeval tv1;
	
	gettimeofday(&tv1, (struct timezone*)0);
	final_time = (tv1.tv_usec + (tv1.tv_sec)*1000000ULL);

	return final_time;
}


C_REAL *get_memory1D(int nx){ 
	C_REAL *buffer = new C_REAL[nx];	

	for(int i = 0; i < nx; i++ )
		buffer[i] = (C_REAL)(i*10);

	return buffer;
}


void delete_memory1D(C_REAL *buffer) { 
	delete[] buffer;
}


unsigned char *get_memory1D_uchar(int nx) { 
	unsigned char *buffer = new unsigned char[nx];

    for(int i = 0; i < nx; i++)
		buffer[i] = (int)(0);

	return buffer ;
}


void delete_memory1D_uchar(unsigned char *buffer) { 
	delete[] buffer;
}


C_REAL *get_memory2D_in_1D(int nx, int ny) {
	C_REAL *buffer = new C_REAL[nx*ny];

	for(int i = 0; i < nx; i++)
		for(int j = 0; j < ny; j++)
			buffer[i*ny + j] = (C_REAL)(i*100 + j);

	return buffer;
}


void matrix_copy1D_uchar(unsigned char *in, unsigned char *out, int nx) {
	for (int i = 0; i < nx; i++)
		out[i] = in[i];
}


void matrix_copy2D(buffer<C_REAL, 1> &b_in, C_REAL *out, int nx, int ny) {
	auto in = b_in.get_access<sycl_read>();
	for (int i = 0; i < nx; i++)
		for(int j = 0; j < ny; j++)
			out[i*ny + j] = in[i*ny + j];
}


void initWH(buffer<C_REAL, 1> &b_W, buffer<C_REAL, 1> &b_Htras, int N, int M, int K) {	
    auto W = b_W.get_access<sycl_write>();
    auto Htras = b_Htras.get_access<sycl_write>();	
	srand(0);

	for (int i = 0; i < N; i++)
		for (int j = 0; j < K; j++)
			W[i*K + j] = ((C_REAL)(rand()))/RAND_MAX;

	for (int i = 0; i < M; i++)
        for (int j = 0; j < K; j++)
			Htras[i*K + j] = ((C_REAL)(rand()))/RAND_MAX;

#ifdef DEBUG
	/* Added to debug */
	FILE *fIn;
	int size_W = N*K;

	fIn = fopen("w_bin.bin", "r");
	fread(W, sizeof(C_REAL), size_W, fIn);
	fclose(fIn);

	int size_H = M*K;
	fIn = fopen("h_bin.bin", "r");
	fread(Htras, sizeof(C_REAL), size_H, fIn);
	fclose(fIn);
#endif
}


void printMATRIX(C_REAL *m, int I, int J) {	
	printf("--------------------- matrix --------------------\n");
	printf("             ");
	for (int j = 0; j < J; j++) {
		if (j < 10)
			printf("%i      ", j);
		else if (j < 100)
			printf("%i     ", j);
		else 
			printf("%i    ", j);
	}
	printf("\n");

	for (int i = 0; i < I; i++) {
		if (i<10)
			printf("Line   %i: ", i);
		else if (i<100)
			printf("Line  %i: ", i);
		else
			printf("Line %i: ", i);

		for (int j = 0; j < J; j++)
			printf("%5.4f ", m[i*J + j]);
		printf("\n");
	}
}


void print_WH(C_REAL *W, C_REAL *Htras, int N, int M, int K) {
	for (int i = 0; i < N; i++){
		printf("W[%i]: ", i);

		for (int j = 0; j < K; j++)
			printf("%f ", W[i*K + j]);

		printf("\n");
	}

	for (int i = 0; i < K; i++){
		printf("H[%i]: ", i);

        	for (int j = 0; j < M; j++)
				printf("%f ", Htras[j*K + i]);

		printf("\n");
	}
}


C_REAL *get_V(int N, int M, char* file_name) {
	C_REAL *V = get_memory2D_in_1D(N, M);

#ifndef RANDOM
	FILE *fIn = fopen(file_name, "r");
	fread(V, sizeof(C_REAL), N*M, fIn);
	fclose(fIn);
#else
    srand( 0 );

    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            V[i*M + j] = ((C_REAL)(rand()))/RAND_MAX;
#endif
	return V;
}


/* Gets the difference between matrix_max_index_h and conn_last matrices. */
int get_difference(unsigned char *classification, unsigned char *last_classification, int nx) {
	int diff = 0;
	int conn, conn_last;
	
	for(int i = 0; i < nx; i++)
		for(int j = i+1; j < nx; j++) {
			conn = (int)( classification[j] == classification[i] );
			conn_last = (int) ( last_classification[j] == last_classification[i] );
			diff += ( conn != conn_last );
		}

	return diff;
}


/* Get consensus from the classificacion vector */
void get_consensus(unsigned char *classification, unsigned char *consensus, int nx) {
	unsigned char conn;
	int ii = 0;
	
	for(int i = 0; i < nx; i++)
		for(int j = i+1; j < nx; j++) {
			conn = ( classification[j] == classification[i] );
			consensus[ii] += conn;
			ii++;
		}
}


/* Obtain the classification vector from the Ht matrix */
void get_classification(buffer<C_REAL, 1> &b_Htras, unsigned char *classification,
    int M, int K)
{
    auto Htras = b_Htras.get_access<sycl_read>();
	C_REAL max;
	
	for (int i = 0; i < M; i++) {
		max = 0.0;
		for (int j = 0; j < K; j++)
			if (Htras[i*K + j] > max) {
				classification[i] = (unsigned char)(j);
				max = Htras[i*K + j];
			}
	}
}


C_REAL get_Error(buffer<C_REAL, 1> &b_V, buffer<C_REAL, 1> &b_W, 
    buffer<C_REAL, 1> &b_Htras, int N, int M, int K) 
{
	/*
	* norm( V-WH, 'Frobenius' ) == sqrt( sum( diag( (V-WH)'* (V-WH) ) )
	* norm( V-WH, 'Frobenius' )**2 == sum( diag( (V-WH)'* (V-WH) ) )
	*/

	/*
	* d[1..m] = diag( (V-WH)t * (V-WH) )
	*
	* is equivalent to:
	*     for i=1..m
	*         d[i] = sum( V-WH[:,i] .* V-WH[:,i] )
	*
	*
	* is equivalent to; error = sum( ( V-Vnew[:,i] .* V-Vnew[:,i] )
	*
	*/
    auto V = b_V.get_access<sycl_read>();
    auto W = b_W.get_access<sycl_read>();
    auto Htras = b_Htras.get_access<sycl_read>();
	C_REAL error = 0.0;
	C_REAL Vnew;

	for(int i = 0; i < N; i++) {
		for(int j = 0; j < M; j++){
			Vnew = 0.0;
			for(int k = 0; k < K; k++)
				Vnew += W[i*K + k] * Htras[j*K + k];

			error += (V[i*M + j] - Vnew) * (V[i*M + j] - Vnew);
		}
	}
	
	return error;
}


void writeSolution(C_REAL *W, C_REAL*Ht, unsigned char *consensus, int N, int M,
    int K, int nTests)
{
	FILE *fOut;
	char file[100];
	C_REAL *H = new C_REAL[K*M];
	
	for (int i = 0; i < K; i++)
		for (int j = 0; j < M; j++)
			H[i*M + j] = Ht[j*K + i];
	
	sprintf(file,"solution-NMFLeeSeung_%i", K);
	fOut = fopen(file, "w");
	fwrite( &N, sizeof(int), 1, fOut);
	fwrite( &M, sizeof(int), 1, fOut);
	fwrite( &K, sizeof(int), 1, fOut);
	fwrite( W, sizeof(C_REAL), N*K, fOut);
	fwrite( H, sizeof(C_REAL), K*M, fOut);
	fwrite( &nTests, sizeof(int), 1, fOut);
	fwrite( consensus, sizeof(unsigned char), (M*(M-1))/2, fOut);
	fclose( fOut );
	delete [] H;
}


void nmf(int niter, queue &q, 
	buffer<C_REAL, 1> &b_V, buffer<C_REAL, 1> &b_WH, 
	buffer<C_REAL, 1> &b_W, buffer<C_REAL, 1> &b_Htras, 
    buffer<C_REAL, 1> &b_Waux, buffer<C_REAL, 1> &b_Haux,
	buffer<C_REAL, 1> &b_accW, buffer<C_REAL, 1> &b_accH,
	int N, int M, int K)
{
	/*************************************/
	/*                                   */
	/*      Main Iterative Process       */
	/*                                   */
	/*************************************/
	for (int iter = 0; iter < niter; iter++) {
		/*******************************************/
		/*** H = H .* (W'*(V./(W*H))) ./ accum_W ***/
		/*******************************************/

        W_mult_H(q, b_WH, b_W, b_Htras, N, M, K);	/* WH = W*H */
        V_div_WH(q, b_V, b_WH, N, M);			/* WH = (V./(W*H) */
        accum(q, b_accW, b_W, N, K); 		/* Shrink into one column */
        Wt_mult_WH(q, b_Haux, b_W, b_WH, N, M, K);	/* Haux = (W'* {V./(WH)} */
        mult_M_div_vect(q, b_Htras, b_Haux, b_accW, M, K);/* H = H .* (Haux) ./ accum_W */

		/*******************************************/
		/*** W = W .* ((V./(W*H))*H') ./ accum_H ***/
		/*******************************************/
        W_mult_H(q, b_WH, b_W, b_Htras, N, M, K);	/* WH = W*H */
        V_div_WH(q, b_V, b_WH, N, M );			/* WH = (V./(W*H) */
        WH_mult_Ht(q, b_Waux, b_WH, b_Htras, N, M, K);/* Waux =  {V./(W*H)} *H' */
        accum(q, b_accH, b_Htras, M, K);		/* Shrink into one column */
        mult_M_div_vect(q, b_W, b_Waux, b_accH, N, K);/* W = W .* Waux ./ accum_H */
    }
}


int main(int argc, char *argv[]) {
	const property_list props = property::buffer::use_host_ptr();
	int niters;

	C_REAL *h_V, *h_WH, *h_W, *h_Htras, *h_Haux, *h_Waux, *h_acumm_W, *h_acumm_H;
	C_REAL *W_best, *Htras_best;
	unsigned char *classification, *last_classification;
	unsigned char *consensus;

	int stop;
	char file_name[255];
	int iter;
	int diff, inc;
	
	double time0, time1;
	
	C_REAL error;
	C_REAL error_old = 9.99e+50;

    setbuf( stdout, NULL );
	
	if (argc != 7) {
		printf("./exec dataInput.bin N M K nTests stop_threshold (argc=%i %i)\n", argc, atoi(argv[2]));
		return 1;
	}

	strcpy(file_name, argv[1]);
	int N              = atoi(argv[2]);
	int M              = atoi(argv[3]);
	int K              = atoi(argv[4]);
	int nTests         = atoi(argv[5]);
	int stop_threshold = atoi(argv[6]);

    printf("file=%s\nN=%i M=%i K=%i nTests=%i stop_threshold=%i\n", file_name, N, M, K, nTests, stop_threshold);

#if defined(INTEL_IGPU_DEVICE)
	NEOGPUDeviceSelector selector{};
#elif defined(NVIDIA_DEVICE)
	CUDASelector selector{};
#elif defined(CPU_DEVICE)	
	cpu_selector selector{};
#else
	default_selector selector{};
#endif

	sycl::queue q{selector};
	std::cout << "Running on "
				<< q.get_device().get_info<sycl::info::device::name>()
				<< std::endl;

	h_V                 = get_V(N, M, file_name);
	h_W                 = get_memory2D_in_1D(N, K);
	h_Htras             = get_memory2D_in_1D(M, K);
	h_WH                = get_memory2D_in_1D(N, M);
	h_Haux              = get_memory2D_in_1D(M, K);
	h_Waux              = get_memory2D_in_1D(N, K);
	h_acumm_W           = get_memory1D(K);
	h_acumm_H           = get_memory1D(K);

    W_best              = get_memory2D_in_1D(N, K);
    Htras_best          = get_memory2D_in_1D(M, K);
    classification      = get_memory1D_uchar(M);
	last_classification = get_memory1D_uchar(M);
	consensus           = get_memory1D_uchar(M*(M-1)/2);

    buffer<C_REAL, 1> b_V(h_V, N * M, props);
    buffer<C_REAL, 1> b_W(h_W, N * K, props);
    buffer<C_REAL, 1> b_Htras(h_Htras, M * K, props);
    buffer<C_REAL, 1> b_WH(h_WH, N * M, props);
    buffer<C_REAL, 1> b_Haux(h_Haux, M * K, props);
    buffer<C_REAL, 1> b_Waux(h_Waux, N * K, props);
    buffer<C_REAL, 1> b_acumm_W(h_acumm_W, K, props);
    buffer<C_REAL, 1> b_acumm_H(h_acumm_H, K, props);

	/**********************************/
	/******     MAIN PROGRAM     ******/
	/**********************************/
	time0 = gettime();

	for(int test = 0; test < nTests; test++) {
		/* Init W and H */
		initWH(b_W, b_Htras, N, M, K);

		niters = 2000 / NITER_TEST_CONV;

		stop   = 0;
		iter   = 0;
		inc    = 0;
		while(iter < niters && !stop) {
			iter++;

			/* Main Proccess of NMF Brunet */
			nmf(NITER_TEST_CONV, q, b_V, b_WH, b_W, 
				b_Htras, b_Waux, b_Haux, b_acumm_W, b_acumm_H,
				N, M, K);

			/* Adjust small values to avoid undeflow: h=max(h,eps);w=max(w,eps); */
			adjust_WH(q, b_W, b_Htras, N, M, K);

			/* Test of convergence: construct connectivity matrix */
			get_classification(b_Htras, classification, M, K);

			diff = get_difference(classification, last_classification, M);
			matrix_copy1D_uchar(classification, last_classification, M);

			if(diff > 0) 	/* If connectivity matrix has changed, then: */
				inc = 0;  /* restarts count */
			else		/* else, accumulates count */
				inc++;

			if (verbose)
				printf("iter=%i inc=%i number_changes=%i\n", iter*NITER_TEST_CONV, inc, 2*diff);

			/* Breaks if connectivity stops changing: NMF converged */
			if (inc > stop_threshold)
				stop = 1;
		}

		/* Get Matrix consensus */
		get_consensus(classification, consensus, M);

		/* Get variance of the method error = |V-W*H| */
		error = get_Error(b_V, b_W, b_Htras, N, M, K);
		if (error < error_old) {
			printf("Better W and H, Error %e Test=%i, Iter=%i\n", error, test, iter);
			matrix_copy2D(b_W, W_best, N, K);
			matrix_copy2D(b_Htras, Htras_best, M, K);
			error_old = error;
		}
	}
	time1 = gettime();
	/**********************************/
	/**********************************/

	printf("\n\n\n EXEC TIME %f (us).       N=%i M=%i K=%i Tests=%i (%lu)\n", time1-time0, N, M, K, nTests, sizeof(C_REAL));
	printf("Final error %e \n", error);

	/* Write the solution of the problem */
	writeSolution(W_best, Htras_best, consensus, N, M, K, nTests);

	//printMATRIX(W_best, N, K);

    /* Free memory used */
	delete_memory1D(h_V);
	delete_memory1D(h_W);
	delete_memory1D(h_Htras);
	delete_memory1D(h_WH);
	delete_memory1D(h_Haux);
	delete_memory1D(h_Waux);
	delete_memory1D(h_acumm_W);
	delete_memory1D(h_acumm_H);
	delete_memory1D(W_best);
	delete_memory1D(Htras_best);
	delete_memory1D_uchar(classification);
	delete_memory1D_uchar(last_classification);
	delete_memory1D_uchar(consensus);

	return 0;
}
