/*
 * Copyright 2018 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors: Jérôme Glisse <jglisse@redhat.com>
 */
#ifndef SVM_CL_TESTS_HELPERS_H
#define SVM_CL_TESTS_HELPERS_H

#define CL_TARGET_OPENCL_VERSION 220

#include <CL/opencl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <hugetlbfs.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

#define ALIGN(v, a) (((v) + ((a) - 1)) & ~((a) - 1))


void *mem_anon_map(size_t size)
{
    void *res;

    /* Align on 4K pages ... no real need for that though ... */
    size = ALIGN(size, 1 << 12);

    res = mmap(0, size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        return NULL;
    }
    return res;
}

void *mem_share_map(size_t size)
{
    void *res;

    /* Align on 4K pages ... no real need for that though ... */
    size = ALIGN(size, 1 << 12);

    res = mmap(0, size, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        return NULL;
    }
    return res;
}

void *mem_file_map_private(int fd, size_t size)
{
    void *res;

    /* Align on 4K pages ... no real need for that though ... */
    size = ALIGN(size, 1 << 12);

    res = mmap(0, size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_FILE, fd, 0);
    if (res == MAP_FAILED) {
        return NULL;
    }
    return res;
}

void *mem_file_map_share(int fd, size_t size)
{
    void *res;

    /* Align on 4K pages ... no real need for that though ... */
    size = ALIGN(size, 1 << 12);

    res = mmap(0, size, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_FILE, fd, 0);
    if (res == MAP_FAILED) {
        return NULL;
    }
    return res;
}

void mem_unmap(void *ptr, size_t size)
{
    size = ALIGN(size, 1 << 12);
    munmap(ptr, size);
}


void *hugefs_alloc(size_t size)
{
    long pagesizes[4];
    int n, i, idx;
    void *res;

    n = gethugepagesizes(pagesizes, 4);
    if (n <= 0 || n > 4) {
        return NULL;
    }
    for (i = idx = 0; i < n; ++i) {
        if (pagesizes[i] < pagesizes[idx]) {
            idx = i;
        }
    }

    printf("Use hugepagesize %ld 0x%08lx\n", pagesizes[idx], pagesizes[idx]);

    size = ALIGN(size, pagesizes[idx]);

    res = get_hugepage_region(size, GHR_STRICT);
    return res;
}

void hugefs_free(void *ptr)
{
    free_hugepage_region(ptr);
}


enum status {
    SUCCESS = 0,
    WARNING,
    ERROR,
};

static inline void print_status(enum status status, char *argv[],
                                const char *msg, ...)
{
    va_list ap;

    switch (status) {
    case SUCCESS:
        printf("\r[\033[0;32mOK\033[0m] %s ", argv[0]);
        break;
    case WARNING:
        printf("\r[\033[0;33mWW\033[0m] %s ", argv[0]);
        break;
    case ERROR:
        // Fall-through
    default:
        printf("\r[\033[0;31mEE\033[0m] %s ", argv[0]);
        break;
    }

    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);

    fflush(stdout);
}


struct cl_program {
    cl_command_queue queue;
    cl_device_id device_id;
    cl_platform_id platform;
    cl_program program;
    cl_context context;
    cl_kernel kernel;
    cl_mem mem_a;
    cl_mem mem_b;
    cl_mem mem_r;
    unsigned nwords;
    int *a;
    int *b;
    int *r;
};

static const char *kernel =                                     "\n" \
"__kernel void dumb(__global int *a,                             \n" \
"                   __global int *b,                             \n" \
"                   __global int *r,                             \n" \
"                   const unsigned int n)                        \n" \
"{                                                               \n" \
"    int id = get_global_id(0);                                  \n" \
"    // Bounds check                                             \n" \
"    if (id < n)                                                 \n" \
"        r[id] = a[id] + b[id];                                  \n" \
"}                                                               \n";

