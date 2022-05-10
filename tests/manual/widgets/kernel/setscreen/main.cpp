// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtWidgets>

class ScreenWidget : public QWidget
{
public:
    ScreenWidget(QWidget *parent)
    : QWidget(parent, Qt::Window)
    {
        textEdit = new QTextEdit;
        textEdit->setReadOnly(true);

        QHBoxLayout *layout = new QHBoxLayout;
        layout->addWidget(textEdit);
        setLayout(layout);
    }

    void updateText()
    {
        QString text = "<html><body>";
        text += QString("<p>Screen: %1\n</p>").arg(screen()->name());
        text += QString("<p>DPR: %1\n</p>").arg(screen()->devicePixelRatio());
        text += QString("</body></html>");

        textEdit->setText(text);
    }

private:
    QTextEdit *textEdit;
};

class Controller : public QDialog
{
    Q_OBJECT

public:
    Controller()
    {
        QPushButton *screenButton = new QPushButton;
        screenButton->setText("Show on Screen");
        screenButton->setEnabled(false);
        connect(screenButton, &QAbstractButton::clicked, this, &Controller::setScreen);

        QPushButton *exitButton = new QPushButton;
        exitButton->setText("E&xit");
        connect(exitButton, &QAbstractButton::clicked, QApplication::instance(), &QCoreApplication::quit);

        QHBoxLayout *actionLayout = new QHBoxLayout;
        actionLayout->addWidget(screenButton);
        actionLayout->addWidget(exitButton);

        QGroupBox *radioGroup = new QGroupBox;
        radioGroup->setTitle(QLatin1String("Select target screen"));

        QVBoxLayout *groupLayout = new QVBoxLayout;
        const auto screens = QGuiApplication::screens();
        int count = 0;
        for (const auto &screen : screens) {
            QRadioButton *choice = new QRadioButton;
            choice->setText(QString(QLatin1String("%1: %2")).arg(count).arg(screen->name()));
            connect(choice, &QAbstractButton::toggled, this, [=](bool on){
                if (on)
                    targetScreen = count;
                screenButton->setEnabled(targetScreen != -1);
            });
            groupLayout->addWidget(choice);
            ++count;
        }
        radioGroup->setLayout(groupLayout);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(radioGroup);
        layout->addLayout(actionLayout);
        setLayout(layout);
    }

private slots:
    void setScreen()
    {
        QScreen *screen = QGuiApplication::screens().at(targetScreen);
        if (!widget) {
            widget = new ScreenWidget(this);
            widget->setAttribute(Qt::WA_DeleteOnClose);
            widget->setWindowTitle("Normal Window");
        }
        widget->setScreen(screen);
        widget->show();
        widget->updateText();
    }

private:
    QPointer<ScreenWidget> widget = nullptr;
    int targetScreen = -1;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Controller controller;
    controller.show();

    return app.exec();
}

#include "main.moc"
