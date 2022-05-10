// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QBuffer>
#include <QPalette>

static void main_snippet()
{
//! [0]
    QBuffer buffer;
    char ch;

    buffer.open(QBuffer::ReadWrite);
    buffer.write("Qt rocks!");
    buffer.seek(0);
    buffer.getChar(&ch);  // ch == 'Q'
    buffer.getChar(&ch);  // ch == 't'
    buffer.getChar(&ch);  // ch == ' '
    buffer.getChar(&ch);  // ch == 'r'
//! [0]
}

static void write_datastream_snippets()
{
//! [1]
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    QDataStream out(&buffer);
    out << QApplication::palette();
//! [1]
}

static void read_datastream_snippets()
{
    QByteArray byteArray;

//! [2]
    QPalette palette;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadOnly);

    QDataStream in(&buffer);
    in >> palette;
//! [2]
}

static void bytearray_ptr_ctor_snippet()
{
//! [3]
    QByteArray byteArray("abc");
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    buffer.seek(3);
    buffer.write("def", 3);
    buffer.close();
    // byteArray == "abcdef"
//! [3]
}

static void setBuffer_snippet()
{
//! [4]
    QByteArray byteArray("abc");
    QBuffer buffer;
    buffer.setBuffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    buffer.seek(3);
    buffer.write("def", 3);
    buffer.close();
    // byteArray == "abcdef"
//! [4]
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    main_snippet();
    bytearray_ptr_ctor_snippet();
    write_datastream_snippets();
    read_datastream_snippets();
    setBuffer_snippet();
    return 0;
}
