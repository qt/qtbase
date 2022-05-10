// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QRandomGenerator>

class tst_QGraphicsWidget : public QObject
{
    Q_OBJECT

public:
    tst_QGraphicsWidget();
    virtual ~tst_QGraphicsWidget();

public slots:
    void init();
    void cleanup();

private slots:
    void move();
};

tst_QGraphicsWidget::tst_QGraphicsWidget()
{
}

tst_QGraphicsWidget::~tst_QGraphicsWidget()
{
}

void tst_QGraphicsWidget::init()
{
}

void tst_QGraphicsWidget::cleanup()
{
}

void tst_QGraphicsWidget::move()
{
    QGraphicsScene scene;
    QGraphicsWidget *widget = new QGraphicsWidget();
    scene.addItem(widget);
    QGraphicsView view(&scene);
    view.show();
    QBENCHMARK {
        // truncate the random values to 24 bits to
        // avoid overflowing
        widget->setPos(QRandomGenerator::global()->generate() & 0xffffff, QRandomGenerator::global()->generate() & 0xffffff);
    }
}

QTEST_MAIN(tst_QGraphicsWidget)
#include "tst_qgraphicswidget.moc"
