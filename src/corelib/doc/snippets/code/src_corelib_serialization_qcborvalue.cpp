/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
    QCborValue(uuid) == QCborValue(QCborKnownTags::Uuid, uuid.toRfc4122());
//! [0]

//! [1]
    QCborValue value(QCborSimpleType(12));
//! [1]

//! [2]
    value.isSimpleType(QCborSimpleType(12));
//! [2]

//! [3]
    QCborValue(QUrl("https://example.com")) == QCborValue(QCborKnownTags::Url, "https://example.com");
//! [3]

//! [4]
    value.toMap().value(key);
//! [4]

//! [5]
    value.toMap().value(key);
//! [5]

//! [6]
    if (reader.isTag() && reader.toTag() == QCborKnownTags::Signature)
        reader.next();

    QCborValue contents = QCborValue::fromCbor(reader);
//! [6]
