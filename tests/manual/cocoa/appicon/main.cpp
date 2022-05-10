// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>

class TopWidget : public QWidget
{
    Q_OBJECT
public:
    TopWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout;
        QPushButton *button = new QPushButton("Change app icon");
        connect(button, SIGNAL(clicked()), this, SLOT(changeIcon()));
        layout->addWidget(button);
        setLayout(layout);
    }
public slots:
    void changeIcon()
    {
        QPixmap pix(32, 32);
        pix.fill(Qt::red);
        QIcon i(pix);
        qApp->setWindowIcon(i);
    }
};

#include "main.moc"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    TopWidget w;
    w.show();
    return a.exec();
}
