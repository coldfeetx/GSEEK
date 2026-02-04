#!/bin/bash
ctags -R
find . -name "*.c" -o -name "*.h" > cscope.files
cscope -q -R -b -i cscope.files
echo "Created Code Cross-Reference"
