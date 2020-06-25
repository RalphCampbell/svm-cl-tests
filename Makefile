# SPDX-License-Identifier: GPL-2.0
LDFLAGS += -fsanitize=address -fsanitize=undefined
CFLAGS += -D_GNU_SOURCE -I$(HOME)/local/include -I. -g -Og -Wall -I/usr/include/libdrm -Wno-unused-function
LDLIBS += -L$(HOME)/local/lib64 -lhugetlbfs -ldrm -lOpenCL -lm
TARGETS = test-malloc-read test-malloc-write \
	test-malloc-vram-read test-malloc-vram-clear test-malloc-vram-write \
	test-malloc-vram-plus \
	test-share-read test-share-write \
	test-hugetlbfs-read test-hugetlbfs-write \
	test-file-read test-file-write \
	test-file-private test-write-hole \
	test-data-read test-data-write \
	test-stack-read test-stack-write \
	test-thp-read test-thp-write test-malloc-read-zero \
	test-thp-migrate test-thp-zero

targets: $(TARGETS)

$(TARGETS): $(TARGETS:%=%.c) *.h
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $@.c

clean:
	$(RM) $(TARGETS) *.o

%.o: Makefile *.h %.c
	$(CC) $(CFLAGS) -o $@ -c $(@:%.o=%.c)
