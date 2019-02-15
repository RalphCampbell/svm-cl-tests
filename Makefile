# SPDX-License-Identifier: GPL-2.0
LDFLAGS += -fsanitize=address -fsanitize=undefined
CFLAGS += -std=c99 -D_GNU_SOURCE -I. -g -Og -Wall -I/usr/include/libdrm -Wno-unused-function
LDLIBS += -L/home/glisse/local/lib64 -lrdmacm -libverbs -lhugetlbfs -ldrm -lOpenCL -lm
TARGETS = test-malloc-read test-malloc-write \
	test-malloc-vram-read test-malloc-vram-write \
	test-share-read test-share-write \
	test-hugetlbfs-read test-hugetlbfs-write \
	test-file-read test-file-write \
	test-file-private

targets: $(TARGETS)

$(TARGETS): $(TARGETS:%=%.c) *.h
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $@.c

clean:
	$(RM) $(TARGETS) *.o

%.o: Makefile *.h %.c
	$(CC) $(CFLAGS) -o $@ -c $(@:%.o=%.c)
