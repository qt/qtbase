// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QtDebug>
#include <QDeclarativeComponent>

//! [1]
    void statusChanged(QDeclarativeComponent::Status status) {
        if (status == QDeclarativeComponent::Error) {
            foreach (const QDeclarativeError &error, component->errors()) {
                const QByteArray file = error.url().toEncoded();
                QMessageLogger(file.constData(), error.line(), 0).debug() << error.description();
            }
        }
    }
//! [1]

//! [2]
    const QLoggingCategory &category();
//! [2]
