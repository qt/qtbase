// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSINTERNALMIME_H
#define QWINDOWSINTERNALMIME_H

#include <QtCore/qt_windows.h>

#include <QtGui/private/qinternalmimedata_p.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QDebug;

// Implementation in qwindowsclipboard.cpp.
class QWindowsInternalMimeData : public QInternalMimeData {
public:
    bool hasFormat_sys(const QString &mimetype) const override;
    QStringList formats_sys() const override;
    QVariant retrieveData_sys(const QString &mimetype, QMetaType preferredType) const override;

protected:
    virtual IDataObject *retrieveDataObject() const = 0;
    virtual void releaseDataObject(IDataObject *) const {}
};

QT_END_NAMESPACE

#endif // QWINDOWSINTERNALMIME_H
