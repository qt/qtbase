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
#include <QByteArray>

class tst_Stylesheet : public QObject
{
    Q_OBJECT

public:
    tst_Stylesheet();

    QWidget *testWindow() const { return window; }
    void loadTestFiles();

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void tst_QToolButton_data();
    void tst_QToolButton();

private:
    void makeVisible();
    QImage takeSnapshot();
    QDir styleSheetDir;

    QWidget *window = nullptr;
};

tst_Stylesheet::tst_Stylesheet()
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
        appearanceStream << palette << font <<
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QApplication::style()->metaObject()->className();
#else
            QApplication::style()->name();
#endif
        const qreal screenDpr = QApplication::primaryScreen()->devicePixelRatio();
        if (screenDpr != 1.0)
            qWarning() << "DPR is" << screenDpr << "- images will be scaled";
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const quint16 appearanceId = qChecksum(appearanceBytes, appearanceBytes.size());
#else
    const quint16 appearanceId = qChecksum(appearanceBytes);
#endif

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

void tst_Stylesheet::initTestCase()
{
    QString baseDir = QFINDTESTDATA("qss/default.qss");
    styleSheetDir = QDir(QFileInfo(baseDir).path());

    // Check and setup the environment. Failure to do so skips the test.
    QByteArray msg;
    if (!QBaselineTest::connectToBaselineServer(&msg))
        QSKIP(msg);
}

void tst_Stylesheet::init()
{
    QFETCH(QString, styleSheet);

    QVERIFY(!window);
    window = new QWidget;
    window->setWindowTitle(QTest::currentDataTag());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    window->setScreen(QGuiApplication::primaryScreen());
#endif
    window->move(QGuiApplication::primaryScreen()->availableGeometry().topLeft());
    window->setStyleSheet(styleSheet);
}

void tst_Stylesheet::loadTestFiles()
{
    QTest::addColumn<QString>("styleSheet");

    QStringList qssFiles;
    // first add generic test files
    for (const auto &qssFile : styleSheetDir.entryList({QStringLiteral("*.qss")}, QDir::Files | QDir::Readable))
        qssFiles << styleSheetDir.absoluteFilePath(qssFile);

    // then test-function specific files
    const QString testFunction = QString(QTest::currentTestFunction()).remove("tst_").toLower();
    if (styleSheetDir.cd(testFunction)) {
        for (const auto &qssFile : styleSheetDir.entryList({QStringLiteral("*.qss")}, QDir::Files | QDir::Readable))
            qssFiles << styleSheetDir.absoluteFilePath(qssFile);
        styleSheetDir.cdUp();
    }

    for (const auto &qssFile : qssFiles) {
        QFileInfo fileInfo(qssFile);
        QFile file(qssFile);
        file.open(QFile::ReadOnly);
        QString styleSheet = QString::fromUtf8(file.readAll());
        QBaselineTest::newRow(fileInfo.baseName().toUtf8()) << styleSheet;
    }
}

void tst_Stylesheet::makeVisible()
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
QImage tst_Stylesheet::takeSnapshot()
{
    QGuiApplication::processEvents();
    QPixmap pm = window->grab();
    QTransform scaleTransform = QTransform::fromScale(1.0 / pm.devicePixelRatioF(), 1.0 / pm.devicePixelRatioF());
    return pm.toImage().transformed(scaleTransform, Qt::SmoothTransformation);
}

void tst_Stylesheet::cleanup()
{
    delete window;
    window = nullptr;
}

void tst_Stylesheet::tst_QToolButton_data()
{
    loadTestFiles();
}

void tst_Stylesheet::tst_QToolButton()
{
    const QIcon fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);

    QVBoxLayout *vbox = new QVBoxLayout;

    QHBoxLayout *normalButtons = new QHBoxLayout;
    for (const auto &buttonStyle : {Qt::ToolButtonIconOnly, Qt::ToolButtonTextOnly,
                                    Qt::ToolButtonTextUnderIcon, Qt::ToolButtonTextBesideIcon}) {
        QToolButton *normal = new QToolButton;
        normal->setToolButtonStyle(buttonStyle);
        normal->setText("Text");
        normal->setIcon(fileIcon);
        normalButtons->addWidget(normal);
    }
    vbox->addLayout(normalButtons);

    QHBoxLayout *arrowButtons = new QHBoxLayout;
    for (const auto &arrowType : {Qt::LeftArrow, Qt::RightArrow, Qt::UpArrow, Qt::DownArrow}) {
        QToolButton *arrow = new QToolButton;
        arrow->setText("Text");
        arrow->setArrowType(arrowType);
        arrowButtons->addWidget(arrow);
    }
    vbox->addLayout(arrowButtons);

    QHBoxLayout *arrowWithTextButtons = new QHBoxLayout;
    for (const auto &buttonStyle : {Qt::ToolButtonTextOnly,
                                    Qt::ToolButtonTextUnderIcon, Qt::ToolButtonTextBesideIcon}) {
        QToolButton *arrow = new QToolButton;
        arrow->setText("Text");
        arrow->setArrowType(Qt::UpArrow);
        arrow->setToolButtonStyle(buttonStyle);
        arrowWithTextButtons->addWidget(arrow);
    }
    vbox->addLayout(arrowWithTextButtons);

    QHBoxLayout *menuButtons = new QHBoxLayout;
    for (const auto &popupMode : {QToolButton::InstantPopup, QToolButton::MenuButtonPopup,
                                  QToolButton::DelayedPopup}) {
        QToolButton *menuButton = new QToolButton;
        menuButton->setText("Text");
        menuButton->setIcon(fileIcon);
        QMenu *menuButtonMenu = new QMenu;
        menuButtonMenu->addAction(QIcon(":/icons/align-left.png"), "Left");
        menuButtonMenu->addAction(QIcon(":/icons/align-right.png"), "Right");
        menuButtonMenu->addAction(QIcon(":/icons/align-center.png"), "Center");
        menuButton->setMenu(menuButtonMenu);
        menuButton->setPopupMode(popupMode);
        menuButtons->addWidget(menuButton);
    }
    vbox->addLayout(menuButtons);
    testWindow()->setLayout(vbox);

    makeVisible();
    QBASELINE_TEST(takeSnapshot());
}

#define main _realmain
QTEST_MAIN(tst_Stylesheet)
#undef main

int main(int argc, char *argv[])
{
    qSetGlobalQHashSeed(0);   // Avoid rendering variations caused by QHash randomization

    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_stylesheet.moc"
