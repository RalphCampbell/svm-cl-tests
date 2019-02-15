#!/bin/sh
export LD_LIBRARY_PATH=`echo ~/local/lib64`
export LD_PRELOAD=`echo ~/local/lib64`/libOpenCL.so
sudo sync
$@
