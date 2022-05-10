// Copyright (C) 2014 by Southwest Research Institute (R)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


#include <QByteArrayList>

int main(int, char **)
{
    QByteArray ba1, ba2, ba3;
//! [0]
    QByteArrayList longerList = (QByteArrayList() << ba1 << ba2 << ba3);
//! [0]
}
