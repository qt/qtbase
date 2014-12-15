#!/bin/sh

PSK="1a2b3c4d5e6f"
SERVERHINT="QtTestServerHint"

# openssl s_server will try to read from stdin; if it gets EOF, it will quit
# therefore, we can't simply redirect /dev/null as its stdin,
# but we need a small trick

tail -f /dev/null 2> /dev/null < /dev/null | openssl s_server -quiet -nocert -psk "$PSK" -psk_hint "$SERVERHINT" > /tmp/logopenssl 2>&1 &
