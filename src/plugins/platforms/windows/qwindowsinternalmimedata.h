/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
    QVariant retrieveData_sys(const QString &mimetype, QVariant::Type preferredType) const override;

protected:
    virtual IDataObject *retrieveDataObject() const = 0;
    virtual void releaseDataObject(IDataObject *) const {}
};

QT_END_NAMESPACE

#endif // QWINDOWSINTERNALMIME_H
