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

#define NWORDS  (1 << 16)

int main(int argc, char* argv[])
{
    enum status status = SUCCESS;
    struct cl_program clprog;
    char *append = "\n";
    void *map;
    int res;
    unsigned nwords = NWORDS;
    unsigned i;

    if (argc > 1)
        nwords = strtol(argv[1], NULL, 0);

    map = mem_anon_map(nwords * sizeof(int));
    if (map == NULL) {
        append = "mapping anon failed\n";
        status = ERROR;
        goto out;
    }

    res = cl_program_init(&clprog, nwords);
    if (res) {
        append = "cl program init failed\n";
        status = ERROR;
        goto out;
    }

    if (mprotect(map, nwords * sizeof(int), PROT_READ)) {
        append = "mprotect failed\n";
        status = ERROR;
        goto out;
    }
    res = cl_program_migrate(&clprog, map);
    if (res) {
        append = "migrating memory failed\n";
        status = ERROR;
        goto out;
    }

    res = cl_program_run_nocheck(&clprog, map, NULL, NULL);
    if (res) {
        append = "cl program run failed\n";
        status = ERROR;
        goto out;
    }
    for (i = 0; i < nwords; ++i) {
        if (clprog.r[i] != -i) {
            printf("i %u map %d r %d\n", i, ((int *)map)[i], clprog.r[i]); // XXX
            append = "data compare failed\n";
            status = ERROR;
            goto out;
        }
    }

    mem_unmap(map, nwords * sizeof(int));

out:
    print_status(status, argv, append);
    return 0;
}
