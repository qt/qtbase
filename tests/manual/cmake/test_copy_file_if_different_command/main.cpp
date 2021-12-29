/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <fstream>
#include <iostream>

int checkFileLastByte(const std::string &filename, std::fstream::off_type offset)
{
    std::ifstream file(filename, std::ios_base::in);
    if (!file.is_open()) {
        std::cerr << "Unable to open test data file: " << filename << std::endl;
        return 1;
    }
    file.seekg(offset, std::ios_base::beg);
    char data = 0;
    file.read(&data, sizeof(data));
    if (data != '2') { // We always expect it's the second copy of the file
        std::cerr << "Invalid data inside the file: " << filename << std::endl;
        return 2;
    }

    return 0;
};

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Test requires exact 2 arguments that point to the test data" << std::endl;
        return 1;
    }

    int result = checkFileLastByte(argv[1], 1024);
    if (result != 0)
        return result;
    return checkFileLastByte(argv[2], 2147483648);
}
