// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
namespace Foo
{
    struct MyInterface { ... };
}

Q_DECLARE_INTERFACE(Foo::MyInterface, "org.examples.MyInterface")
//! [0]


//! [1]
class MyInstance : public QObject
{
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDummyPlugin" FILE "mymetadata.json")
};
//! [1]


//! [2]
Q_IMPORT_PLUGIN(qjpeg)
//! [2]
