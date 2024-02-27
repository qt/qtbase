#!/bin/bash
# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Requires mkcert and openssl

warn () { echo "$@" >&2; }
die () { warn "$@"; exit 1; }


command -v mkcert 1>/dev/null 2>&1 || die "Failed to find mkcert"
command -v openssl 1>/dev/null 2>&1 || die "Failed to find openssl"

SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

pushd "$SCRIPTPATH" || die "Unable to pushd to $SCRIPTPATH"
mkcert 127.0.0.1
mkcert -client 127.0.0.1
warn "Remember to run mkcert -install if you haven't already"

# Generate CA
openssl genrsa -out ca-key.pem 2048
openssl req -new -x509 -noenc -days 365 -key ca-key.pem -out rootCA.pem

# Generate accepted client certificate
openssl genrsa -out accepted-client-key.pem 2048
openssl req -new -sha512 -nodes -key accepted-client-key.pem -out accepted-client.csr -config accepted-client.conf
openssl x509 -req -sha512 -days 45 -in accepted-client.csr -CA rootCA.pem -CAkey ca-key.pem -CAcreateserial -out accepted-client.pem
rm accepted-client.csr
rm rootCA.srl

popd || die "Unable to popd"
