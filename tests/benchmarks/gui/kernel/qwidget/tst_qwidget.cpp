/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qtest.h>

#include <QtWidgets/QLayout>
#include <QtGui/QPainter>

static void processEvents()
{
    QApplication::flush();
    QApplication::processEvents();
    QApplication::processEvents();
}

class UpdateWidget : public QWidget
{
public:
    UpdateWidget(int rows, int columns)
        : QWidget(0), rowCount(0), columnCount(0), opaqueChildren(false)
    {
        fill(rows, columns);
    }

    UpdateWidget(QWidget *parent = 0)
        : QWidget(parent), rowCount(0), columnCount(0), opaqueChildren(false) {}

    void fill(int rows, int columns)
    {
        if (rows == rowCount && columns == columnCount)
            return;
        delete layout();
        QGridLayout *layout = new QGridLayout;
        rowCount = rows;
        columnCount = columns;
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                UpdateWidget *widget = new UpdateWidget;
                widget->setFixedSize(20, 20);
                layout->addWidget(widget, row, column);
                children.append(widget);
            }
        }
        setLayout(layout);
        adjustSize();
        QTest::qWait(250);
        processEvents();
    }

    void setOpaqueChildren(bool enable)
    {
        if (opaqueChildren != enable) {
            foreach (QWidget *w, children)
                w->setAttribute(Qt::WA_OpaquePaintEvent, enable);
            opaqueChildren = enable;
            processEvents();
        }
    }

    void paintEvent(QPaintEvent *)
    {
        static int color = Qt::black;

        QPainter painter(this);
        painter.fillRect(rect(), Qt::GlobalColor(color));

        if (++color > Qt::darkYellow)
            color = Qt::black;
    }

    QRegion updateRegion;
    QList<UpdateWidget*> children;
    int rowCount;
    int columnCount;
    bool opaqueChildren;
};

class tst_QWidget : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init();

private slots:
    void update_data();
    void update();
    void updatePartial_data();
    void updatePartial();
    void updateComplex_data();
    void updateComplex();

private:
    UpdateWidget widget;
};

void tst_QWidget::initTestCase()
{
    widget.show();
    QTest::qWaitForWindowShown(&widget);
    QTest::qWait(300);
    processEvents();
}

void tst_QWidget::init()
{
    QVERIFY(widget.isVisible());
    for (int i = 0; i < 3; ++i)
        processEvents();
}

void tst_QWidget::update_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("numUpdates");
    QTest::addColumn<bool>("opaque");

    QTest::newRow("10x10x1 transparent")   << 10 << 10 << 1   << false;
    QTest::newRow("10x10x10 transparent")  << 10 << 10 << 10  << false;
    QTest::newRow("10x10x100 transparent") << 10 << 10 << 100 << false;
    QTest::newRow("10x10x1 opaque")        << 10 << 10 << 1   << true;
    QTest::newRow("10x10x10 opaque")       << 10 << 10 << 10  << true;
    QTest::newRow("10x10x100 opaque")      << 10 << 10 << 100 << true;
    QTest::newRow("25x25x1 transparent ")  << 25 << 25 << 1   << false;
    QTest::newRow("25x25x10 transparent")  << 25 << 25 << 10  << false;
    QTest::newRow("25x25x100 transparent") << 25 << 25 << 100 << false;
    QTest::newRow("25x25x1 opaque")        << 25 << 25 << 1   << true;
    QTest::newRow("25x25x10 opaque")       << 25 << 25 << 10  << true;
    QTest::newRow("25x25x100 opaque")      << 25 << 25 << 100 << true;
}

void tst_QWidget::update()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, numUpdates);
    QFETCH(bool, opaque);

    widget.fill(rows, columns);
    widget.setOpaqueChildren(opaque);

    QBENCHMARK {
        for (int i = 0; i < widget.children.size(); ++i) {
            for (int j = 0; j < numUpdates; ++j)
                widget.children.at(i)->update();
            QApplication::processEvents();
        }
    }

    QApplication::flush();
}

void tst_QWidget::updatePartial_data()
{
    update_data();
}

void tst_QWidget::updatePartial()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, numUpdates);
    QFETCH(bool, opaque);

    widget.fill(rows, columns);
    widget.setOpaqueChildren(opaque);

    QBENCHMARK {
        for (int i = 0; i < widget.children.size(); ++i) {
            QWidget *w = widget.children[i];
            const int x = w->width() / 2;
            const int y = w->height() / 2;
            for (int j = 0; j < numUpdates; ++j) {
                w->update(0, 0, x, y);
                w->update(x, 0, x, y);
                w->update(0, y, x, y);
                w->update(x, y, x, y);
            }
            QApplication::processEvents();
        }
    }
}

void tst_QWidget::updateComplex_data()
{
    update_data();
}

void tst_QWidget::updateComplex()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, numUpdates);
    QFETCH(bool, opaque);

    widget.fill(rows, columns);
    widget.setOpaqueChildren(opaque);

    QBENCHMARK {
        for (int i = 0; i < widget.children.size(); ++i) {
            QWidget *w = widget.children[i];
            const int x = w->width() / 2;
            const int y = w->height() / 2;
            QRegion r1(0, 0, x, y, QRegion::Ellipse);
            QRegion r2(x, y, x, y, QRegion::Ellipse);
            for (int j = 0; j < numUpdates; ++j) {
                w->update(r1);
                w->update(r2);
            }
            QApplication::processEvents();
        }
    }
}

QTEST_MAIN(tst_QWidget)

#include "tst_qwidget.moc"
