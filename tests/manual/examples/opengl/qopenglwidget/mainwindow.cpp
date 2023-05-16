// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include <QApplication>
#include <QMenuBar>
#include <QGroupBox>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QScrollArea>
#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>

#include "glwidget.h"

MainWindow::MainWindow()
    : m_nextX(1), m_nextY(1)
{
    GLWidget *glwidget = new GLWidget(this, qRgb(20, 20, 50));
    m_glWidgets << glwidget;
    QLabel *label = new QLabel(this);
    m_timer = new QTimer(this);
    QSlider *slider = new QSlider(this);
    slider->setOrientation(Qt::Horizontal);

    QLabel *updateLabel = new QLabel("Update interval");
    QSpinBox *updateInterval = new QSpinBox(this);
    updateInterval->setSuffix(" ms");
    updateInterval->setValue(10);
    updateInterval->setToolTip("Interval for the timer that calls update().\n"
                               "Note that on most systems the swap will block to wait for vsync\n"
                               "and therefore an interval < 16 ms will likely lead to a 60 FPS update rate.");
    QGroupBox *updateGroupBox = new QGroupBox(this);
    QCheckBox *timerBased = new QCheckBox("Use timer", this);
    timerBased->setChecked(false);
    timerBased->setToolTip("Toggles using a timer to trigger update().\n"
                           "When not set, each paintGL() schedules the next update immediately,\n"
                           "expecting the blocking swap to throttle the thread.\n"
                           "This shows how unnecessary the timer is in most cases.");
    QCheckBox *transparent = new QCheckBox("Transparent background", this);
    transparent->setToolTip("Toggles Qt::WA_AlwaysStackOnTop and transparent clear color for glClear().\n"
                            "Note how the button on top stacks incorrectly when enabling this.");
    QHBoxLayout *updateLayout = new QHBoxLayout;
    updateLayout->addWidget(updateLabel);
    updateLayout->addWidget(updateInterval);
    updateLayout->addWidget(timerBased);
    updateLayout->addWidget(transparent);
    updateGroupBox->setLayout(updateLayout);

    slider->setRange(0, 50);
    slider->setSliderPosition(30);
    m_timer->setInterval(10);
    label->setText("A scrollable QOpenGLWidget");
    label->setAlignment(Qt::AlignHCenter);

    QGroupBox * groupBox = new QGroupBox(this);
    setCentralWidget(groupBox);
    groupBox->setTitle("QOpenGLWidget Example");

    m_layout = new QGridLayout(groupBox);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidget(glwidget);

    m_layout->addWidget(scrollArea,1,0,8,1);
    m_layout->addWidget(label,9,0,1,1);
    m_layout->addWidget(updateGroupBox, 10, 0, 1, 1);
    m_layout->addWidget(slider, 11,0,1,1);

    groupBox->setLayout(m_layout);


    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("E&xit", this, &QWidget::close);
    QMenu *showMenu = menuBar()->addMenu("&Show");
    showMenu->addAction("Show 3D Logo", glwidget, &GLWidget::setLogo);
    showMenu->addAction("Show 2D Texture", glwidget, &GLWidget::setTexture);
    QAction *showBubbles = showMenu->addAction("Show bubbles", glwidget, &GLWidget::setShowBubbles);
    showBubbles->setCheckable(true);
    showBubbles->setChecked(true);
    showMenu->addAction("Open tab window", this, &MainWindow::showNewWindow);
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("About Qt", qApp, &QApplication::aboutQt);

    connect(m_timer, &QTimer::timeout, glwidget, QOverload<>::of(&QWidget::update));

    connect(slider, &QAbstractSlider::valueChanged, glwidget, &GLWidget::setScaling);
    connect(transparent, &QCheckBox::toggled, glwidget, &GLWidget::setTransparent);
    connect(updateInterval, &QSpinBox::valueChanged,
            this, &MainWindow::updateIntervalChanged);
    connect(timerBased, &QCheckBox::toggled, this, &MainWindow::timerUsageChanged);
    connect(timerBased, &QCheckBox::toggled, updateInterval, &QWidget::setEnabled);

    if (timerBased->isChecked())
        m_timer->start();
    else
        updateInterval->setEnabled(false);
}

void MainWindow::updateIntervalChanged(int value)
{
    m_timer->setInterval(value);
    if (m_timer->isActive())
        m_timer->start();
}

void MainWindow::addNew()
{
    if (m_nextY == 4)
        return;
    GLWidget *w = new GLWidget(nullptr, qRgb(QRandomGenerator::global()->bounded(256),
                                             QRandomGenerator::global()->bounded(256),
                                             QRandomGenerator::global()->bounded(256)));
    m_glWidgets << w;
    connect(m_timer, &QTimer::timeout, w, QOverload<>::of(&QWidget::update));
    m_layout->addWidget(w, m_nextY, m_nextX, 1, 1);
    if (m_nextX == 3) {
        m_nextX = 1;
        ++m_nextY;
    } else {
        ++m_nextX;
    }
}

void MainWindow::timerUsageChanged(bool enabled)
{
    if (enabled) {
        m_timer->start();
    } else {
        m_timer->stop();
        for (QOpenGLWidget *w : std::as_const(m_glWidgets))
            w->update();
    }
}

void MainWindow::resizeEvent(QResizeEvent *)
{
     m_glWidgets[0]->setMinimumSize(size() + QSize(128, 128));
}

void MainWindow::showNewWindow()
{
    QTabWidget *tabs = new QTabWidget;
    tabs->resize(800, 600);

    QToolButton *tb = new QToolButton;
    tb->setText(QLatin1String("+"));
    tabs->addTab(new QLabel(QLatin1String("Add OpenGL widgets with +")), QString());
    tabs->setTabEnabled(0, false);
    tabs->tabBar()->setTabButton(0, QTabBar::RightSide, tb);
    tabs->tabBar()->setTabsClosable(true);
    QObject::connect(tabs->tabBar(), &QTabBar::tabCloseRequested, tabs, [tabs](int index) {
        tabs->widget(index)->deleteLater();
    });

    const QString msgToTopLevel = QLatin1String("Break out to top-level window");
    const QString msgFromTopLevel = QLatin1String("Move back under tab widget");

    QObject::connect(tb, &QAbstractButton::clicked, tabs, [=] {
        GLWidget *glwidget = new GLWidget(nullptr, Qt::blue);
        glwidget->resize(tabs->size());
        glwidget->setWindowTitle(QString::asprintf("QOpenGLWidget %p", glwidget));

        QPushButton *btn = new QPushButton(msgToTopLevel, glwidget);
        connect(btn, &QPushButton::clicked, glwidget, [=] {
            if (glwidget->parent()) {
                glwidget->setAttribute(Qt::WA_DeleteOnClose, true);
                glwidget->setParent(nullptr);
                glwidget->show();
                btn->setText(msgFromTopLevel);
            } else {
                glwidget->setAttribute(Qt::WA_DeleteOnClose, false);
                tabs->addTab(glwidget, glwidget->windowTitle());
                btn->setText(msgToTopLevel);
            }
        });

        tabs->setCurrentIndex(tabs->addTab(glwidget, glwidget->windowTitle()));
    });

    tabs->setAttribute(Qt::WA_DeleteOnClose);
    tabs->show();
}
