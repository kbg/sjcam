#!/bin/bash

GSJC=gsjc1

INDIR=/srv/$GSJC
OUTDIR=/srv/$GSJC/out

while : ; do
    files=$(find "$INDIR" -name "*.fits")
    for f in $files; do
        #echo -n "."
        gzip $f
        mv $f.gz "$OUTDIR/"
    done
    sleep 1
done
