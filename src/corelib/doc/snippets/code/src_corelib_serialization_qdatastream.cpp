// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    QDataStream &operator<<(QDataStream &, const QXxx &);
    QDataStream &operator>>(QDataStream &, QXxx &);
//! [0]

//! [1]
    QDataStream & operator<< (QDataStream& stream, const QImage& image);
    QDataStream & operator>> (QDataStream& stream, QImage& image);
//! [1]
