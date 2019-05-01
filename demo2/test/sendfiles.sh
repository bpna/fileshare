#!/bin/sh

for x in {1..1000}
do
    ./client upload_file $1 $2 $x
done
