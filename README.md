
# About

Convert filenames inside ZIP archives from autodetected older Russian encodings
(koi8-r, koi8-u, cp866, windows-1251) to UTF-8.

This tool does not touch the file contents, it just renames the files inside
a ZIP archive.

# Build and Install

    test -f configure || autoreconf -iv
    ./configure
    make install

[![Build Status](https://travis-ci.org/vlm/zip-fix-filename-encoding.svg?branch=master)](https://travis-ci.org/vlm/zip-fix-filename-encoding)

# Usage

    Usage: runzip [OPTIONS] <filename.zip>...
    Where OPTIONS are:
      -h                 Display this help screen
      -n                 Dry run. Do not modify the <file.zip>
      -v                 Verbose output
      -s <encoding>      Set source encoding. Auto-detect, if not set
      -t <encoding>      Set target encoding. Default is UTF-8
      -w                 Make archive readable on Windows (reverse operation)
                         NOTE: -w implies -t cp866 (Yes! MS-DOS!)

