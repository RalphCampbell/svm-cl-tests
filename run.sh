#!/bin/sh
export LD_LIBRARY_PATH=`echo ~/local/lib64`
export LD_PRELOAD=`echo ~/local/lib64`/libOpenCL.so
export NOUVEAU_ENABLE_CL=1
sudo sync
$@
