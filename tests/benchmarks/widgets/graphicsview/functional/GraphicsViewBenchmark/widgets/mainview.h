// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QGraphicsView>
#include <QTime>
#include <QElapsedTimer>
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
    MainView(const bool enableOpenGL, const bool outputFps, const bool imageBasedRendering = false, QWidget *parent = nullptr);
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
    virtual void wheelEvent(QWheelEvent *event) { Q_UNUSED (event); }

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
    QElapsedTimer m_fpsUpdated;
    QList<qreal> m_Fpss;

    int m_angle;
    bool m_enableOpenGL;
};

#endif //__MAINWINDOW_H__
