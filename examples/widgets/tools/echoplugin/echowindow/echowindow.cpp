// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "echowindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPluginLoader>
#include <QPushButton>

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
    QDir pluginsDir(QCoreApplication::applicationDirPath());
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
    const QStringList entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            echoInterface = qobject_cast<EchoInterface *>(plugin);
            if (echoInterface)
                return true;
            pluginLoader.unload();
        }
    }

    return false;
}
//! [3]
