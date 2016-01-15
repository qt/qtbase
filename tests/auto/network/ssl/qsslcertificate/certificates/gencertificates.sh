#!/bin/sh
#############################################################################
##
## Copyright (C) 2016 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

# This script generates digital certificates of different types.

#--- RSA Certificates -----------------------------------------------------------------------

echo -e "\ngenerating 1024-bit RSA private key to PEM file ..."
openssl genrsa -out rsa-pri-1024.pem 1024

echo -e "\ngenerating the corresponding public key to PEM and DER file ..."
openssl rsa -in rsa-pri-1024.pem -pubout -out rsa-pub-1024.pem
openssl rsa -in rsa-pri-1024.pem -pubout -out rsa-pub-1024.der -outform der

echo -e "\ngenerating certificate signing request (CSR) ..."
openssl req -out req.pem -new -key rsa-pri-1024.pem -subj "/CN=name\/with\/slashes/C=NO"

echo -e "\n generating a self-signed certifificate to PEM file ..."
openssl x509 -req -in req.pem -out cert-ss.pem -signkey rsa-pri-1024.pem

echo -e "\n generating a self-signed certifificate to DER file ..."
openssl x509 -req -in req.pem -out cert-ss.der -signkey rsa-pri-1024.pem -outform der

echo -e "\n generating a certifificate signed by a dummy CA to PEM file ..."
openssl x509 -req -in req.pem -out cert.pem -CA ca-cert.pem -set_serial 17

echo -e "\n generating a certifificate signed by a dummy CA to DER file ..."
openssl x509 -req -in req.pem -out cert.der -CA ca-cert.pem -set_serial 17 -outform der

#--- DSA Certificates -----------------------------------------------------------------------
echo -e "\ngenerating DSA parameters to PEM file ..."
openssl dsaparam -out dsapar-1024.pem 1024

echo -e "\ngenerating DSA private key to PEM file ..."
openssl gendsa dsapar-1024.pem -out dsa-pri-1024.pem
/bin/rm dsapar-1024.pem

echo -e "\ngenerating DSA public key to PEM and DER file ..."
openssl dsa -in dsa-pri-1024.pem -pubout -out dsa-pub-1024.pem
openssl dsa -in dsa-pri-1024.pem -pubout -out dsa-pub-1024.der -outform der

echo -e "\ngenerating certificate signing request (CSR) ..."
openssl req -out req.pem -new -key dsa-pri-1024.pem -subj "/CN=name\/with\/slashes/C=NO"

echo -e "\n generating a self-signed certifificate to PEM file ..."
openssl x509 -req -in req.pem -out dsa-cert-ss.pem -signkey dsa-pri-1024.pem

#--- EC Certificates ------------------------------------------------------------------------
echo -e "\ngenerating EC private key to PEM file ..."
openssl ecparam -name secp384r1 -genkey -noout -out ec-pri-384.pem

echo -e "\ngenerating EC public key to PEM and DER file ..."
openssl ec -in ec-pri-384.pem -pubout -out ec-pub-384.pem
openssl ec -in ec-pri-384.pem -pubout -out ec-pub-384.der -outform DER

echo -e "\ngenerating certificate signing request (CSR) ..."
openssl req -out req.pem -new -key ec-pri-384.pem -subj "/CN=name\/with\/slashes/C=NO"

echo -e "\n generating a self-signed certifificate to PEM file ..."
openssl x509 -req -in req.pem -out ec-cert-ss.pem -signkey ec-pri-384.pem

#--- Public keys --------------------------------------------------------------------------------
echo -e "\n associate public keys with all certificates ..."
# Note: For now, there is only one public key (encoded in both PEM and DER), but that could change.
/bin/cp rsa-pub-1024.pem cert-ss.pem.pubkey
/bin/cp rsa-pub-1024.der cert-ss.der.pubkey
/bin/cp rsa-pub-1024.pem cert.pem.pubkey
/bin/cp rsa-pub-1024.der cert.der.pubkey
/bin/cp dsa-pub-1024.pem dsa-cert-ss.pem.pubkey
/bin/cp dsa-pub-1024.der dsa-cert-ss.der.pubkey
/bin/cp ec-pub-384.pem ec-cert-ss.pem.pubkey
/bin/cp ec-pub-384.der ec-cert-ss.der.pubkey

#--- Digests --------------------------------------------------------------------------------
echo -e "\n generating md5 and sha1 digests of all certificates ..."
for digest in md5 sha1
do
  openssl x509 -in ca-cert.pem -noout -fingerprint -$digest > ca-cert.pem.digest-$digest
  openssl x509 -in cert-ss.pem -noout -fingerprint -$digest > cert-ss.pem.digest-$digest
  openssl x509 -in cert.pem -noout -fingerprint -$digest > cert.pem.digest-$digest
  openssl x509 -in dsa-cert-ss.pem -noout -fingerprint -$digest > dsa-cert-ss.pem.digest-$digest
  openssl x509 -in ec-cert-ss.pem -noout -fingerprint -$digest > ec-cert-ss.pem.digest-$digest
done

#--- Subjet Alternative Name extension ----------------------------------------------------
echo -e "\n generating self signed root cert. with Subject Alternative Name extension (X509v3) ..."
outname=cert-ss-san.pem
openssl req -out req-san.pem -new -key rsa-pri-1024.pem -subj "/CN=Johnny GuitarC=NO"
openssl req -x509 -in req-san.pem -out $outname -key rsa-pri-1024.pem \
    -config san.cnf -extensions subj_alt_name
/bin/cp san.cnf $outname.san

#--- Non-ASCII Subject ---------------------------------------------------------------------
echo -e "\n generating self signed root cert. with Subject containing UTF-8 characters ..."
outname=cert-ss-san-utf8.pem
#subject="/O=HĕĂƲÿ ʍếʈặḻ Récördŝ/OU=㈧A㉁ｫBC/CN=Johnny Guitar/C=NO"
subject=$'/O=H\xc4\x95\xc4\x82\xc6\xb2\xc3\xbf \xca\x8d\xe1\xba\xbf\xca\x88\xe1\xba\xb7\xe1\xb8\xbb R\xc3\xa9c\xc3\xb6rd\xc5\x9d/OU=\xe3\x88\xa7A\xe3\x89\x81\xef\xbd\xabBC/CN=Johnny Guitar/C=NO'
openssl req -out req-san.pem -new -key rsa-pri-1024.pem -utf8 -subj "$subject"
openssl req -x509 -in req-san.pem -out $outname -key rsa-pri-1024.pem \
    -config san.cnf -extensions subj_alt_name -nameopt multiline,utf8,-esc_msb
/bin/cp san.cnf $outname.san

echo -e "\n cleaning up ..."
/bin/rm rsa-pri-1024.pem rsa-pub-1024.*
/bin/rm dsa-pri-1024.pem dsa-pub-1024.*
/bin/rm ec-pri-384.pem ec-pub-384.*
/bin/rm req*.pem
