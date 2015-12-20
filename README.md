
# About

Convert filenames inside ZIP archives from older Russian encodings
(koi8-r, cp866, windows-1251) to UTF-8.

This tool does not touch the file contents, it just renames the files.

# Build and Install

    test -f configure || autoreconf -iv
    ./configure
    make install

# Usage

    runzip <filename.zip>
