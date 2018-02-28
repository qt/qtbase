#!/bin/sh

echo "Content-type: text/plain";
echo "Content-length: 33"
echo
md5sum | cut -f 1 -d " "
