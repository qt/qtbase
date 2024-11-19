// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtWidgets>

class WindowManagedWindow: public QWidget
{
    Q_OBJECT
public:
    WindowManagedWindow(QWidget *parent = nullptr)
    :QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout();

        // UI for controlling window show state
        QHBoxLayout *showWindowLayout = new QHBoxLayout();
        layout->addLayout(showWindowLayout);
        QPushButton *showNormal = new QPushButton("Show Normal");
        QPushButton *showFullscreen = new QPushButton("Show FullScreen");
        QPushButton *showMaximized = new QPushButton("Show Maximized");
        QPushButton *close = new QPushButton("Close");

        showWindowLayout->addWidget(showNormal);
        showWindowLayout->addWidget(showFullscreen);
        showWindowLayout->addWidget(showMaximized);
        showWindowLayout->addWidget(close);

        connect(showNormal, &QPushButton::clicked, [=]() {
            this->showNormal();
        });
        connect(showFullscreen, &QPushButton::clicked, [=]() {
            this->showFullScreen();
        });
        connect(showMaximized, &QPushButton::clicked, [=]() {
            this->showMaximized();
        });
        connect(close, &QPushButton::clicked, [=]() {
            this->close();
        });

        // UI for adding a child window
        QHBoxLayout *childWindowLayout = new QHBoxLayout();
        layout->addLayout(childWindowLayout);        
        QPushButton *showChild = new QPushButton("Add Child Window");
        childWindowLayout->addWidget(showChild);
        connect(showChild, &QPushButton::clicked, [=]() {
            WindowManagedWindow *childWindow = new WindowManagedWindow(this);
            childWindow->winId(); // ensure it has a QWindow
            childWindow->setBackgroundColor("AliceBlue");
            childWindowLayout->insertWidget(1, childWindow);
            childWindow->show();
        });
        childWindowLayout->addStretch();

        // UI for showing a transient child window
        QHBoxLayout *transientChildWindowLayout = new QHBoxLayout();
        layout->addLayout(transientChildWindowLayout);
        QPushButton *showTransientChild = new QPushButton("Add Transient Child Window");
        transientChildWindowLayout->addWidget(showTransientChild);
        QRadioButton *modal = new QRadioButton("Modal Dialog");
        transientChildWindowLayout->addWidget(modal);
        QRadioButton *popup = new QRadioButton("Popup");
        transientChildWindowLayout->addWidget(popup);
        QButtonGroup *radioGroup = new QButtonGroup();
        radioGroup->addButton(modal);
        radioGroup->addButton(popup);
        modal->setChecked(true);

        connect(showTransientChild, &QPushButton::clicked, [this, modal]() {
            WindowManagedWindow *transientChildWindow = new WindowManagedWindow();
            transientChildWindow->winId();

            if (modal->isChecked()) {
                transientChildWindow->setWindowFlag(Qt::Dialog);
                transientChildWindow->setWindowModality(Qt::WindowModal);
                transientChildWindow->setBackgroundColor("MistyRose");
            } else {
                transientChildWindow->setWindowFlag(Qt::Popup);
                transientChildWindow->setBackgroundColor("LavenderBlush");
            }

            transientChildWindow->windowHandle()->setTransientParent(this->windowHandle());
            transientChildWindow->show();
            
        });
        transientChildWindowLayout->addStretch();

        // UI for adding a new top-level window
        if (!parentWidget()) {
            QHBoxLayout *addWindowLayout = new QHBoxLayout();
            layout->addLayout(addWindowLayout);
            QPushButton *addWindow = new QPushButton("Add Top-level Window");
            QRadioButton *addNormal = new QRadioButton("Normal");
            QRadioButton *addFullScreen = new QRadioButton("FullScreen");
            QRadioButton *addMaximized = new QRadioButton("Maximized");

            QButtonGroup *radioGroup = new QButtonGroup();
            radioGroup->addButton(addNormal);
            radioGroup->addButton(addFullScreen);
            radioGroup->addButton(addMaximized);
            addNormal->setChecked(true);

            addWindowLayout->addWidget(addWindow);
            addWindowLayout->addWidget(addNormal);
            addWindowLayout->addWidget(addFullScreen);
            addWindowLayout->addWidget(addMaximized);

            connect(addWindow, &QPushButton::clicked, [=]() {
                QWidget *newWindow = new WindowManagedWindow();
                if (addNormal->isChecked()) {
                    newWindow->showNormal();
                } else if (addFullScreen->isChecked()) {
                    newWindow->showFullScreen();
                } else if (addMaximized->isChecked()) {
                    newWindow->showMaximized();
                }
            });
        }

        // Test label
        QHBoxLayout *labelLayout = new QHBoxLayout();
        layout->addLayout(labelLayout);
        labelLayout->addWidget(new QLabel("Test Text Input:"));
        labelLayout->addWidget(new QLineEdit());
        labelLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        setLayout(layout);

        updateWindowFocusState();
    }

    void paintEvent(QPaintEvent *event) override
    {
        updateWindowFocusState(); // hacky update
        QWidget::paintEvent(event);
    }

    void updateWindowFocusState()
    {
        QString title = QString("isActive: %1 Modal: %2")
            .arg(isActiveWindow() ? "yes" : "no")
            .arg(isModal() ? "yes" : "no");
        setWindowTitle(title);
    }

    void setBackgroundColor(const QString &colorName)
    {
        QColor color(colorName);
        if (!color.isValid()) {
            qWarning("Invalid color name provided: %s", qPrintable(colorName));
            return;
        }
        QPalette palette = this->palette();
        palette.setColor(QPalette::Window, color);
        this->setAutoFillBackground(true);
        this->setPalette(palette);
    }
};

int main(int argc, char **argv)
{
    // Limit native widgets to the WindowManagedWindow instances
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);

    QObject::connect(qApp, &QApplication::focusChanged, [](QWidget *oldWidget, QWidget *newWidget) {
        if (oldWidget)
            if (WindowManagedWindow *window = qobject_cast<WindowManagedWindow *>(oldWidget->window()))
                window->updateWindowFocusState();
        if (newWidget)
            if (WindowManagedWindow *window = qobject_cast<WindowManagedWindow *>(newWidget->window()))
                window->updateWindowFocusState();
    });

    WindowManagedWindow window;
    window.showNormal();

    return app.exec();
}

#include "main.moc"
