/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QGraphicsView>
#include <QTime>
#include <QTimer>

#include "settings.h"

class QGraphicsScene;
class QGraphicsLinearLayout;
class QResizeEvent;
class Label;
class Menu;
class BackgroundItem;
class TopBar;

class MainView : public QGraphicsView {

Q_OBJECT

public:
    MainView(const bool enableOpenGL, const bool outputFps, const bool imageBasedRendering = false, QWidget *parent = 0);
    ~MainView();

    void setTestWidget(QGraphicsWidget *testWidget);
    QGraphicsWidget *takeTestWidget();
    QGraphicsWidget *testWidget();

    qreal fps();
    void fpsReset();
    void setImageBasedRendering(const bool imageBasedRendering);
    bool imageBasedRendering() const;
    Menu *menu();
    int rotationAngle() const;

signals:
    void repainted();

public slots:
    void rotateContent(int angle);

protected:

    virtual void resizeEvent(QResizeEvent * event);
    virtual void paintEvent(QPaintEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void wheelEvent(QWheelEvent *event) { Q_UNUSED (event); };

private slots:
    void updateFps();

private:
    void construct();
    void resizeContent(const QSize &s);

private:
    Q_DISABLE_COPY(MainView)

    QGraphicsScene *m_scene;
    QGraphicsLinearLayout *m_mainLayout;
    QGraphicsWidget *m_mainWidget;
    QGraphicsWidget *m_testWidget;
    Menu* m_menu;
    BackgroundItem* m_backGround;
    TopBar* m_topBar;

    bool m_imageBasedRendering;
    QPixmap *m_pixmapToRender;
    // Used for FPS
    int m_frameCount;
    QTime m_fpsFirstTs;
    QTime m_fpsLatestTs;
    bool m_OutputFps;
    QTime m_fpsUpdated;
    QList<qreal> m_Fpss;

    int m_angle;
    bool m_enableOpenGL;
};

#endif //__MAINWINDOW_H__
