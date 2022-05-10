// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [4]
QResource::registerResource("/path/to/myresource.rcc");
//! [4]


//! [5]
MyClass::MyClass() : BaseClass()
{
    Q_INIT_RESOURCE(resources);

    QFile file(":/myfile.dat");
    ...
}
//! [5]


//! [6]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(graphlib);

    QFile file(":/graph.png");
    ...
    return app.exec();
}
//! [6]
