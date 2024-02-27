#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

set -ex

# package vsftpd

# add users
usermod -d "/home/$USER/ftp/" ftp #existing user
useradd -d "/home/$USER/ftp" -s /bin/bash ftptest; echo "ftptest:$PASS" | chpasswd

# install configurations and test data
cp $TESTDATA/vsftpd.{conf,user_list} /etc/

# Resolve error message "vsftpd failed - probably invalid config" during boot
# This bug has been reported to Debian bug tracking system (ID #911396)
command='ps -C vsftpd | grep -qs "${_PID}"'
sed -i -e 's,while [ ${n} -le 5 ].*$,while true,' \
    -e "s,\t\t\tif ! $command.*$,\t\t\tif $command," /etc/init.d/vsftpd

# Populate the FTP sites:
su $USER -c "cp -r $TESTDATA/ftp ~/ftp"
ln -s /home/$USER/ftp /var/ftp

# tst_QNetworkReply::getFromFtp_data()
su $USER -c "mkdir -p ~/ftp/qtest/"
su $USER -c "cp rfc3252.txt ~/ftp/qtest/"; rm rfc3252.txt

# tst_QNetworkReply::proxy_data()
su $USER -c "ln ~/ftp/qtest/rfc3252.txt ~/ftp/qtest/rfc3252"
su $USER -c "mkdir -p ~/ftp/qtest/nonASCII/"

# Duplicate rfc3252.txt 20 times for bigfile tests:
su $USER -c "seq 20 | xargs -i cat ~/ftp/qtest/rfc3252.txt >> ~/ftp/qtest/bigfile"

# tst_QNetworkReply::getErrors_data(), testdata with special permissions
su $USER -c "chmod 0600 ~/ftp/pub/file-not-readable.txt"

# Shared FTP folder (sticky bit)
su $USER -c "mkdir -p -m 1777 ~/ftp/qtest/upload" # FTP incoming dir

# enable service with installed configurations
service vsftpd restart