static int cl_program_init(struct cl_program *clprog, unsigned nwords)
{
    size_t size = nwords * sizeof(int32_t);
    cl_event event;
    unsigned i;
    cl_int res;

    clprog->nwords = nwords;
    clprog->a = malloc(size);
    clprog->b = malloc(size);
    clprog->r = malloc(size);
    for (i = 0; i < nwords; ++i) {
        clprog->a[i] = i;
        clprog->b[i] = -i;
        clprog->r[i] = 0xcafedead;
    }

    res = clGetPlatformIDs(1, &clprog->platform, NULL);
    if (res != CL_SUCCESS) {
        return -1;
    }

    res = clGetDeviceIDs(clprog->platform, CL_DEVICE_TYPE_GPU,
                         1, &clprog->device_id, NULL);
    if (res != CL_SUCCESS) {
        return -1;
    }

    clprog->context = clCreateContext(0, 1, &clprog->device_id,
                                     NULL, NULL, &res);
    if (res != CL_SUCCESS) {
        return -1;
    }
    clprog->queue = clCreateCommandQueueWithProperties(clprog->context,
                                                       clprog->device_id,
                                                       NULL, &res);
    if (res != CL_SUCCESS) {
        goto error_queue;
    }

    clprog->program = clCreateProgramWithSource(clprog->context, 1,
                                               (const char **)&kernel,
                                               NULL, &res);
    if (res != CL_SUCCESS) {
        goto error_program;
    }
    res = clBuildProgram(clprog->program, 0, NULL, NULL, NULL, NULL);
    if (res != CL_SUCCESS) {
        goto error_build;
    }
    clprog->kernel = clCreateKernel(clprog->program, "dumb", &res);
    if (res != CL_SUCCESS) {
        goto error_kernel;
    }

    clprog->mem_a = clCreateBuffer(clprog->context, CL_MEM_READ_ONLY,
                                   size, NULL, &res);
    if (res != CL_SUCCESS) {
        goto error_buffer_a;
    }
    clprog->mem_b = clCreateBuffer(clprog->context, CL_MEM_READ_ONLY,
                                   size, NULL, &res);
    if (res != CL_SUCCESS) {
        goto error_buffer_b;
    }
    clprog->mem_r = clCreateBuffer(clprog->context, CL_MEM_WRITE_ONLY,
                                   size, NULL, &res);
    if (res != CL_SUCCESS) {
        goto error_buffer_r;
    }

    res = clEnqueueWriteBuffer(clprog->queue, clprog->mem_a, CL_TRUE,
                               0, size, clprog->a, 0, NULL, NULL);
    if (res != CL_SUCCESS) {
        goto error_write_a;
    }
    res = clEnqueueWriteBuffer(clprog->queue, clprog->mem_b, CL_TRUE,
                               0, size, clprog->b, 0, NULL, NULL);
    if (res != CL_SUCCESS) {
        goto error_write_b;
    }
    res = clEnqueueWriteBuffer(clprog->queue, clprog->mem_r, CL_TRUE,
                               0, size, clprog->r, 0, NULL, &event);
    if (res != CL_SUCCESS) {
        goto error_write_r;
    }

    res = clWaitForEvents(1, &event);
    if (res != CL_SUCCESS) {
        goto error_wait;
    }

    return 0;

error_wait:
error_write_r:
error_write_b:
error_write_a:
error_buffer_r:
    clReleaseMemObject(clprog->mem_b);
error_buffer_b:
    clReleaseMemObject(clprog->mem_a);
error_buffer_a:
    clReleaseKernel(clprog->kernel);
error_build:
error_kernel:
    clReleaseProgram(clprog->program);
error_program:
    clReleaseCommandQueue(clprog->queue);
error_queue:
    clReleaseContext(clprog->context);

    return -1;
}

static int cl_program_run_nocheck(struct cl_program *clprog, void *a, void *b, void *r)
{
    size_t global_size, local_size;
    cl_event event;
    cl_int res;

    if (a) {
        res  = clSetKernelArgSVMPointer(clprog->kernel, 0, a);
        if (res != CL_SUCCESS) {
            return -1;
        }
    } else {
        res = clSetKernelArg(clprog->kernel, 0, sizeof(cl_mem), &clprog->mem_a);
        if (res != CL_SUCCESS) {
            return -1;
        }
    }
    if (b) {
        res  = clSetKernelArgSVMPointer(clprog->kernel, 1, b);
        if (res != CL_SUCCESS) {
            return -1;
        }
    } else {
        res = clSetKernelArg(clprog->kernel, 1, sizeof(cl_mem), &clprog->mem_b);
        if (res != CL_SUCCESS) {
            return -1;
        }
    }
    if (r) {
        res  = clSetKernelArgSVMPointer(clprog->kernel, 2, r);
        if (res != CL_SUCCESS) {
            return -1;
        }
    } else {
        res = clSetKernelArg(clprog->kernel, 2, sizeof(cl_mem), &clprog->mem_r);
        if (res != CL_SUCCESS) {
            return -1;
        }
    }
    res = clSetKernelArg(clprog->kernel, 3, sizeof(unsigned), &clprog->nwords);
    if (res != CL_SUCCESS) {
        return -1;
    }


    local_size = 64;
    global_size = ceil(clprog->nwords / (float)local_size) * local_size;
    res = clEnqueueNDRangeKernel(clprog->queue, clprog->kernel, 1,
                                 NULL, &global_size, &local_size,
                                 0, NULL, &event);
    clFinish(clprog->queue);
    res = clWaitForEvents(1, &event);
    if (res != CL_SUCCESS) {
        return -1;
    }

    if (r) {
        memcpy(clprog->r, r, clprog->nwords * sizeof(int));
    } else {
        clEnqueueReadBuffer(clprog->queue, clprog->mem_r, CL_TRUE, 0,
                            clprog->nwords * sizeof(int), clprog->r,
                            0, NULL, &event);
        clFinish(clprog->queue);
        res = clWaitForEvents(1, &event);
        if (res != CL_SUCCESS) {
            return -1;
        }
    }

    return 0;
}

static int cl_program_run(struct cl_program *clprog, void *a, void *b, void *r)
{
    unsigned i;
    int ret = cl_program_run_nocheck(clprog, a, b, r);

    if (ret)
        return ret;

    for (i = 0; i < clprog->nwords; ++i) {
        if (clprog->r[i]) {
            return -1;
        }
    }

    return 0;
}

static int cl_program_migrate(struct cl_program *clprog, void *mem)
{
    const void *ptrs[1];
    cl_event event;
    size_t size;
    cl_int res;

    ptrs[0] = (void *)((uintptr_t)mem & ~0xFFFUL);
    size = clprog->nwords * sizeof(int);
    size = ALIGN(size, 1 << 12);
    res = clEnqueueSVMMigrateMem(clprog->queue, 1, ptrs, &size,
                                 0, 0, NULL, &event);
    if (res != CL_SUCCESS) {
        return -1;
    }

    res = clWaitForEvents(1, &event);
    if (res != CL_SUCCESS) {
        return -1;
    }

    return 0;
}


#endif /* SVM_CL_TESTS_HELPERS_H */
