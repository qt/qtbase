#!/bin/sh
#############################################################################
##
## Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
## Contact: http://www.qt-project.org/
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## GNU Lesser General Public License Usage
## This file may be used under the terms of the GNU Lesser General Public
## License version 2.1 as published by the Free Software Foundation and
## appearing in the file LICENSE.LGPL included in the packaging of this
## file. Please review the following information to ensure the GNU Lesser
## General Public License version 2.1 requirements will be met:
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Nokia gives you certain additional
## rights. These rights are described in the Nokia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU General
## Public License version 3.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of this
## file. Please review the following information to ensure the GNU General
## Public License version 3.0 requirements will be met:
## http://www.gnu.org/copyleft/gpl.html.
##
## Other Usage
## Alternatively, this file may be used in accordance with the terms and
## conditions contained in a signed written agreement between you and Nokia.
##
##
##
##
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
