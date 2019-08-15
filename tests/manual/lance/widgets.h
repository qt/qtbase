/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
#ifndef WIDGETS_H
#define WIDGETS_H

#include "paintcommands.h"

#include <QWidget>
#include <QSettings>
#include <QFileInfo>
#include <QPainter>
#include <QPaintEvent>
#include <QListWidgetItem>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QFileDialog>
#include <QTextStream>
#include <QPaintEngine>
#include <QAction>
#include <QDebug>

#include <qmath.h>

const int CP_RADIUS = 10;

template <class T>
class OnScreenWidget : public T
{
public:

    enum ViewMode {
        RenderView,
        BaselineView,
        DifferenceView
    };

    OnScreenWidget(const QString &file, QWidget *parent = nullptr)
        : T(parent),
          m_filename(file),
          m_view_mode(RenderView)
    {
        QSettings settings("QtProject", "lance");
        for (int i=0; i<10; ++i) {
            QPointF suggestion(100 + i * 40, 100 + 100 * qSin(i * 3.1415 / 10.0));
            m_controlPoints << settings.value("cp" + QString::number(i), suggestion).toPointF();
        }

        m_currentPoint = -1;
        m_showControlPoints = false;
        m_deviceType = WidgetType;
        m_checkersBackground = true;
        m_verboseMode = false;

        m_baseline_name = QString(m_filename).replace(".qps", "_qps") + ".png";
        if (QFileInfo(m_baseline_name).exists()) {
            m_baseline = QPixmap(m_baseline_name);
        }

        if (m_baseline.isNull()) {
            T::setWindowTitle("Rendering: '" + file + "'. No baseline available");
        } else {
            T::setWindowTitle("Rendering: '" + file + "'. Shortcuts: 1=render, 2=baseline, 3=difference");

            QAction *renderViewAction = new QAction("Render View", this);
            renderViewAction->setShortcut(Qt::Key_1);
            T::connect(renderViewAction, &QAction::triggered, [&] { setMode(RenderView); });
            T::addAction(renderViewAction);

            QAction *baselineAction = new QAction("Baseline", this);
            baselineAction->setShortcut(Qt::Key_2);
            T::connect(baselineAction, &QAction::triggered, [&] { setMode(BaselineView); });
            T::addAction(baselineAction);

            QAction *differenceAction = new QAction("Difference View", this);
            differenceAction->setShortcut(Qt::Key_3);
            T::connect(differenceAction, &QAction::triggered, [&] { setMode(DifferenceView); });
            T::addAction(differenceAction);
        }

    }

    ~OnScreenWidget()
    {
        QSettings settings("QtProject", "lance");
        for (int i=0; i<10; ++i) {
            settings.setValue("cp" + QString::number(i), m_controlPoints.at(i));
        }
        settings.sync();
    }

    void setMode(ViewMode mode) {
        m_view_mode = mode;
        QString title;
        switch (m_view_mode) {
        case RenderView: title = "Render"; break;
        case BaselineView: title = "Baseline"; break;
        case DifferenceView: title = "Difference"; break;
        }
        T::setWindowTitle(title + " View: " + m_filename);
        T::update();
    }

    void setVerboseMode(bool v) { m_verboseMode = v; }
    void setCheckersBackground(bool b) { m_checkersBackground = b; }
    void setType(DeviceType t) { m_deviceType = t; }

    void resizeEvent(QResizeEvent *e) {
        m_image = QImage();
        T::resizeEvent(e);
    }

    void paintEvent(QPaintEvent *) {
        switch (m_view_mode) {
        case RenderView: paintRenderView(); break;
        case BaselineView: paintBaselineView(); break;
        case DifferenceView: paintDifferenceView(); break;
        }
    }

