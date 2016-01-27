#!/bin/bash

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

booted_simulator=$(xcrun simctl list devices | grep -E "iPhone|iPad" | grep -v unavailable | grep Booted | perl -lne 'print $1 if /\((.*?)\)/')
echo "IPHONESIMULATOR_DEVICES = $booted_simulator"

xcodebuild test -scheme $1 -destination 'id=0' -destination-timeout 1 2>&1| sed -n 's/{ \(platform:.*\) }/\1/p' | while read destination; do
    id=$(echo $destination | sed -n -E 's/.*id:([^ ,]+).*/\1/p')
    echo $destination | tr ',' '\n' | while read keyval; do
        key=$(echo $keyval | cut -d ':' -f 1 | tr '[:lower:]' '[:upper:]')
        val=$(echo $keyval | cut -d ':' -f 2)
        echo "%_$id: DESTINATION_${key} = $val"

        if [ $key = 'PLATFORM' ]; then
            if [ "$val" = "iOS" ]; then
                echo "IPHONEOS_DEVICES += $id"
            elif [ "$val" = "iOS Simulator" -a "$id" != "$booted_simulator" ]; then
                echo "IPHONESIMULATOR_DEVICES += $id"
            fi
        fi
    done
    echo
done
