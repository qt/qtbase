#!/bin/sh
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# generate ca.crt
openssl genrsa -out ca.key 8192
openssl req -x509 -new -sha512 -nodes -key ca.key -days 10000 -out ca.crt -config ca.conf

# generate inter.crt
openssl genrsa -out inter.key 8192
openssl req -new -sha512 -nodes -key inter.key -out inter.csr -config inter.conf
openssl x509 -req -sha512 -days 45 -in inter.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out inter.crt
rm inter.csr
rm ca.srl

# generate leaf.crt
openssl genrsa -out leaf.key 8192
openssl req -new -sha512 -nodes -key leaf.key -out leaf.csr -config leaf.conf
openssl x509 -req -sha512 -days 45 -in leaf.csr -CA inter.crt -CAkey inter.key -CAcreateserial -out leaf.crt
rm leaf.csr
rm inter.srl
