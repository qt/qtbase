// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
Q_PROPERTY(QColor color READ color WRITE setColor USER true)
//! [0]


//! [1]
QItemEditorCreator<MyEditor> *itemCreator =
    new QItemEditorCreator<MyEditor>("myProperty");

QItemEditorFactory *factory = new QItemEditorFactory;
//! [1]


//! [setDefaultFactory]
//! [2]
QItemEditorFactory *editorFactory = new QItemEditorFactory;
QItemEditorCreatorBase *creator = new QStandardItemEditorCreator<MyFancyDateTimeEdit>();
editorFactory->registerEditor(QMetaType::QDateTime, creator);
//! [2]

QItemEditorFactory::setDefaultFactory(editorFactory);
//! [setDefaultFactory]


//! [3]
Q_PROPERTY(QColor color READ color WRITE setColor USER true)
//! [3]
