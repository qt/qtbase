# Copyright (C) 2019 Intel Corporation.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
