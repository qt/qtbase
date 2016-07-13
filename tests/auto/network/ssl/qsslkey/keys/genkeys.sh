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

# This script generates cryptographic keys of different types.

#--- RSA ---------------------------------------------------------------------------
# Note: RSA doesn't require the key size to be divisible by any particular number
for size in 40 511 512 999 1023 1024 2048
do
  echo -e "\ngenerating RSA private key to PEM file ..."
  openssl genrsa -out rsa-pri-$size.pem $size

  echo -e "\ngenerating RSA private key to DER file ..."
  openssl rsa -in rsa-pri-$size.pem -out rsa-pri-$size.der -outform DER

  echo -e "\ngenerating RSA public key to PEM file ..."
  openssl rsa -in rsa-pri-$size.pem -pubout -out rsa-pub-$size.pem

  echo -e "\ngenerating RSA public key to DER file ..."
  openssl rsa -in rsa-pri-$size.pem -pubout -out rsa-pub-$size.der -outform DER
done

#--- DSA ----------------------------------------------------------------------------
# Note: DSA requires the key size to be in interval [512, 1024] and be divisible by 64
for size in 512 576 960 1024
do
  echo -e "\ngenerating DSA parameters to PEM file ..."
  openssl dsaparam -out dsapar-$size.pem $size

  echo -e "\ngenerating DSA private key to PEM file ..."
  openssl gendsa dsapar-$size.pem -out dsa-pri-$size.pem

  /bin/rm dsapar-$size.pem

  echo -e "\ngenerating DSA private key to DER file ..."
  openssl dsa -in dsa-pri-$size.pem -out dsa-pri-$size.der -outform DER

  echo -e "\ngenerating DSA public key to PEM file ..."
  openssl dsa -in dsa-pri-$size.pem -pubout -out dsa-pub-$size.pem

  echo -e "\ngenerating DSA public key to DER file ..."
  openssl dsa -in dsa-pri-$size.pem -pubout -out dsa-pub-$size.der -outform DER
done

#--- EC ----------------------------------------------------------------------------
# Note: EC will be generated with pre-defined curves. You can check supported curves
#       with openssl ecparam -list_curves.
for curve in secp224r1 prime256v1 secp384r1 brainpoolP256r1 brainpoolP384r1 brainpoolP512r1
do
  size=`tr -cd 0-9 <<< $curve`
  size=${size::-1} # remove last number of curve name as we need bit size only
  echo -e "\ngenerating EC private key to PEM file ..."
  openssl ecparam -name $curve -genkey -noout -out ec-pri-$size-$curve.pem

  echo -e "\ngenerating EC private key to DER file ..."
  openssl ec -in ec-pri-$size-$curve.pem -out ec-pri-$size-$curve.der -outform DER

  echo -e "\ngenerating EC public key to PEM file ..."
  openssl ec -in ec-pri-$size-$curve.pem -pubout -out ec-pub-$size-$curve.pem

  echo -e "\ngenerating EC public key to DER file ..."
  openssl ec -in ec-pri-$size-$curve.pem -pubout -out ec-pub-$size-$curve.der -outform DER
done

#--- DH ----------------------------------------------------------------------------
for size in 512 1024 2048
do
  echo -e "\ngenerating DH parameters to PEM file ..."
  openssl dhparam -out dhpar-$size.pem $size

  echo -e "\ngenerating DH private key to PEM file ..."
  openssl genpkey -paramfile dhpar-$size.pem -out dh-pri-$size.pem

  /bin/rm dhpar-$size.pem

  echo -e "\ngenerating DH private key to DER file ..."
  openssl pkey -in dh-pri-$size.pem -out dh-pri-$size.der -outform DER

  echo -e "\ngenerating DH public key to PEM file ..."
  openssl pkey -in dh-pri-$size.pem -pubout -out dh-pub-$size.pem

  echo -e "\ngenerating DH public key to DER file ..."
  openssl pkey -in dh-pri-$size.pem -pubout -out dh-pub-$size.der -outform DER
done

