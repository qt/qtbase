// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

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

    static QList<xcb_atom_t> mimeAtomsForFormat(QXcbConnection *connection, const QString &format);
    static QString mimeAtomToString(QXcbConnection *connection, xcb_atom_t a);
    static bool mimeDataForAtom(QXcbConnection *connection, xcb_atom_t a, QMimeData *mimeData, QByteArray *data,
                                xcb_atom_t *atomFormat, int *dataFormat);
    static QVariant mimeConvertToFormat(QXcbConnection *connection, xcb_atom_t a, const QByteArray &data, const QString &format,
                                        QMetaType requestedType, bool hasUtf8);
    static xcb_atom_t mimeAtomForFormat(QXcbConnection *connection, const QString &format, QMetaType requestedType,
                                        const QList<xcb_atom_t> &atoms, bool *hasUtf8);
};

QT_END_NAMESPACE
