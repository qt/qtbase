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


#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGestureEvent>
#include <QtGui/QScreen>
#include <QtGui/QTouchDevice>
#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QDebug>

class tst_QGestureRecognizer : public QObject
{
    Q_OBJECT
public:
    tst_QGestureRecognizer();

private Q_SLOTS:
    void initTestCase();
#ifndef QT_NO_GESTURES
    void panGesture_data();
    void panGesture();
    void pinchGesture_data();
    void pinchGesture();
    void swipeGesture_data();
    void swipeGesture();
#endif // !QT_NO_GESTURES

private:
    const int m_fingerDistance;
    QTouchDevice *m_touchDevice;
};

tst_QGestureRecognizer::tst_QGestureRecognizer()
    : m_fingerDistance(qRound(QGuiApplication::primaryScreen()->physicalDotsPerInch() / 2.0))
    , m_touchDevice(QTest::createTouchDevice())
{
    qputenv("QT_PAN_TOUCHPOINTS", "2"); // Prevent device detection of pan touch point count.
}

void tst_QGestureRecognizer::initTestCase()
{
}

#ifndef QT_NO_GESTURES

typedef QVector<Qt::GestureType> GestureTypeVector;

class TestWidget : public QWidget
{
public:
    explicit TestWidget(const GestureTypeVector &gestureTypes);

    bool gestureReceived(Qt::GestureType gestureType) const
        { return m_receivedGestures.value(gestureType); }

protected:
    bool event(QEvent * event) Q_DECL_OVERRIDE;

private:
    typedef QHash<Qt::GestureType, bool> GestureTypeHash;
    GestureTypeHash m_receivedGestures;
};

TestWidget::TestWidget(const GestureTypeVector &gestureTypes)
{
    setAttribute(Qt::WA_AcceptTouchEvents);

    foreach (Qt::GestureType gestureType, gestureTypes) {
        grabGesture(gestureType);
        m_receivedGestures.insert(gestureType, false);
    }

    const QRect geometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = geometry.size() / 2;
    resize(size);
    move(geometry.center() - QPoint(size.width() / 2, size.height() / 2));
}

