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
    int res, fd;
    unsigned i;
    void *map;
    ssize_t len;

    /* Create file and initialize its content. */
    fd = open("/tmp/." __FILE__, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
    if (fd < 0) {
        append = "creating file failed\n";
        status = ERROR;
        goto out;
    }
    for (i = 0; i < NWORDS; ++i) {
        write(fd, &i, sizeof(int));
    }
    fsync(fd);
    map = mem_file_map_private(fd, NWORDS * sizeof(int));
    if (map == NULL) {
        append = "mapping file failed\n";
        status = ERROR;
        goto out;
    }

    res = cl_program_init(&clprog, NWORDS);
    if (res) {
        append = "cl program init failed\n";
        status = ERROR;
        goto out;
    }

    res = cl_program_run(&clprog, map, NULL, NULL);
    if (res) {
        append = "cl program run failed\n";
        status = ERROR;
        goto out;
    }

    /* Close file, and re-open it and check its content. */
    mem_unmap(map, NWORDS * sizeof(int));
    close(fd);
    fd = open("/tmp/." __FILE__, O_RDWR);
    if (fd < 0) {
        append = "opening file failed\n";
        status = ERROR;
        goto out;
    }
    for (i = 0; i < NWORDS; ++i) {
        unsigned v;

        len = read(fd, &v, sizeof(int));
        if (len != sizeof(v) || v != i) {
            append = "checking file failed\n";
            status = ERROR;
            goto out;
        }
    }

out:
    print_status(status, argv, append);
    return 0;
}
