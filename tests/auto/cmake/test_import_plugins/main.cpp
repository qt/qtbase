/****************************************************************************
**
** Copyright (C) 2018 Kitware, Inc.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QPluginLoader>
#include <QSet>
#include <QString>
#include <QVector>

#include <cstddef>
#include <iostream>

extern QString expectedPlugins[];
extern std::size_t numExpectedPlugins;

int main(int argc, char **argv)
{
#ifdef QT_STATIC
    QSet<QString> expectedPluginSet;
    for (std::size_t i = 0; i < numExpectedPlugins; i++) {
        expectedPluginSet.insert(expectedPlugins[i]);
    }

    QVector<QStaticPlugin> plugins = QPluginLoader::staticPlugins();
    QSet<QString> actualPluginSet;
    for (QStaticPlugin plugin : plugins) {
        actualPluginSet.insert(plugin.metaData()["className"].toString());
    }

    if (expectedPluginSet != actualPluginSet) {
        std::cerr << "Loaded plugins do not match what was expected!" << std::endl
                  << "Expected plugins:" << std::endl;

        QList<QString> expectedPluginList = expectedPluginSet.toList();
        expectedPluginList.sort();
        for (QString plugin : expectedPluginList) {
            std::cerr << (actualPluginSet.contains(plugin) ? "  " : "- ")
                      << plugin.toStdString() << std::endl;
        }

        std::cerr << std::endl << "Actual plugins:" << std::endl;

        QList<QString> actualPluginList = actualPluginSet.toList();
        actualPluginList.sort();
        for (QString plugin : actualPluginList) {
            std::cerr << (expectedPluginSet.contains(plugin) ? "  " : "+ ")
                      << plugin.toStdString() << std::endl;
        }

        return 1;
    }

#endif
    return 0;
}
