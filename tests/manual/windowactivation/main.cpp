// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWidgets>

class RasterWindow : public QRasterWindow
{
    Q_OBJECT

public:
    RasterWindow(const QColor &color)
        : m_color(color)
    {
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.fillRect(QRect(0, 0, width(), height()), m_color);
    }

private:
    QColor m_color;
};

class ActivateButton : public QPushButton
{
    Q_OBJECT
public:
    ActivateButton(const QString &text, QWindow *window)
        : QPushButton(text)
    {
        connect(this, &QPushButton::clicked, window, [window](){
            window->requestActivate();
        });
        connect(window, &QWindow::activeChanged, this, [this, window](){
            setPalette(window->isActive() ? QPalette(Qt::red) : qApp->palette());
        });
    }
};

class Controller : public QWidget
{
    Q_OBJECT
public:
    Controller() {
        setWindowTitle("Controller window");
        auto *ed1 = new QTextEdit;
        ed1->setWindowTitle("Editor 1");
        ed1->show();
        auto *ed2 = new QTextEdit;
        ed2->setWindowTitle("Editor 2");
        ed2->show();

        auto *win1 = new RasterWindow(Qt::red);
        win1->setTitle("Window 1");
        win1->resize(300, 200);
        win1->show();

        auto *win2 = new RasterWindow(Qt::green);
        win2->setTitle("Window 2");
        win2->resize(300, 200);
        win2->show();

        auto *noFocus = new RasterWindow(Qt::black);
        noFocus->setTitle("No focus");
        noFocus->resize(300, 200);
        noFocus->setFlags(noFocus->flags() | Qt::WindowDoesNotAcceptFocus);
        noFocus->show();

        lab = new QLabel(this);
        lab->move(5, 5);
        connect(qApp, &QGuiApplication::focusObjectChanged, this, &Controller::updateInfo);

        auto *topLayout = new QVBoxLayout(this);
        topLayout->addWidget(lab);

        topLayout->addWidget(new ActivateButton("Activate editor 1", ed1->windowHandle()));
        topLayout->addWidget(new ActivateButton("Activate editor 2", ed2->windowHandle()));
        topLayout->addWidget(new ActivateButton("Activate window 1", win1));
        topLayout->addWidget(new ActivateButton("Activate window 2", win2));

        topLayout->addStretch();
        QPushButton *quitButton = new QPushButton("Quit");
        topLayout->addWidget(quitButton);
        connect(quitButton, &QPushButton::clicked, this, [](){
            QApplication::quit();
        });

        resize(800, 600);
    }

public slots:
    void updateInfo() {
        QString outString;
        QWindow *fWin = QGuiApplication::focusWindow();
        QString windowTitle = fWin ? fWin->title() : QString{};
        QWidget *aWdg = QApplication::activeWindow();
        QString widgetTitle = aWdg ? aWdg->windowTitle() : QString{};
        QDebug(&outString) << "focus window" << fWin << windowTitle
                           << "\nfocus object" << QGuiApplication::focusObject()
                           << "\nfocus widget" << QApplication::focusWidget()
                           << "\nactive widget" << aWdg << widgetTitle;
        lab->setText(outString);
    }

private:
    QLabel *lab = nullptr;
};



int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Controller controller;
    controller.show();
    controller.updateInfo();

    return app.exec();
}

#include "main.moc"
