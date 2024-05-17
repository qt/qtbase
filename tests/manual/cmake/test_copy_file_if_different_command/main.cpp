// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
