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

#include <qbaselinetest.h>
#include <QtWidgets>

class tst_Widgets : public QObject
{
    Q_OBJECT

public:
    tst_Widgets();

    void takeStandardSnapshots();
    QWidget *testWindow() const { return window; }

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void tst_QSlider_data();
    void tst_QSlider();

    void tst_QPushButton_data();
    void tst_QPushButton();

private:
    void makeVisible();
    QImage takeSnapshot();

    QWidget *window = nullptr;
};

tst_Widgets::tst_Widgets()
{
    QBaselineTest::addClientProperty("Project", "Widgets");

    // Set key platform properties that are relevant for the appearance of widgets
    const QString platformName = QGuiApplication::platformName() + "-" + QSysInfo::productType();
    QBaselineTest::addClientProperty("PlatformName", platformName);
    QBaselineTest::addClientProperty("OSVersion", QSysInfo::productVersion());

    // Encode a number of parameters that impact the UI
    QPalette palette;
    QFont font;
    QByteArray appearanceBytes;
    {
        QDataStream appearanceStream(&appearanceBytes, QIODevice::WriteOnly);
        appearanceStream << palette << font << QApplication::style()->name();
        const qreal screenDpr = QApplication::primaryScreen()->devicePixelRatio();
        if (screenDpr != 1.0)
            qWarning() << "DPR is" << screenDpr << "- images will be scaled";
    }
    const quint16 appearanceId = qChecksum(appearanceBytes);

    // Assume that text that's darker than the background means we run in light mode
    // This results in a more meaningful appearance ID between different runs than
    // just the checksum of the various attributes.
    const QColor windowColor = palette.window().color();
    const QColor textColor = palette.text().color();
    const QString appearanceIdString = (windowColor.value() > textColor.value()
                                        ? QString("light-%1") : QString("dark-%1"))
                                       .arg(appearanceId, 0, 16);
    QBaselineTest::addClientProperty("AppearanceID", appearanceIdString);

    // let users know where they can find the results
    qDebug() << "PlatformName computed to be:" << platformName;
    qDebug() << "Appearance ID computed as:" << appearanceIdString;
}

void tst_Widgets::initTestCase()
{
    // Check and setup the environment. Failure to do so skips the test.
    QByteArray msg;
    if (!QBaselineTest::connectToBaselineServer(&msg))
        QSKIP(msg);
}

void tst_Widgets::init()
{
    QVERIFY(!window);
    window = new QWidget;
    window->setWindowTitle(QTest::currentDataTag());
    window->setScreen(QGuiApplication::primaryScreen());
    window->move(QGuiApplication::primaryScreen()->availableGeometry().topLeft());
}

void tst_Widgets::makeVisible()
{
    window->show();
    window->window()->windowHandle()->requestActivate();
    // explicitly unset focus, the test needs to control when focus is shown
    if (window->focusWidget())
        window->focusWidget()->clearFocus();
    QVERIFY(QTest::qWaitForWindowActive(window));
}

/*
    Always return images scaled to a DPR of 1.0.

    This might produce some fuzzy differences, but lets us
    compare those.
*/
QImage tst_Widgets::takeSnapshot()
{
    QGuiApplication::processEvents();
    QPixmap pm = window->grab();
    QTransform scaleTransform = QTransform::fromScale(1.0 / pm.devicePixelRatioF(), 1.0 / pm.devicePixelRatioF());
    return pm.toImage().transformed(scaleTransform, Qt::SmoothTransformation);
}

/*!
    Sets standard widget properties on the test window and its children,
    and uploads snapshots. The widgets are returned in the same state
    that they had before.

    Call this helper after setting up the test window.
*/
void tst_Widgets::takeStandardSnapshots()
{
    makeVisible();
    struct PublicWidget : QWidget { friend tst_Widgets; };

    QBASELINE_CHECK(takeSnapshot(), "default");

    // try hard to set focus
    static_cast<PublicWidget*>(window)->focusNextPrevChild(true);
    if (!window->focusWidget()) {
        QWidget *firstChild = window->findChild<QWidget*>();
        if (firstChild)
            firstChild->setFocus();
    }
    if (window->focusWidget()) {
        QBASELINE_CHECK(takeSnapshot(), "focused");
        window->focusWidget()->clearFocus();
    }

    // this disables all children
    window->setEnabled(false);
    QBASELINE_CHECK(takeSnapshot(), "disabled");
    window->setEnabled(true);

    // show and activate another window so that our test window becomes inactive
    QWidget otherWindow;
    otherWindow.move(window->geometry().bottomRight() + QPoint(10, 10));
    otherWindow.resize(50, 50);
    otherWindow.setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    otherWindow.show();
    otherWindow.windowHandle()->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&otherWindow));
    QBASELINE_CHECK(takeSnapshot(), "inactive");

    window->windowHandle()->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    if (window->focusWidget())
        window->focusWidget()->clearFocus();
}

