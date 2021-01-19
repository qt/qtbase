/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QFILEDEVICE_P_H
#define QFILEDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qiodevice_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

class QAbstractFileEngine;
class QFSFileEngine;

class QFileDevicePrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QFileDevice)
protected:
    QFileDevicePrivate();
    ~QFileDevicePrivate();

    virtual QAbstractFileEngine *engine() const;

    inline bool ensureFlushed() const;

    bool putCharHelper(char c) override;

    void setError(QFileDevice::FileError err);
    void setError(QFileDevice::FileError err, const QString &errorString);
    void setError(QFileDevice::FileError err, int errNum);

    mutable std::unique_ptr<QAbstractFileEngine> fileEngine;
    mutable qint64 cachedSize;

    QFileDevice::FileHandleFlags handleFlags;
    QFileDevice::FileError error;

    bool lastWasWrite;
};

inline bool QFileDevicePrivate::ensureFlushed() const
{
    // This function ensures that the write buffer has been flushed (const
    // because certain const functions need to call it.
    if (lastWasWrite) {
        const_cast<QFileDevicePrivate *>(this)->lastWasWrite = false;
        if (!const_cast<QFileDevice *>(q_func())->flush())
            return false;
    }
    return true;
}

QT_END_NAMESPACE

#endif // QFILEDEVICE_P_H
