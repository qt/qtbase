// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

class Notepad : public QMainWindow
{
    Q_OBJECT

public:
    Notepad();

private slots:
    void load();
    void save();

private:
    QTextEdit *textEdit;

    QAction *loadAction;
    QAction *saveAction;
    QAction *exitAction;

    QMenu *fileMenu;
};

Notepad::Notepad()
{
    loadAction = new QAction(tr("&Load"), this);
    saveAction = new QAction(tr("&Save"), this);
    exitAction = new QAction(tr("E&xit"), this);

    connect(loadAction, &QAction::triggered,
            this, &Notepad::load);
    connect(saveAction, &QAction::triggered,
            this, &Notepad::save);
    connect(exitAction, &QAction::triggered,
            qApp, &QApplication::quit);

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(loadAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    textEdit = new QTextEdit;
    setCentralWidget(textEdit);

    setWindowTitle(tr("Notepad"));
}

void Notepad::load()
{

}

void Notepad::save()
{

}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Notepad notepad;
    notepad.show();

    return app.exec();
};

#include "main.moc"

