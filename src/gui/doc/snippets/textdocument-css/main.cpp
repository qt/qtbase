// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QTextBrowser>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

//! [0]
    QTextBrowser browser;
    QColor linkColor(Qt::red);
    QString sheet = QString::fromLatin1("a { text-decoration: underline; color: %1 }").arg(linkColor.name());
    browser.document()->setDefaultStyleSheet(sheet);
//! [0]
    browser.setSource(QUrl("../../../html/index.html"));
    browser.resize(800, 600);
    browser.show();

    return app.exec();
}

