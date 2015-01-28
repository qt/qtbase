#!/bin/sh
#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing/
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL21$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see http://www.qt.io/terms-conditions. For further
## information use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file. Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## As a special exception, The Qt Company gives you certain additional
## rights. These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#       If OpenSSL 1.0.2 is available brainpool should be added!
# brainpoolP256r1 brainpoolP384r1 brainpoolP512r1
for curve in secp224r1 prime256v1 secp384r1
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
