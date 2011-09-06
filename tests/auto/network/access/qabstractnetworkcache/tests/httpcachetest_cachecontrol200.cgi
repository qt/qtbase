#!/bin/bash
cc=`echo "${QUERY_STRING}" | sed -e s/%20/\ /g`
echo "Status: 200"
echo "Cache-Control: $cc"
echo "Last-Modified: Sat, 31 Oct 1981 06:00:00 GMT"
echo "Content-type: text/html";
echo "X-Script: $0"
echo ""
echo "Hello World!"
