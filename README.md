
# About

Convert filenames inside ZIP archives from being Windows-1251 (CP-1251) to UTF-8. This tool does not touch the file contents, it just renames the files.

# Build and Install

    test -f configure || autoreconf -iv
    ./configure
    make install

# Usage

    runzip <filename.zip>
