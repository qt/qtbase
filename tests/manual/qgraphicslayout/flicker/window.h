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

#ifndef WINDOW_H
#define WINDOW_H


#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include <QGraphicsView>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QThread>
#include <QMap>
#include <QTime>
#include <QDebug>

struct Statistics {
    Statistics() : setGeometryCount(0), sleepMsecs(0), output(0),
        currentBenchmarkIteration(0), relayoutClicked(false)
    {
    }
    QMap<QGraphicsWidget*, int> setGeometryTracker;
    QTime time;
    int setGeometryCount;
    int sleepMsecs;
    QLabel *output;
    void sleep()
    {
#if QT_VERSION >= 0x050000
        QThread::msleep(sleepMsecs);
#else
        qWarning("%s unimplemented", Q_FUNC_INFO);
#endif
    }
    int currentBenchmarkIteration;
    bool relayoutClicked;

};


class Window;

class SlowWidget : public QGraphicsWidget {
public:
    SlowWidget(QGraphicsWidget *w = nullptr, Qt::WindowFlags wFlags = {}) : QGraphicsWidget(w, wFlags)
    {
        m_window = 0;
    }

    void setStats(Statistics *stats)
    {
        m_stats = stats;
    }

    void setWindow(Window *window)
    {
        m_window = window;
    }

    void setGeometry(const QRectF &rect);

    bool event(QEvent *e)
    {
        if (e->type() == QEvent::LayoutRequest) {
            if (m_stats->sleepMsecs > 0) {
                m_stats->sleep();
                qDebug("sleep %d ms\n", m_stats->sleepMsecs);
            }
        }
        return QGraphicsWidget::event(e);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->setBrush(m_brush);
        painter->drawRoundedRect(rect(), 25, 25, Qt::RelativeSize);
        painter->drawLine(rect().topLeft(), rect().bottomRight());
        painter->drawLine(rect().bottomLeft(), rect().topRight());
    }

    void setBrush(const QBrush &brush)
    {
        m_brush = brush;
    }
private:
    QBrush m_brush;
    Statistics *m_stats;
    Window *m_window;
};

class Window : public QWidget {
    Q_OBJECT
public:
    Window() : QWidget()
    {
        QGraphicsView *m_view = new QGraphicsView(&scene);

        m_window = 0;
        m_leaf = 0;

        m_button = new QPushButton(tr("Relayout"));
        m_button->setObjectName("button");

        m_sleepLabel = new QLabel(tr("Sleep:"));
        m_sleepSpinBox = new QSpinBox;
        m_sleepSpinBox->setRange(0, 1000);
        m_sleepSpinBox->setSingleStep(10);

        m_depthLabel = new QLabel(tr("Depth:"));
        m_depthSpinBox = new QSpinBox;
        m_depthSpinBox->setObjectName("depthSpinBox");
        m_depthSpinBox->setRange(1, 200);
        m_depthSpinBox->setSingleStep(5);

        m_benchmarkIterationsLabel = new QLabel(tr("Benchmark iterations"));
        m_benchmarkIterationsSpinBox = new QSpinBox;
        m_benchmarkIterationsSpinBox->setObjectName("benchmarkIterationsSpinBox");
        m_benchmarkIterationsSpinBox->setRange(1, 1000);
        m_benchmarkIterationsSpinBox->setValue(41);
        m_benchmarkIterationsSpinBox->setSingleStep(10);

        m_instantCheckBox = new QCheckBox(tr("Instant propagation"));
        m_instantCheckBox->setObjectName("instantPropagationCheckbox");
        QGraphicsLayout::setInstantInvalidatePropagation(true);
        m_instantCheckBox->setChecked(QGraphicsLayout::instantInvalidatePropagation());

        m_resultLabel = new QLabel(tr("Press relayout to start test"));

        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addWidget(m_sleepLabel);
        hbox->addWidget(m_sleepSpinBox);
        hbox->addWidget(m_depthLabel);
        hbox->addWidget(m_depthSpinBox);
        hbox->addWidget(m_benchmarkIterationsLabel);
        hbox->addWidget(m_benchmarkIterationsSpinBox);
        hbox->addWidget(m_instantCheckBox);
        hbox->addWidget(m_resultLabel);
        hbox->addStretch();
        hbox->addWidget(m_button);

        QVBoxLayout *vbox = new QVBoxLayout;
        vbox->addWidget(m_view);
        vbox->addLayout(hbox);
        setLayout(vbox);

        metaObject()->connectSlotsByName(this);

        m_depthSpinBox->setValue(20);   // triggers purposedly on_depthSpinBox_valueChanged
    }

private slots:
    void on_depthSpinBox_valueChanged(int value)
    {
        m_stats.relayoutClicked = false;
        if (m_window) {
            QApplication::processEvents();
            delete m_window;
        }
        m_window = new SlowWidget(0, Qt::Window);
        m_window->setStats(&m_stats);
        m_window->setWindow(this);
        QColor col(Qt::black);
        m_window->setBrush(col);
        scene.addItem(m_window);
        m_leaf = 0;
        const int depth = value;
        SlowWidget *parent = m_window;
        for (int i = 1; i < depth; ++i) {
            QGraphicsLinearLayout *l = new QGraphicsLinearLayout(parent);
            l->setContentsMargins(2,2,2,2);
            SlowWidget *child = new SlowWidget;
            QColor col;
            col.setHsl(0, 0, 255*i/(depth - 1));
            child->setBrush(col);
            child->setStats(&m_stats);
            child->setWindow(this);
            l->addItem(child);
            parent = child;
        }
        m_leaf = parent;
    }

    void on_button_clicked(bool /*check = false*/)
    {
        m_stats.relayoutClicked = true;
        if (m_leaf) {
            QSizeF sz = m_leaf->size();
            int w = int(sz.width());
            w^=16;
            sz = QSizeF(w,w);
            m_stats.output = m_resultLabel;
            m_stats.output->setText(QString("wait..."));
            m_stats.setGeometryCount = 0;
            m_stats.setGeometryTracker.clear();
            m_stats.sleepMsecs = m_sleepSpinBox->value();
            m_stats.time.start();
            m_stats.currentBenchmarkIteration = 0;
            m_leaf->setMinimumSize(sz);
            m_leaf->setMaximumSize(sz);
        }
    }

    void on_instantPropagationCheckbox_toggled(bool checked)
    {
        QGraphicsLayout::setInstantInvalidatePropagation(checked);
    }

public slots:
    void doAgain()
    {
        if (m_leaf) {
            QSizeF sz = m_leaf->size();
            int w = int(sz.width());
            w^=16;
            sz = QSizeF(w,w);
            m_leaf->setMinimumSize(sz);
            m_leaf->setMaximumSize(sz);
        }
    }

private:
public:
    QGraphicsScene scene;
    QGraphicsView *m_view;
    QPushButton *m_button;
    QLabel *m_sleepLabel;
    QSpinBox *m_sleepSpinBox;
    QLabel *m_depthLabel;
    QSpinBox *m_depthSpinBox;
    QLabel *m_benchmarkIterationsLabel;
    QSpinBox *m_benchmarkIterationsSpinBox;
    QCheckBox *m_instantCheckBox;
    QLabel *m_resultLabel;
    QGraphicsWidget *m_leaf;
    SlowWidget *m_window;
    Statistics m_stats;


};


#endif //WINDOW_H