#--- PKCS#8 ------------------------------------------------------------------------
# Note: We'll just grab some of the keys generated earlier and convert those
# https://www.openssl.org/docs/manmaster/man1/pkcs8.html#PKCS-5-v1.5-and-PKCS-12-algorithms
echo -e "\ngenerating unencrypted PKCS#8-format RSA PEM file ..."
openssl pkcs8 -topk8 -nocrypt -in rsa-pri-512.pem -out rsa-pri-512-pkcs8.pem
echo -e "\ngenerating unencrypted PKCS#8-format RSA DER file ..."
openssl pkcs8 -topk8 -nocrypt -in rsa-pri-512.pem -outform DER -out rsa-pri-512-pkcs8.der

echo -e "\ngenerating unencrypted PKCS#8-format DSA PEM file ..."
openssl pkcs8 -topk8 -nocrypt -in dsa-pri-512.pem -out dsa-pri-512-pkcs8.pem
echo -e "\ngenerating unencrypted PKCS#8-format DSA DER file ..."
openssl pkcs8 -topk8 -nocrypt -in dsa-pri-512.pem -outform DER -out dsa-pri-512-pkcs8.der

echo -e "\ngenerating unencrypted PKCS#8-format EC PEM file ..."
openssl pkcs8 -topk8 -nocrypt -in ec-pri-224-secp224r1.pem -out ec-pri-224-secp224r1-pkcs8.pem
echo -e "\ngenerating unencrypted PKCS#8-format EC DER file ..."
openssl pkcs8 -topk8 -nocrypt -in ec-pri-224-secp224r1.pem -outform DER -out ec-pri-224-secp224r1-pkcs8.der

for pkey in rsa-pri-512 dsa-pri-512 ec-pri-224-secp224r1
do
  pkeystem=`echo "$pkey" | cut -d- -f 1`
  # List: https://www.openssl.org/docs/manmaster/man1/pkcs8.html#PKCS-5-v1.5-and-PKCS-12-algorithms
  # These are technically supported, but fail to generate. Probably because MD2 is deprecated/removed
  # PBE-MD2-DES PBE-MD2-RC2-64
  for algorithm in PBE-MD5-DES PBE-SHA1-RC2-64 PBE-MD5-RC2-64 PBE-SHA1-DES
  do
    echo -e "\ngenerating encrypted PKCS#8-format (v1) PEM-encoded $pkeystem key using $algorithm ..."
    openssl pkcs8 -topk8 -in $pkey.pem -v1 $algorithm -out $pkey-pkcs8-$algorithm.pem -passout pass:1234

    echo -e "\ngenerating encrypted PKCS#8-format (v1) DER-encoded $pkeystem key using $algorithm ..."
    openssl pkcs8 -topk8 -in $pkey.pem -v1 $algorithm -outform DER -out $pkey-pkcs8-$algorithm.der -passout pass:1234
  done

  for algorithm in PBE-SHA1-RC4-128 PBE-SHA1-RC4-40 PBE-SHA1-3DES PBE-SHA1-2DES PBE-SHA1-RC2-128 PBE-SHA1-RC2-40
  do
    echo -e "\ngenerating encrypted PKCS#8-format (v1 PKCS#12) PEM-encoded $pkeystem key using $algorithm ..."
    openssl pkcs8 -topk8 -in $pkey.pem -v1 $algorithm -out $pkey-pkcs8-pkcs12-$algorithm.pem -passout pass:1234

    echo -e "\ngenerating encrypted PKCS#8-format (v1 PKCS#12) DER-encoded $pkeystem key using $algorithm ..."
    openssl pkcs8 -topk8 -in $pkey.pem -v1 $algorithm -outform DER -out $pkey-pkcs8-pkcs12-$algorithm.der -passout pass:1234
  done

  for algorithm in des3 aes128 aes256 rc2
  do
    for prf in hmacWithSHA1 hmacWithSHA256
    do
      echo -e "\ngenerating encrypted PKCS#8-format (v2) PEM-encoded $pkeystem key using $algorithm and $prf ..."
      openssl pkcs8 -topk8 -in $pkey.pem -v2 $algorithm -v2prf $prf -out $pkey-pkcs8-$algorithm-$prf.pem -passout pass:1234

      echo -e "\ngenerating encrypted PKCS#8-format (v2) DER-encoded $pkeystem key using $algorithm and $prf ..."
      openssl pkcs8 -topk8 -in $pkey.pem -v2 $algorithm -v2prf $prf -outform DER -out $pkey-pkcs8-$algorithm-$prf.der -passout pass:1234
    done
  done
done
