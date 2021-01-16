/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

#include <iostream>

#include <qdebug.h>
#include <qjsondocument.h>

using namespace std;

int main()
{
    QByteArray json;
    while (!cin.eof()) {
        char arr[1024];
        cin.read(arr, sizeof(arr));
        json.append(arr, cin.gcount());
    }

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (document.isNull()) {
        qDebug() << "error:" << qPrintable(error.errorString()) << "at offset" << error.offset;
        return 1;
    }

    QByteArray binaryJson = document.toBinaryData();
    cout.write(binaryJson.constData(), binaryJson.size());
}
