/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QApplication>
#include <QGraphicsLinearLayout>
#ifndef QT_NO_OPENGL
#include <QGLWidget>
#endif
#include <QObject>

#include "button.h"
#include "label.h"
#include "menu.h"
#include "topbar.h"
#include "backgrounditem.h"
#include "theme.h"
#include "mainview.h"
#include "gvbwidget.h"

MainView::MainView(const bool enableOpenGL, const bool outputFps, const bool imageRendering, QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(0)
    , m_mainLayout(0)
    , m_mainWidget(0)
    , m_testWidget(0)
    , m_imageBasedRendering(imageRendering)
    , m_pixmapToRender(0)
    , m_OutputFps(outputFps)
    , m_fpsUpdated()
    , m_Fpss()
    , m_angle(0)
    , m_enableOpenGL(enableOpenGL)
{
    construct();
}

MainView::~MainView()
{
    if (!m_scene->parent())
        delete m_scene;

    delete m_pixmapToRender;
}

void MainView::setTestWidget(QGraphicsWidget *testWidget)
{
    if (!testWidget)
        return;

    if (m_testWidget) {
        m_mainLayout->removeItem(m_testWidget);
        if (!m_testWidget->parent() && !m_testWidget->parentLayoutItem())
            delete m_testWidget;
    }
    m_testWidget = testWidget;
    m_mainLayout->addItem(m_testWidget);
    resizeContent(size());
}

QGraphicsWidget *MainView::takeTestWidget()
{
    if (m_testWidget) {
        m_mainLayout->removeItem(m_testWidget);
        QGraphicsWidget *tmp = m_testWidget;
        m_testWidget = 0;
        return tmp;
    }
    return 0;
}

QGraphicsWidget *MainView::testWidget()
{
    return m_testWidget;
}

void MainView::setImageBasedRendering(const bool imageBasedRendering)
{
    m_imageBasedRendering = imageBasedRendering;
    delete m_pixmapToRender;
    m_pixmapToRender = 0;
    viewport()->update();
}

bool MainView::imageBasedRendering() const
{
    return m_imageBasedRendering;
}

qreal MainView::fps()
{
    if (m_Fpss.count() <= 0)
        updateFps();

    if (m_Fpss.count() <= 0)
        return 0.0;

    qreal sum = 0;
    int count = m_Fpss.count();
    for (int i = 0; i<count; ++i)
        sum += m_Fpss.at(i);
    m_Fpss.clear();
    fpsReset();
    return sum/qreal(count);
}

void MainView::fpsReset()
{
    m_frameCount = 0;
    m_fpsFirstTs.start();
    m_fpsLatestTs = m_fpsFirstTs;
    m_fpsUpdated.start();
}

void MainView::rotateContent(int angle)
{
    bool portrait = ((m_angle+angle)%90 == 0) && ((m_angle+angle)%180 != 0);
    bool landscape = ((m_angle+angle)%180 == 0);
    if (!portrait && !landscape)
        return;

    m_angle = (m_angle + angle)%360;

    rotate(angle);

    resizeContent(size());
}

int MainView::rotationAngle() const
{
    return m_angle;
}

void MainView::resizeContent(const QSize &s)
{
    QSizeF l_size(s);
    QSizeF p_size(l_size.height(), l_size.width());
    bool portrait = (m_angle%90 == 0) && (m_angle%180 != 0);
    if (portrait) {
        m_mainWidget->resize(p_size);
        m_backGround->resize(p_size);
    }
    else {
        m_mainWidget->resize(l_size);
        m_backGround->resize(l_size);
    }
    m_menu->setPos(m_topBar->getStatusBarLocation());
    setSceneRect(QRectF(m_mainWidget->pos(), m_mainWidget->size()));
}

void MainView::resizeEvent(QResizeEvent * event)
{
    QGraphicsView::resizeEvent(event);
    resizeContent(event->size());
}

void MainView::paintEvent (QPaintEvent *event)
{
    if (m_imageBasedRendering) {
        if (!m_pixmapToRender)
            m_pixmapToRender = new QPixmap(size());

        if (m_pixmapToRender->size() != size()) {
            delete m_pixmapToRender;
            m_pixmapToRender = new QPixmap(size());
        }
        QPainter p(m_pixmapToRender);
        render(&p);
        p.end();
    }
    else {
        QGraphicsView::paintEvent(event);
    }

    if (!m_OutputFps)
        emit repainted();

    m_frameCount++;
    m_fpsLatestTs.start();
    if(m_fpsUpdated.elapsed() > 2000) {
        updateFps();
        m_fpsUpdated.start();
    }
}

void MainView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F) {
        if (isFullScreen())
            showNormal();
        else
            showFullScreen();
    }

    //S60 3.x specific
    if(m_menu->menuVisible()) {
        m_menu->keyPressEvent(event);
        return;
    }

    if(event->key() == 16777235 ) { //Up Arrow
        GvbWidget* widget = qobject_cast<GvbWidget*>(m_testWidget);
        if(widget)
            widget->keyPressEvent(event);
    }

    if(event->key() == 16777237 ) { //Down Arrow
        GvbWidget* widget = qobject_cast<GvbWidget*>(m_testWidget);
        if(widget)
            widget->keyPressEvent(event);
    }

    if(event->key() == 17825792 ) { //LSK
        if(!m_menu->menuVisible())
            m_menu->menuShowHide();
    }

    if(event->key() == 17825793 ) { //RSK
        QApplication::quit();
    }
}

