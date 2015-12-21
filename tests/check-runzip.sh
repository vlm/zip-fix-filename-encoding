#!/usr/bin/env bash

set -e
set -u

RUNZIP=${RUNZIP=../src/runzip}

rm -rf processed
cp -r originals processed

${RUNZIP} -vv processed/*.zip

# zipdetails processed/windows-archive.zip
unzip -d processed/windows processed/windows-archive.zip

# zipdetails processed/mac-archive.zip
unzip -d processed/mac processed/mac-archive.zip

ls -alR processed

rm -rf processed
