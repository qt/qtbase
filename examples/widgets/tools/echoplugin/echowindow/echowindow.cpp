/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include <QtWidgets>

#include "echowindow.h"

//! [0]
EchoWindow::EchoWindow()
{
    createGUI();
    setLayout(layout);
    setWindowTitle("Echo Plugin Example");

    if (!loadPlugin()) {
        QMessageBox::information(this, "Error", "Could not load the plugin");
        lineEdit->setEnabled(false);
        button->setEnabled(false);
    }
}
//! [0]

//! [1]
void EchoWindow::sendEcho()
{
    QString text = echoInterface->echo(lineEdit->text());
    label->setText(text);
}
//! [1]

//! [2]
void EchoWindow::createGUI()
{
    lineEdit = new QLineEdit;
    label = new QLabel;
    label->setFrameStyle(QFrame::Box | QFrame::Plain);
    button = new QPushButton(tr("Send Message"));

    connect(lineEdit, &QLineEdit::editingFinished,
            this, &EchoWindow::sendEcho);
    connect(button, &QPushButton::clicked,
            this, &EchoWindow::sendEcho);

    layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Message:")), 0, 0);
    layout->addWidget(lineEdit, 0, 1);
    layout->addWidget(new QLabel(tr("Answer:")), 1, 0);
    layout->addWidget(label, 1, 1);
    layout->addWidget(button, 2, 1, Qt::AlignRight);
    layout->setSizeConstraint(QLayout::SetFixedSize);
}
//! [2]

//! [3]
bool EchoWindow::loadPlugin()
{
    QDir pluginsDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd("plugins");
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            echoInterface = qobject_cast<EchoInterface *>(plugin);
            if (echoInterface)
                return true;
        }
    }

    return false;
}
//! [3]
