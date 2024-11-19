// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PLUGINLOG_H
#define PLUGINLOG_H

#include <QCoreApplication>
#include <QVariant>

class PluginLog {
public:
    static QStringList get() { return qApp->property(testProp).toStringList(); }
    static void set(const QStringList &log) { qApp->setProperty(testProp, log); }
    static void clear() { set({}); }
    static void append(const QString &msg) { set(get() << msg); }
    static int size() { return get().size(); }
    static QString item(int index) { return get().value(index); }
private:
    static constexpr auto testProp = "_q_testimageplugin";
};

#endif // PLUGINLOG_H
