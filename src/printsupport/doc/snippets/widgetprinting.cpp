// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
        const auto pageLayout = printer.pageLayout();
        const auto pageRect = pageLayout.paintRectPixels(printer.resolution());
        const auto paperRect = pageLayout.fullRectPixels(printer.resolution());
        double xscale = pageRect.width() / double(myWidget->width());
        double yscale = pageRect.height() / double(myWidget->height());
        double scale = qMin(xscale, yscale);
        painter.translate(pageRect.x() + paperRect.width() / 2.,
                          pageRect.y() + paperRect.height() / 2.);
        painter.scale(scale, scale);
        painter.translate(-myWidget->width() / 2., -myWidget->height() / 2.);

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
        if (dialog.exec() != QDialog::Accepted)
            return;
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
