#!/bin/sh

echo "Content-type: text/plain";
echo
cat $(dirname $(readlink -f $0))/../rfc3252
sleep 1s