void MainView::construct()
{
    m_scene = new QGraphicsScene;

#ifndef QT_NO_OPENGL
    if (m_enableOpenGL) {
        qDebug() << "OpenGL enabled";
        m_scene->setSortCacheEnabled(false);
        setViewport(new QGLWidget);

        // Qt doc says: This is the preferred update mode for
        // viewports that do not support partial updates, such as QGLWidget...
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    } else
#endif
        setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    setScene(m_scene);
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);

    //setCacheMode(QGraphicsView::CacheBackground);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Turn off automatic background
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);

    //Background
    m_backGround = new BackgroundItem("background.svg");
    m_scene->addItem(m_backGround);
    m_backGround->setZValue(0);

    //Menu
    m_menu = new Menu(this);
    m_scene->addItem(m_menu); //Add menu to the scene directly
    m_menu->setZValue(10); //Bring to front

    m_mainLayout = new QGraphicsLinearLayout(Qt::Vertical);
    m_mainLayout->setContentsMargins(0,0,0,0);
    m_mainLayout->setSpacing(0);

    m_mainWidget = new QGraphicsWidget;
    m_mainWidget->setLayout(m_mainLayout);
    m_mainWidget->setZValue(1);
    m_scene->addItem(m_mainWidget);

    //Topbar
    m_topBar = new TopBar(this, 0);
    m_mainLayout->addItem(m_topBar);
    m_topBar->setZValue(1);
    connect(m_topBar, SIGNAL(clicked(bool)), m_menu, SLOT(menuShowHide()));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setContentsMargins(0,0,0,0);
    setViewportMargins(0,0,0,0);
    setFrameShape(QFrame::NoFrame);

    fpsReset();
    m_fpsUpdated.start();
}

void MainView::updateFps()
{
    int msecs =  m_fpsFirstTs.msecsTo(m_fpsLatestTs);
    qreal fps = 0;
    if (msecs > 0) {
        fps = m_frameCount * 1000.0 / msecs;

        if (m_OutputFps)
            qDebug() << "FPS: " << fps;

        m_Fpss.append(fps);
    }
    m_fpsFirstTs = m_fpsLatestTs;
    m_frameCount = 0;
}

Menu *MainView::menu()
{
    return m_menu;
}
