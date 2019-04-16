#!/bin/bash
# cache control takes precedence over expires
echo "Cache-Control: max-age=-1"
echo "Expires: Mon, 30 Oct 2028 14:19:41 GMT"
echo "Content-type: text/html";
echo ""
echo "Hello World!"
