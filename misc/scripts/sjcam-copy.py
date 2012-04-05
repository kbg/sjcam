#!/usr/bin/env python
#
# Copyright (c) 2012 Kolja Glogowski
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

import os, sys, signal, re
from subprocess import call
from time import sleep

gzip_cmd = ['/bin/gzip', '-1']
mkdir_cmd = ['/bin/mkdir', '-p']
mv_cmd = ['/bin/mv']
re_fname = re.compile(r'^(.*)_(\d{4})(\d{2})(\d{2})-\d{9}\.fits(?:\.gz)?$')

def gzip(fname):
    return call(gzip_cmd + [fname]) == 0

def mkdir(dname):
    return call(mkdir_cmd + [dname]) == 0

def mv(src, dst):
    return call(mv_cmd + [src, dst]) == 0

def move_file(fname, srcdir, destbasedir, verbose=False):
    m = re_fname.match(fname)
    if not m:
        return False
    dstdir = os.path.join(dstbasedir, *m.groups())
    if not os.path.isdir(dstdir):
        if not mkdir(dstdir):
            return False
    if verbose:
        sys.stdout.write(os.path.join(dstdir, fname))
    return mv(os.path.join(srcdir, fname), dstdir)

if __name__ == '__main__':
    from optparse import OptionParser

    parser = OptionParser(usage='usage: sjcam-copy [options]')
    parser.formatter.max_help_position = 30
    parser.add_option('-i', '--indir', dest='indir',
                      help='input directory')
    parser.add_option('-o', '--outdir', dest='outdir',
                      help='output base directory')
    parser.add_option('-v', '--verbose', action='store_true',
                      dest='verbose', default=False,
                      help='verbose text output')
    opts, args = parser.parse_args()

    if args:
        parser.error('Invalid arguments specified.')
    if not opts.indir:
        parser.error('No input directory specified.')
    if not opts.outdir:
        parser.error('No output directory specified.')

    verbose = opts.verbose
    srcdir = os.path.abspath(opts.indir)
    dstbasedir = os.path.abspath(opts.outdir)
    if not os.path.isdir(srcdir):
        parser.error('Input directory does not exist.')
    if not os.path.isdir(dstbasedir):
        parser.error('Output directory does not exist.')

    # Install SIGINT and SIGTERM handler that only sets a global quit flag
    quit = False
    def sighandler(sig, frame):
        global quit
        quit = True
    signal.signal(signal.SIGTERM, sighandler)
    signal.signal(signal.SIGINT, sighandler)

    # Loop until a SIGINT or SIGTERM occurs
    while not quit:
        fnlist = [fn for fn in os.listdir(srcdir) if fn.endswith('.fits')]
        for fn in fnlist:
            if verbose:
                sys.stdout.write(os.path.join(srcdir, fn) + ' -> ')
            if gzip(os.path.join(srcdir, fn)):
                move_file(fn + '.gz', srcdir, dstbasedir, verbose)
            if verbose:
                sys.stdout.write('\n')
                sys.stdout.flush()
            if quit:
                break
        sleep(1)
