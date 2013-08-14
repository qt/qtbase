#!/bin/sh

echo "Content-type: text/plain";
echo
echo "file stored under 'testfile'"
cat 2>&1 > testfile 
