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

#ifndef QXCBMIME_H
#define QXCBMIME_H

#include <QtGui/private/qinternalmimedata_p.h>

#include <QtGui/QClipboard>

#include "qxcbintegration.h"
#include "qxcbconnection.h"

QT_BEGIN_NAMESPACE

class QXcbMime : public QInternalMimeData {
    Q_OBJECT
public:
    QXcbMime();
    ~QXcbMime();

    static QVector<xcb_atom_t> mimeAtomsForFormat(QXcbConnection *connection, const QString &format);
    static QString mimeAtomToString(QXcbConnection *connection, xcb_atom_t a);
    static bool mimeDataForAtom(QXcbConnection *connection, xcb_atom_t a, QMimeData *mimeData, QByteArray *data,
                                xcb_atom_t *atomFormat, int *dataFormat);
    static QVariant mimeConvertToFormat(QXcbConnection *connection, xcb_atom_t a, const QByteArray &data, const QString &format,
                                        QMetaType::Type requestedType, const QByteArray &encoding);
    static xcb_atom_t mimeAtomForFormat(QXcbConnection *connection, const QString &format, QMetaType::Type requestedType,
                                        const QVector<xcb_atom_t> &atoms, QByteArray *requestedEncoding);
};

QT_END_NAMESPACE

#endif // QXCBMIME_H
