/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrinter>
#include <QPrintDialog>
#endif

class Window : public QWidget
{
    Q_OBJECT

public:
    Window()
    {
        myWidget = new QPushButton("Print Me");
        connect(myWidget, &QPushButton::clicked, this, &Window::print);
        myWidget2 = new QPushButton("Print Document");
        connect(myWidget2, &QPushButton::clicked, this, &Window::printFile);
        editor = new QTextEdit(this);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(myWidget);
        layout->addWidget(myWidget2);
        layout->addWidget(editor);
        setLayout(layout);
    }

private slots:
    void print()
    {
    #if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printer)
        QPrinter printer(QPrinter::HighResolution);

        printer.setOutputFileName("test.pdf");

//! [0]
        QPainter painter;
        painter.begin(&printer);
        double xscale = printer.pageRect().width() / double(myWidget->width());
        double yscale = printer.pageRect().height() / double(myWidget->height());
        double scale = qMin(xscale, yscale);
        painter.translate(printer.paperRect().x() + printer.pageRect().width()/2,
                           printer.paperRect().y() + printer.pageRect().height()/2);
        painter.scale(scale, scale);
        painter.translate(-width()/2, -height()/2);

        myWidget->render(&painter);
//! [0]
    #endif
    }

    void printFile()
    {
    #if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
//! [1]
        QPrinter printer;

        QPrintDialog dialog(&printer, this);
        dialog.setWindowTitle(tr("Print Document"));
        if (editor->textCursor().hasSelection())
            dialog.addEnabledOption(QAbstractPrintDialog::PrintSelection);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
//! [1]
        editor->print(&printer);
    #endif
    }

private:
    QPushButton *myWidget;
    QPushButton *myWidget2;
    QTextEdit   *editor;
};

#include "main.moc"

int main(int argv, char **args)
{
    QApplication app(argv, args);

    Window window;
    window.show();

    return app.exec();
}
