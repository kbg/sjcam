#!/bin/bash

GSJC=gsjc1

INDIR=/srv/$GSJC/out
OUTDIR=/instruments/gsjc/$GSJC

while : ; do
    files=$(find "$INDIR" -name "*.fits.gz")
    for f in $files; do
        mv $f "$OUTDIR/"
    done
    sleep 1
done
