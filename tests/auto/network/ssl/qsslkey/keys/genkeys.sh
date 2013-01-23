#!/bin/sh
#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
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
