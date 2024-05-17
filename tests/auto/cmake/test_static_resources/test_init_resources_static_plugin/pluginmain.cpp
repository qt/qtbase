// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class TestStaticPlugin : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "TestStaticPlugin" URI "qt.teststaticplugin")
public:
    TestStaticPlugin() = default;
    Q_INVOKABLE bool checkResources()
    {
        return QFile::exists(":/teststaticplugin1/testfile1.txt")
                && QFile::exists(":/teststaticplugin2/testfile2.txt");
    }
};

QT_END_NAMESPACE

#include "pluginmain.moc"
