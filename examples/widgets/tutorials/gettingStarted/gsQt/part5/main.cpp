// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

class Notepad : public QMainWindow
{
    Q_OBJECT

public:
    Notepad();

private slots:
    void open();
    void save();

private:
    QTextEdit *textEdit;

    QAction *openAction;
    QAction *saveAction;
    QAction *exitAction;

    QMenu *fileMenu;
};

Notepad::Notepad()
{
    openAction = new QAction(tr("&Load"), this);
    saveAction = new QAction(tr("&Save"), this);
    exitAction = new QAction(tr("E&xit"), this);

    connect(openAction, &QAction::triggered,
            this, &Notepad::open);
    connect(saveAction, &QAction::triggered,
            this, &Notepad::save);
    connect(exitAction, &QAction::triggered,
            qApp, &QApplication::quit);

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    textEdit = new QTextEdit;
    setCentralWidget(textEdit);

    setWindowTitle(tr("Notepad"));
}

void Notepad::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
        tr("Text Files (*.txt);;C++ Files (*.cpp *.h)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Could not open file"));
            return;
        }
        QTextStream in(&file);
        textEdit->setText(in.readAll());
        file.close();
    }
}

void Notepad::save()
{

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "",
        tr("Text Files (*.txt);;C++ Files (*.cpp *.h)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            // error message
        } else {
            QTextStream stream(&file);
            stream << textEdit->toPlainText();
            stream.flush();
            file.close();
        }
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Notepad notepad;
    notepad.show();

    return app.exec();
}

#include "main.moc"

