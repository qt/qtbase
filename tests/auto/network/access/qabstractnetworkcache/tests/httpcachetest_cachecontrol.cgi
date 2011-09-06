#!/bin/bash
if [ ! -z ${HTTP_IF_MODIFIED_SINCE} ] ; then
    echo "Status: 304"
    echo ""
    exit;
fi

cc=`echo "${QUERY_STRING}" | sed -e s/%20/\ /g`
echo "Cache-Control: $cc"
echo "Last-Modified: Sat, 31 Oct 1981 06:00:00 GMT"
echo "Content-type: text/html";
echo ""
echo "Hello World!"
