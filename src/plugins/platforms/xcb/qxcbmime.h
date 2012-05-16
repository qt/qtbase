/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBMIME_H
#define QXCBMIME_H

#include <private/qdnd_p.h>

#include <QtGui/QClipboard>

#include "qxcbintegration.h"
#include "qxcbconnection.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DRAGANDDROP

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
                                        QVariant::Type requestedType, const QByteArray &encoding);
    static xcb_atom_t mimeAtomForFormat(QXcbConnection *connection, const QString &format, QVariant::Type requestedType,
                                        const QList<xcb_atom_t> &atoms, QByteArray *requestedEncoding);
};

#endif // QT_NO_DRAGANDDROP

QT_END_NAMESPACE

#endif // QXCBMIME_H
