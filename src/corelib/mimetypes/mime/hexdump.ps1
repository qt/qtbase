#############################################################################
##
## Copyright (C) 2019 Intel Corporation.
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

param([String]$path, [String]$orig)

"static const unsigned char mimetype_database[] = {"
ForEach ($byte in Get-Content -Encoding byte -ReadCount 16 -path $path) {
#    if (($byte -eq 0).count -ne 16) {
        $hex = $byte | Foreach-Object {
            " 0x" + ("{0:x}" -f $_).PadLeft( 2, "0" ) + ","
        }
        "    $hex"
#    }
}
"};"

$file = Get-Childitem -file $orig
"static constexpr size_t MimeTypeDatabaseOriginalSize = " + $file.length + ";"
