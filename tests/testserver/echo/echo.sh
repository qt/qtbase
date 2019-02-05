#!/usr/bin/env bash

# Disabled by default, enable it.
sed -i 's/disable\t\t= yes/disable = no/' /etc/xinetd.d/echo

service xinetd restart
