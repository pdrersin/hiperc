/**********************************************************************************
 This file is part of Phase-field Accelerator Benchmarks, written by Trevor Keller
 and available from https://github.com/usnistgov/phasefield-accelerator-benchmarks.

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
 \file  opencl_data.c
 \brief Implementation of functions to create and destroy OpenCLData struct
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opencl_data.h"
#include "opencl_kernels.h"

void report_error(cl_int status)
{
	if (status < -999) {
		printf("OpenCL extension (driver) error: %i\n", status);
		exit(-1);
	} else if (status < -29) {
		printf("OpenCL compilation error: %i\n", status);
		exit(-1);
	} else if (status < 0) {
		printf("OpenCL runtime error: %i\n", status);
		exit(-1);
	}
}

void init_opencl(fp_t** conc_old, fp_t** mask_lap, fp_t bc[2][2],
               int nx, int ny, int nm, struct OpenCLData* dev)
{
	size_t gridSize = nx * ny * sizeof(fp_t);
	size_t maskSize = nm * nm * sizeof(fp_t);
	size_t bcSize = 2 * 2 * sizeof(fp_t);
	size_t numDevices;

	cl_int status;
	cl_device_id gpu;
	cl_device_id* devices;
	cl_uint numPlatforms;

	status = clGetPlatformIDs(0, NULL, &numPlatforms);

	cl_platform_id platforms[numPlatforms];
	status = clGetPlatformIDs(numPlatforms, platforms, NULL);
	report_error(status);

	cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (int) platforms[0], 0};
	dev->context = clCreateContextFromType(properties, CL_DEVICE_TYPE_ALL, 0, NULL, &status);
	report_error(status);

	status = clGetContextInfo(dev->context, CL_CONTEXT_DEVICES, 0, NULL, &numDevices);
	report_error(status);
	devices = malloc(numDevices);
	status = clGetContextInfo(dev->context, CL_CONTEXT_DEVICES, numDevices, devices, NULL);
	report_error(status);

	gpu = devices[0];
	dev->commandQueue = clCreateCommandQueue(dev->context, gpu, 0, &status);
	report_error(status);

	/* allocate memory on device */
	dev->conc_old = clCreateBuffer(dev->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, gridSize, conc_old[0], &status);
	report_error(status);
	dev->conc_new = clCreateBuffer(dev->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, gridSize, NULL, &status);
	report_error(status);
	dev->conc_lap = clCreateBuffer(dev->context, CL_MEM_READ_WRITE, gridSize, NULL, &status);
	report_error(status);
	dev->mask = clCreateBuffer(dev->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, maskSize, mask_lap[0], &status);
	report_error(status);
	dev->bc = clCreateBuffer(dev->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bcSize, bc[0], &status);
	report_error(status);

	/* read programs from kernel files */
	build_program("kernel_boundary.cl", dev->context, &gpu, &(dev->boundary_program), &status);
	report_error(status);
	build_program("kernel_convolution.cl", dev->context, &gpu, &(dev->convolution_program), &status);
	report_error(status);
	build_program("kernel_diffusion.cl", dev->context, &gpu, &(dev->diffusion_program), &status);
	report_error(status);

	/* prepare kernels compatible with the just-in-time (JIT) compiler */
	dev->boundary_kernel = clCreateKernel(dev->boundary_program, "boundary_kernel", &status);
	report_error(status);
	dev->convolution_kernel = clCreateKernel(dev->convolution_program, "convolution_kernel", &status);
	report_error(status);
	dev->diffusion_kernel = clCreateKernel(dev->diffusion_program, "diffusion_kernel", &status);
	report_error(status);

	free(devices);

	/* That's a lot of source code required to simply prepare your accelerator to do work. */
}

void build_program(const char* filename,
                  cl_context context,
                  cl_device_id* gpu,
                  cl_program* program,
                  cl_int* status)
{
	FILE *fp;
	char *source_str;
	size_t source_len, program_size;

	fp = fopen(filename, "rb");
	if (!fp) {
	    printf("Failed to load kernel %s\n", filename);
	    exit(-1);
	}

	fseek(fp, 0, SEEK_END);
	program_size = ftell(fp);
	rewind(fp);

	source_str = (char*)malloc(program_size + 1);
	source_str[program_size] = '\0';

	fread(source_str, sizeof(char), program_size, fp);
	fclose(fp);

	source_len = strlen(source_str);
	*program = clCreateProgramWithSource(context, 1, (const char **)&source_str, &source_len, status);

	*status = clBuildProgram(*program, 0, NULL, NULL, NULL, NULL);

	free(source_str);
}

void free_opencl(struct OpenCLData* dev)
{
	/* free memory on device */
	clReleaseContext(dev->context);

	clReleaseKernel(dev->boundary_kernel);
	clReleaseKernel(dev->convolution_kernel);
	clReleaseKernel(dev->diffusion_kernel);

	clReleaseProgram(dev->boundary_program);
	clReleaseProgram(dev->convolution_program);
	clReleaseProgram(dev->diffusion_program);

	clReleaseCommandQueue(dev->commandQueue);
}