    void paintRenderView()
    {
        QPainter pt;
        QPaintDevice *dev = this;
        if (m_deviceType == ImageWidgetType) {
            if (m_image.size() != T::size())
                m_image = QImage(T::size(), QImage::Format_ARGB32_Premultiplied);
            m_image.fill(0);
            dev = &m_image;
        }

        pt.begin(dev);

        PaintCommands paintCommands(m_commands, 800, 800, QImage::Format_ARGB32_Premultiplied);
        paintCommands.setVerboseMode(m_verboseMode);
        paintCommands.setCheckersBackground(m_checkersBackground);
        paintCommands.setType(m_deviceType);
        paintCommands.setPainter(&pt);
        paintCommands.setControlPoints(m_controlPoints);
        paintCommands.setFilePath(QFileInfo(m_filename).absolutePath());
#ifdef DO_QWS_DEBUGGING
        qt_show_painter_debug_output = true;
#endif
        pt.save();
        paintCommands.runCommands();
        pt.restore();
#ifdef DO_QWS_DEBUGGING
        qt_show_painter_debug_output = false;
#endif

        pt.end();

        if (m_deviceType == ImageWidgetType) {
            QPainter(this).drawImage(0, 0, m_image);
        }

        if (m_currentPoint >= 0 || m_showControlPoints) {
            pt.begin(this);
            pt.setRenderHint(QPainter::Antialiasing);
            pt.setFont(this->font());
            pt.resetTransform();
            pt.setPen(QColor(127, 127, 127, 191));
            pt.setBrush(QColor(191, 191, 255, 63));
            for (int i=0; i<m_controlPoints.size(); ++i) {
                if (m_showControlPoints || m_currentPoint == i) {
                    QPointF cp = m_controlPoints.at(i);
                    QRectF rect(cp.x() - CP_RADIUS, cp.y() - CP_RADIUS,
                                CP_RADIUS * 2, CP_RADIUS * 2);
                    pt.drawEllipse(rect);
                    pt.drawText(rect, Qt::AlignCenter, QString::number(i));
                }
            }
        }
#if 0
        // ### TBD: Make this work with Qt5
        if (m_render_view.isNull()) {
            m_render_view = QPixmap::grabWidget(this);
            m_render_view.save("renderView.png");
        }
#endif
    }

    void paintBaselineView() {
        QPainter p(this);

        if (m_baseline.isNull()) {
            p.drawText(T::rect(), Qt::AlignCenter,
                       "No baseline found\n"
                       "file '" + m_baseline_name + "' does not exist...");
            return;
        }

        p.drawPixmap(0, 0, m_baseline);

        p.setPen(QColor::fromRgbF(0, 0, 0, 0.1));
        p.setFont(QFont("Arial", 128));
        p.rotate(45);
        p.drawText(100, 0, "BASELINE");
    }

    QPixmap generateDifference()
    {
        QImage img(T::size(), QImage::Format_RGB32);
        img.fill(0);

        QPainter p(&img);
        p.drawImage(0, 0, m_image);

        p.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        p.drawPixmap(0, 0, m_baseline);

        p.end();

        return QPixmap::fromImage(img);
    }

    void paintDifferenceView() {
        QPainter p(this);
        if (m_baseline.isNull()) {
            p.drawText(T::rect(), Qt::AlignCenter,
                       "No baseline found\n"
                       "file '" + m_baseline_name + "' does not exist...");
            return;
        }

        p.fillRect(T::rect(), Qt::black);
        p.drawPixmap(0, 0, generateDifference());
    }


    void mouseMoveEvent(QMouseEvent *e)
    {
        if (m_currentPoint == -1)
            return;
        if (T::rect().contains(e->pos()))
            m_controlPoints[m_currentPoint] = e->pos();
        T::update();
    }

    void mousePressEvent(QMouseEvent *e)
    {
        if (e->button() == Qt::RightButton) {
            m_showControlPoints = true;
        }

        if (e->button() == Qt::LeftButton) {
            for (int i=0; i<m_controlPoints.size(); ++i) {
                if (QLineF(m_controlPoints.at(i), e->pos()).length() < CP_RADIUS) {
                    m_currentPoint = i;
                    break;
                }
            }
        }
        T::update();
    }

    void mouseReleaseEvent(QMouseEvent *e)
    {
        if (e->button() == Qt::LeftButton)
            m_currentPoint = -1;
        if (e->button() == Qt::RightButton)
            m_showControlPoints = false;
        T::update();
    }

    QSize sizeHint() const { return QSize(800, 800); }

    QVector<QPointF> m_controlPoints;
    int m_currentPoint;
    bool m_showControlPoints;

    QStringList m_commands;
    QString m_filename;
    QString m_baseline_name;

    bool m_verboseMode;
    bool m_checkersBackground;
    DeviceType m_deviceType;

    int m_view_mode;

    QImage m_image;

    QPixmap m_baseline;
    QPixmap m_render_view;
};

#endif
