/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include <QtCore/qglobal.h>

#ifndef QSYSINFO_H
#define QSYSINFO_H

QT_BEGIN_NAMESPACE

/*
   System information
*/

class QString;
class Q_CORE_EXPORT QSysInfo
{
public:
    enum Sizes {
        WordSize = (sizeof(void *)<<3)
    };

    enum Endian {
        BigEndian,
        LittleEndian
#  ifdef Q_QDOC
        , ByteOrder = BigEndian or LittleEndian
#  elif Q_BYTE_ORDER == Q_BIG_ENDIAN
        , ByteOrder = BigEndian
#  elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        , ByteOrder = LittleEndian
#  else
#    error "Undefined byte order"
#  endif
    };

    static QString buildCpuArchitecture();
    static QString currentCpuArchitecture();
    static QString buildAbi();

    static QString kernelType();
    static QString kernelVersion();
    static QString productType();
    static QString productVersion();
    static QString prettyProductName();

    static QString machineHostName();
    static QByteArray machineUniqueId();
    static QByteArray bootUniqueId();
};

QT_END_NAMESPACE
#endif // QSYSINFO_H
