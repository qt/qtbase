#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2018 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL$
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
## General Public License version 3 or (at your option) any later version
## approved by the KDE Free Qt Foundation. The licenses are as published by
## the Free Software Foundation and appearing in the file LICENSE.GPL3
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

set -ex

# package apache2

# add users
useradd httptest; echo "httptest:httptest" | chpasswd

# enable apache2 module
/usr/sbin/a2enmod ssl dav_fs headers deflate auth_digest cgi

# enable apache2 config
cp $TESTDATA/{main,security,ssl,dav}.conf /etc/apache2/conf-available/
/usr/sbin/a2enconf main security ssl dav

# install configurations and test data
cp $TESTDATA/deflate.conf /etc/apache2/mods-available/
mkdir -p -m 1777 /home/writeables/dav # dav.conf
a2dissite '*' # disable all of the default apache2 sites

# Populate the web-site:
su $USER -c "cp -r $TESTDATA/www ~/www"

# tst_QNetworkReply::getFromHttp(success-internal)
su $USER -c "cp rfc3252.txt ~/www/htdocs/"; rm rfc3252.txt

# tst_QNetworkReply::synchronousRequest_data()
su $USER -c "mkdir -p ~/www/htdocs/deflate/"
su $USER -c "ln -s ~/www/htdocs/rfc3252.txt ~/www/htdocs/deflate/"

# tst_QNetworkReply::headFromHttp(with-authentication)
su $USER -c "ln -s ~/www/htdocs/rfc3252.txt ~/www/htdocs/rfcs-auth/"

# Duplicate rfc3252.txt 20 times for bigfile tests:
su $USER -c "seq 20 | xargs -i cat ~/www/htdocs/rfc3252.txt >> ~/www/htdocs/bigfile"

# tst_QNetworkReply::postToHttp(empty)
su $USER -c "ln -s ~/www/htdocs/protected/cgi-bin/md5sum.cgi ~/www/cgi-bin/"

# tst_QNetworkReply::lastModifiedHeaderForHttp() expects this time-stamp:
touch -d "2007-05-22 12:04:57 GMT" /home/$USER/www/htdocs/fluke.gif

# Create 10MB file for use by tst_Q*::downloadBigFile and interruption tests:
su $USER -c "/bin/dd if=/dev/zero of=~/www/htdocs/mediumfile bs=1 count=0 seek=10000000"

# Emulate test server's hierarchy:
su $USER -c "ln -s ~/www/htdocs/rfcs/rfc2616.html ~/www/htdocs/deflate/"

# enable service with installed configurations
service apache2 restart