bool TestWidget::event(QEvent * event)
{
    switch (event->type()) {
    case QEvent::Gesture: {
        const QGestureEvent *gestureEvent = static_cast<QGestureEvent *>(event);
        const GestureTypeHash::iterator hend = m_receivedGestures.end();
        for (GestureTypeHash::iterator it = m_receivedGestures.begin(); it != hend; ++it) {
            if (const QGesture *gesture = gestureEvent->gesture(it.key())) {
                if (gesture->state() == Qt::GestureFinished)
                    it.value() = true;
            }
        }
    }
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

static void pressSequence(QTest::QTouchEventSequence &sequence,
                          QVector<QPoint> &points,
                          QWidget *widget)
{
    const int pointCount = points.size();
    for (int p = 0; p < pointCount; ++p)
        sequence.press(p, points.at(p), widget);
    sequence.commit();
}

static void linearSequence(int n, const QPoint &delta,
                           QTest::QTouchEventSequence &sequence,
                           QVector<QPoint> &points,
                           QWidget *widget)
{
    const int pointCount = points.size();
    for (int s = 0; s < n; ++s) {
        for (int p = 0; p < pointCount; ++p) {
            points[p] += delta;
            sequence.move(p, points[p], widget);
        }
        sequence.commit();
    }
}

static void releaseSequence(QTest::QTouchEventSequence &sequence,
                            QVector<QPoint> &points,
                            QWidget *widget)
{
    const int pointCount = points.size();
    for (int p = 0; p < pointCount; ++p)
        sequence.release(p, points[p], widget);
    sequence.commit();
}

// --- Pan

enum PanSubTest {
    TwoFingerPanSubTest
};

void tst_QGestureRecognizer::panGesture_data()
{
    QTest::addColumn<int>("panSubTest");
    QTest::addColumn<bool>("gestureExpected");
    QTest::newRow("Two finger") << int(TwoFingerPanSubTest) << true;
}

void tst_QGestureRecognizer::panGesture()
{
    QFETCH(int, panSubTest);
    QFETCH(bool, gestureExpected);

    Q_UNUSED(panSubTest) // Single finger pan will be added later.

    const int panPoints = 2;
    const Qt::GestureType gestureType = Qt::PanGesture;
    TestWidget widget(GestureTypeVector(1, gestureType));
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QVector<QPoint> points;
    for (int i = 0; i < panPoints; ++i)
        points.append(QPoint(10 + i *20, 10 + i *20));

    QTest::QTouchEventSequence panSequence = QTest::touchEvent(&widget, m_touchDevice);
    pressSequence(panSequence, points, &widget);
    linearSequence(5, QPoint(20, 20), panSequence, points, &widget);
    releaseSequence(panSequence, points, &widget);

    if (gestureExpected) {
        QTRY_VERIFY(widget.gestureReceived(gestureType));
    } else {
        QCoreApplication::processEvents();
        QVERIFY(!widget.gestureReceived(gestureType));
    }
}

// --- Pinch

enum PinchSubTest {
    StandardPinchSubTest
};

void tst_QGestureRecognizer::pinchGesture_data()
{
    QTest::addColumn<int>("pinchSubTest");
    QTest::addColumn<bool>("gestureExpected");
    QTest::newRow("Standard") << int(StandardPinchSubTest) << true;
}

void tst_QGestureRecognizer::pinchGesture()
{
    QFETCH(int, pinchSubTest);
    QFETCH(bool, gestureExpected);

    Q_UNUSED(pinchSubTest)

    const Qt::GestureType gestureType = Qt::PinchGesture;
    TestWidget widget(GestureTypeVector(1, gestureType));
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QVector<QPoint> points;
    points.append(widget.rect().center());
    points.append(points.front() + QPoint(0, 20));

    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&widget, m_touchDevice);
    pressSequence(pinchSequence, points, &widget);

    for (int s = 0; s < 5; ++s) {
        points[0] += QPoint(5, 30);
        pinchSequence.move(0, points[0], &widget);
        points[1] += QPoint(5, -30);
        pinchSequence.move(1, points[1], &widget);
        pinchSequence.commit();
    }

    releaseSequence(pinchSequence, points, &widget);

    if (gestureExpected) {
        QTRY_VERIFY(widget.gestureReceived(gestureType));
    } else {
        QCoreApplication::processEvents();
        QVERIFY(!widget.gestureReceived(gestureType));
    }
}

// --- Swipe

enum SwipeSubTest {
    SwipeLineSubTest,
    SwipeDirectionChangeSubTest,
    SwipeSmallDirectionChangeSubTest
};

void tst_QGestureRecognizer::swipeGesture_data()
{
    QTest::addColumn<int>("swipeSubTest");
    QTest::addColumn<bool>("gestureExpected");
    QTest::newRow("Line") << int(SwipeLineSubTest) << true;
    QTest::newRow("DirectionChange") << int(SwipeDirectionChangeSubTest) << false;
    QTest::newRow("SmallDirectionChange") << int(SwipeSmallDirectionChangeSubTest) << true;
}

void tst_QGestureRecognizer::swipeGesture()
{
    enum { swipePoints = 3 };

    QFETCH(int, swipeSubTest);
    QFETCH(bool, gestureExpected);

    const Qt::GestureType gestureType = Qt::SwipeGesture;
    TestWidget widget(GestureTypeVector(1, gestureType));
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    // Start a swipe sequence with 2 points (QTBUG-15768)
    const QPoint fingerDistance(m_fingerDistance, m_fingerDistance);
    QVector<QPoint> points;
    for (int i = 0; i < swipePoints - 1; ++i)
        points.append(fingerDistance + i * fingerDistance);

    QTest::QTouchEventSequence swipeSequence = QTest::touchEvent(&widget, m_touchDevice);
    pressSequence(swipeSequence, points, &widget);

    // Press point #3
    points.append(points.last() + fingerDistance);
    swipeSequence.press(points.size() - 1, points.last(), &widget);
    swipeSequence.commit();
    Q_ASSERT(points.size() == swipePoints);

    // Move.
    const QPoint moveDelta(60, 20);
    switch (swipeSubTest) {
    case SwipeLineSubTest:
        linearSequence(5, moveDelta, swipeSequence, points, &widget);
        break;
    case SwipeDirectionChangeSubTest:
        linearSequence(5, moveDelta, swipeSequence, points, &widget);
        linearSequence(3, QPoint(-moveDelta.x(), moveDelta.y()), swipeSequence, points, &widget);
        break;
    case SwipeSmallDirectionChangeSubTest: { // QTBUG-46195, small changes in direction should not cause the gesture to be canceled.
        const QPoint smallChangeMoveDelta(50, 1);
        linearSequence(5, smallChangeMoveDelta, swipeSequence, points, &widget);
        linearSequence(1, QPoint(smallChangeMoveDelta.x(), -3), swipeSequence, points, &widget);
        linearSequence(5, smallChangeMoveDelta, swipeSequence, points, &widget);
    }
        break;
    }

    releaseSequence(swipeSequence, points, &widget);

    if (gestureExpected) {
        QTRY_VERIFY(widget.gestureReceived(gestureType));
    } else {
        QCoreApplication::processEvents();
        QVERIFY(!widget.gestureReceived(gestureType));
    }
}

#endif // !QT_NO_GESTURES

QTEST_MAIN(tst_QGestureRecognizer)

#include "tst_qgesturerecognizer.moc"
