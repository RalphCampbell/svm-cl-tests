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
#include "helpers.h"

#define NWORDS  (1 << 19)
#define TWOMEG  (1 << 21)

int main(int argc, char* argv[])
{
    enum status status = SUCCESS;
    struct cl_program clprog;
    char *append = "\n";
    void *map_orig;
    void *map;
    int res;

    map_orig = mem_anon_map(2 * TWOMEG);
    if (map_orig == NULL) {
        append = "mapping anon failed\n";
        status = ERROR;
        goto out;
    }
    map = (void *)ALIGN((uintptr_t)map_orig, TWOMEG);
    if (madvise(map, TWOMEG, MADV_HUGEPAGE)) {
        append = "madvise huge failed\n";
        status = ERROR;
        goto out;
    }

    res = cl_program_init(&clprog, NWORDS);
    if (res) {
        append = "cl program init failed\n";
        status = ERROR;
        goto out;
    }

    memcpy(map, clprog.a, NWORDS * sizeof(int));
#if 0
    if (mprotect(map, NWORDS * sizeof(int), PROT_READ)) {
        append = "mprotect failed\n";
        status = ERROR;
        goto out;
    }
#endif

    res = cl_program_run(&clprog, map, NULL, NULL);
    if (res) {
        append = "cl program run failed\n";
        status = ERROR;
        goto out;
    }

    mem_unmap(map_orig, 2 * TWOMEG);

out:
    print_status(status, argv, append);
    return 0;
}
