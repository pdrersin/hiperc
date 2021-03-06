/**********************************************************************************
 HiPerC: High Performance Computing Strategies for Boundary Value Problems
 written by Trevor Keller and available from https://github.com/usnistgov/hiperc

 This software was developed at the National Institute of Standards and Technology
 by employees of the Federal Government in the course of their official duties.
 Pursuant to title 17 section 105 of the United States Code this software is not
 subject to copyright protection and is in the public domain. NIST assumes no
 responsibility whatsoever for the use of this software by other parties, and makes
 no guarantees, expressed or implied, about its quality, reliability, or any other
 characteristic. We would appreciate acknowledgement if the software is used.

 This software can be redistributed and/or modified freely provided that any
 derivative works bear some notice that they are derived from it, and any modified
 versions bear some notice that they have been modified.

 Questions/comments to Trevor Keller (trevor.keller@nist.gov)
 **********************************************************************************/

/**
 \file  cuda_data.h
 \brief Declaration of CUDA data container
*/

/** \cond SuppressGuard */
#ifndef _CUDA_DATA_H_
#define _CUDA_DATA_H_
/** \endcond */

#include "type.h"

/**
 \brief Container for pointers to arrays on the GPU
*/
struct CudaData {
	fp_t* conc_old;
	fp_t* conc_new;
	fp_t* conc_lap;
};

/**
 \brief Initialize CUDA device memory before marching
*/
void init_cuda(fp_t** conc_old, fp_t** mask_lap,
               const int nx, const int ny, const int nm, struct CudaData* dev);

/**
 \brief Free CUDA device memory after marching
*/
void free_cuda(struct CudaData* dev);

/**
 \brief Apply boundary conditions on device
*/
void device_boundaries(fp_t* conc,
                      const int nx, const int ny, const int nm,
                      const int bx, const int by);

/**
 \brief Compute convolution on device
*/
void device_convolution(fp_t* conc_old, fp_t* conc_lap,
                         const int nx, const int ny, const int nm,
                         const int bx, const int by);

/**
 \brief Step diffusion equation on device
*/
void device_composition(fp_t* conc_old, fp_t* conc_new, fp_t* conc_lap,
                        const int nx, const int ny, const int nm,
                        const int bx, const int by,
                        const fp_t D, const fp_t dt);

/**
 \brief Solve diffusion equation on the GPU
*/
void cuda_diffusion_solver(struct CudaData* dev, fp_t** conc_new,
                           const int bx, const int by,
                           const int nm, const int nx, const int ny,
                           const fp_t D, const fp_t dt,
                           struct Stopwatch* sw);

/**
  \brief Read data from device
*/
void read_out_result(fp_t** conc, fp_t* d_conc, const int nx, const int ny);

/** \cond SuppressGuard */
#endif /* _CUDA_DATA_H_ */
/** \endcond */
