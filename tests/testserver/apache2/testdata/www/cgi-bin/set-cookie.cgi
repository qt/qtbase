#!/bin/sh

echo "Content-type: text/plain"
while read line; do
    echo "Set-Cookie: $line"
done

echo
echo "Success"
