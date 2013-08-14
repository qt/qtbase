#!/bin/bash
if [ ! -z ${HTTP_IF_NONE_MATCH} ] ; then
    echo "Status: 304"
    echo ""
    exit;
fi

echo "ETag: foo"
echo "Content-type: text/html";
echo ""
echo "Hello World!"
