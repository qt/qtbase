// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include <QtGui>
#include <QtDebug>
#include <QQmlComponent>

//! [1]
    void statusChanged(QQmlComponent::Status status) {
        if (status == QQmlComponent::Error) {
            for (const QQmlError &error: std::as_const(component->errors())) {
                const QByteArray file = error.url().toEncoded();
                QMessageLogger(file.constData(), error.line(), 0).debug() << error.description();
            }
        }
    }
//! [1]

//! [2]
    const QLoggingCategory &category();
//! [2]
