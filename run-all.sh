#!/bin/sh
base=`dirname $0`
cd $base
PATH=$PATH:./
tests=`find . -maxdepth 1 -type f -executable  | grep -v .sh`
for i in $tests ; do
    echo Running $i
    $base/run.sh `basename $i`
    sync
    sleep 5
done