void tst_Widgets::cleanup()
{
    delete window;
    window = nullptr;
}

void tst_Widgets::tst_QSlider_data()
{
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<QSlider::TickPosition>("tickPosition");

    QBaselineTest::newRow("horizontal") << Qt::Horizontal << QSlider::NoTicks;
    QBaselineTest::newRow("horizontal ticks above") << Qt::Horizontal << QSlider::TicksAbove;
    QBaselineTest::newRow("horizontal ticks below") << Qt::Horizontal << QSlider::TicksBelow;
    QBaselineTest::newRow("horizontal ticks both") << Qt::Horizontal << QSlider::TicksBothSides;
    QBaselineTest::newRow("vertical") << Qt::Vertical << QSlider::NoTicks;
    QBaselineTest::newRow("vertical ticks left") << Qt::Vertical << QSlider::TicksLeft;
    QBaselineTest::newRow("vertical ticks right") << Qt::Vertical << QSlider::TicksRight;
    QBaselineTest::newRow("vertical ticks both") << Qt::Vertical << QSlider::TicksBothSides;
}

void tst_Widgets::tst_QSlider()
{
    struct PublicSlider : QSlider { friend tst_Widgets; };
    QFETCH(Qt::Orientation, orientation);
    QFETCH(QSlider::TickPosition, tickPosition);

    QBoxLayout *box = new QBoxLayout(orientation == Qt::Horizontal ? QBoxLayout::TopToBottom
                                                                   : QBoxLayout::LeftToRight);
    QList<QSlider*> _sliders;
    for (int i = 0; i < 3; ++i) {
        QSlider *slider = new QSlider;
        slider->setOrientation(orientation);
        slider->setTickPosition(tickPosition);
        _sliders << slider;
        box->addWidget(slider);
    }
    const auto sliders = _sliders;

    window->setLayout(box);

    // we want to see sliders with different values
    int value = 0;
    for (const auto &slider : sliders)
        slider->setValue(value += 33);

    takeStandardSnapshots();

    PublicSlider *slider = static_cast<PublicSlider*>(sliders.first());
    QStyleOptionSlider sliderOptions;
    slider->initStyleOption(&sliderOptions);
    const QRect handleRect = slider->style()->subControlRect(QStyle::CC_Slider, &sliderOptions,
                                                             QStyle::SubControl::SC_SliderHandle, slider);
    QTest::mousePress(slider, Qt::LeftButton, {}, handleRect.center());
    QBASELINE_CHECK(takeSnapshot(), "pressed");
    QTest::mouseRelease(slider, Qt::LeftButton, {}, handleRect.center());
    QBASELINE_CHECK(takeSnapshot(), "released");

    slider->setSliderDown(true);
    QBASELINE_CHECK(takeSnapshot(), "down");

    sliders.first()->setSliderDown(false);
    QBASELINE_CHECK(takeSnapshot(), "notdown");
}

void tst_Widgets::tst_QPushButton_data()
{
    QTest::addColumn<bool>("isFlat");

    QBaselineTest::newRow("normal") << false;
    QBaselineTest::newRow("flat") << true;
}

void tst_Widgets::tst_QPushButton()
{
    QFETCH(bool, isFlat);

    QVBoxLayout *vbox = new QVBoxLayout;
    QPushButton *testButton = new QPushButton("Ok");
    testButton->setFlat(isFlat);
    vbox->addWidget(testButton);

    testWindow()->setLayout(vbox);
    takeStandardSnapshots();

    testButton->setDown(true);
    QBASELINE_CHECK(takeSnapshot(), "down");
    testButton->setDown(false);
    QBASELINE_CHECK(takeSnapshot(), "up");
}


#define main _realmain
QTEST_MAIN(tst_Widgets)
#undef main

int main(int argc, char *argv[])
{
    qSetGlobalQHashSeed(0);   // Avoid rendering variations caused by QHash randomization

    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_widgets.moc"
