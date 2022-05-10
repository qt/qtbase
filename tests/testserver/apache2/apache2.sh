#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
