#!/bin/bash
if [ ${HTTP_IF_MODIFIED_SINCE} == "Sat, 31 Oct 1981 06:00:00 GMT" ] ; then
    echo "Status: 304"
    echo ""
    exit;
fi

echo "Last-Modified: Sat, 31 Oct 1981 06:00:00 GMT"
echo "Content-type: text/html";
echo ""
echo "Hello World!"
