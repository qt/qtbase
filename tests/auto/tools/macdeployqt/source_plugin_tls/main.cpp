// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qsslsocket.h>

#include <QtCore/qcoreapplication.h>

int main(int argc, char ** argv)
{
   QCoreApplication app(argc, argv);
   return QSslSocket::supportsSsl() ? 0 : 1;
}
