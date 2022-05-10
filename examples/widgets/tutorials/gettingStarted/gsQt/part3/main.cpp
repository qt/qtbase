// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

class Notepad : public QWidget
{
    Q_OBJECT

public:
    Notepad();

private slots:
    void quit();

private:
    QTextEdit *textEdit;
    QPushButton *quitButton;

};

Notepad::Notepad()
{
    textEdit = new QTextEdit;
    quitButton = new QPushButton(tr("Quit"));

    connect(quitButton, &QPushButton::clicked,
            this, &Notepad::quit);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textEdit);
    layout->addWidget(quitButton);

    setLayout(layout);

    setWindowTitle(tr("Notepad"));
}

void Notepad::quit()
{
    QMessageBox messageBox;
    messageBox.setWindowTitle(tr("Notepad"));
    messageBox.setText(tr("Do you really want to quit?"));
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::No);
    if (messageBox.exec() == QMessageBox::Yes)
        qApp->quit();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Notepad notepad;
    notepad.show();

    return app.exec();
}

#include "main.moc"

