/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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


#include <QTest>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QApplication>

#include <private/qhighdpiscaling_p.h>

class tst_QWidgetRepaintManager : public QObject
{
    Q_OBJECT

public:
    tst_QWidgetRepaintManager();

public slots:
    void cleanup();

private slots:
    void moveWithOverlap();

private:
    const int m_fuzz;
};

tst_QWidgetRepaintManager::tst_QWidgetRepaintManager() :
     m_fuzz(int(QHighDpiScaling::factor(QGuiApplication::primaryScreen())))
{
}

void tst_QWidgetRepaintManager::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

/*!
    Verify that overlapping children are repainted correctly when
    a widget is moved (via a scroll area) for such a distance that
    none of the old area is still visible. QTBUG-26269
*/
void tst_QWidgetRepaintManager::moveWithOverlap()
{
    if (QStringList{"android"}.contains(QGuiApplication::platformName()))
        QSKIP("This test fails on Android");

    class MainWindow : public QWidget
    {
    public:
        MainWindow(QWidget *parent = 0)
            : QWidget(parent, Qt::WindowStaysOnTopHint)
        {
            m_scrollArea = new QScrollArea(this);
            QWidget *w = new QWidget;
            w->setPalette(QPalette(Qt::gray));
            w->setAutoFillBackground(true);
            m_scrollArea->setWidget(w);
            m_scrollArea->resize(500, 100);
            w->resize(5000, 600);

            m_topWidget = new QWidget(this);
            m_topWidget->setPalette(QPalette(Qt::red));
            m_topWidget->setAutoFillBackground(true);
            m_topWidget->resize(300, 200);
        }

        void resizeEvent(QResizeEvent *e) override
        {
            QWidget::resizeEvent(e);
            // move scroll area and top widget to the center of the main window
            scrollArea()->move((width() - scrollArea()->width()) / 2, (height() - scrollArea()->height()) / 2);
            topWidget()->move((width() - topWidget()->width()) / 2, (height() - topWidget()->height()) / 2);
        }


        inline QScrollArea *scrollArea() const { return m_scrollArea; }
        inline QWidget *topWidget() const { return m_topWidget; }

        bool grabWidgetBackground(QWidget *w)
        {
            // To check widget's background we should compare two screenshots:
            // the first one is taken by system tools through QScreen::grabWindow(),
            // the second one is taken by Qt rendering to a pixmap via QWidget::grab().

            QScreen *screen = w->screen();
            const QRect screenGeometry = screen->geometry();
            QPoint globalPos = w->mapToGlobal(QPoint(0, 0));
            if (globalPos.x() >= screenGeometry.width())
                globalPos.rx() -= screenGeometry.x();
            if (globalPos.y() >= screenGeometry.height())
                globalPos.ry() -= screenGeometry.y();

            return QTest::qWaitFor([&]{
                QImage systemScreenshot = screen->grabWindow(winId(),
                                                             globalPos.x(), globalPos.y(),
                                                             w->width(), w->height()).toImage();
                systemScreenshot = systemScreenshot.convertToFormat(QImage::Format_RGB32);
                QImage qtScreenshot = w->grab().toImage().convertToFormat(systemScreenshot.format());
                return systemScreenshot == qtScreenshot;
            });
        };

    private:
        QScrollArea *m_scrollArea;
        QWidget *m_topWidget;
    };

    MainWindow w;
    w.showFullScreen();

    QVERIFY(QTest::qWaitForWindowActive(&w));

    bool result = w.grabWidgetBackground(w.topWidget());
    // if this fails already, then the system we test on can't compare screenshots from grabbed widgets,
    // and we have to skip this test. Possible reasons are that showing the window took too long, differences
    // in surface formats, or unrelated bugs in QScreen::grabWindow.
    if (!result)
        QSKIP("Cannot compare QWidget::grab with QScreen::grabWindow on this machine");

    // scroll the horizontal slider to the right side
    {
        w.scrollArea()->horizontalScrollBar()->setValue(w.scrollArea()->horizontalScrollBar()->maximum());
        QVERIFY(w.grabWidgetBackground(w.topWidget()));
    }

    // scroll the vertical slider down
    {
        w.scrollArea()->verticalScrollBar()->setValue(w.scrollArea()->verticalScrollBar()->maximum());
        QVERIFY(w.grabWidgetBackground(w.topWidget()));
    }

    // hide the top widget
    {
        w.topWidget()->hide();
        QVERIFY(w.grabWidgetBackground(w.scrollArea()->viewport()));
    }

    // scroll the horizontal slider to the left side
    {
        w.scrollArea()->horizontalScrollBar()->setValue(w.scrollArea()->horizontalScrollBar()->minimum());
        QVERIFY(w.grabWidgetBackground(w.scrollArea()->viewport()));
    }

    // scroll the vertical slider up
    {
        w.scrollArea()->verticalScrollBar()->setValue(w.scrollArea()->verticalScrollBar()->minimum());
        QVERIFY(w.grabWidgetBackground(w.scrollArea()->viewport()));
    }
}

QTEST_MAIN(tst_QWidgetRepaintManager)
#include "tst_qwidgetrepaintmanager.moc"
