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
    int res;
    int data[NWORDS];

    res = cl_program_init(&clprog, NWORDS);
    if (res) {
        append = "cl program init failed\n";
        status = ERROR;
        goto out;
    }

    memcpy(data, clprog.a, NWORDS * sizeof(int));

    res = cl_program_run(&clprog, data, NULL, NULL);
    if (res) {
        append = "cl program run failed\n";
        status = ERROR;
        goto out;
    }

out:
    print_status(status, argv, append);
    return 0;
}
