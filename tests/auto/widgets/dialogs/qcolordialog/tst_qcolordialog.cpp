// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QtGui/QtGui>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QLineEdit>
#include <QSignalSpy>

QT_FORWARD_DECLARE_CLASS(QtTestEventThread)

class tst_QColorDialog : public QObject
{
    Q_OBJECT
public:
    tst_QColorDialog();
    virtual ~tst_QColorDialog();

public slots:
    void postKeyReturn();
    void testGetRgba();
    void testNativeActiveModalWidget();

private slots:
    void defaultOkButton();
    void native_activeModalWidget();
    void task247349_alpha();
    void QTBUG_43548_initialColor();
    void noCrashWhenParentIsDeleted();
    void hexColor_data();
    void hexColor();

    void hideNativeByDestruction();
};

class TestNativeDialog : public QColorDialog
{
    Q_OBJECT
public:
    QWidget *m_activeModalWidget;

    TestNativeDialog(QWidget *parent = nullptr)
        : QColorDialog(parent), m_activeModalWidget(0)
    {
        QTimer::singleShot(1, this, SLOT(test_activeModalWidgetSignal()));
    }

public slots:
    void test_activeModalWidgetSignal()
    {
        m_activeModalWidget = qApp->activeModalWidget();
    }
};

tst_QColorDialog::tst_QColorDialog()
{
}

tst_QColorDialog::~tst_QColorDialog()
{
}

void tst_QColorDialog::testNativeActiveModalWidget()
{
    // Check that QApplication::activeModalWidget retruns the
    // color dialog when it is executing, even when using a native
    // dialog:
#if defined(Q_OS_LINUX)
    QSKIP("This test crashes sometimes. Although rarely, but it happens. See QTBUG-50842.");
#endif
    TestNativeDialog d;
    QTimer::singleShot(1000, &d, SLOT(hide()));
    d.exec();
    QCOMPARE(&d, d.m_activeModalWidget);
}

void tst_QColorDialog::native_activeModalWidget()
{
    QTimer::singleShot(3000, qApp, SLOT(quit()));
    QTimer::singleShot(0, this, SLOT(testNativeActiveModalWidget()));
    qApp->exec();
}

void tst_QColorDialog::postKeyReturn() {
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i=0; i<list.size(); ++i) {
        QColorDialog *dialog = qobject_cast<QColorDialog *>(list[i]);
        if (dialog) {
            QTest::keyClick( list[i], Qt::Key_Return, Qt::NoModifier );
            return;
        }
    }
}

void tst_QColorDialog::testGetRgba()
{
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "Sending QTest::keyClick to OSX color dialog helper fails, see QTBUG-24320", Continue);
#endif
    QTimer::singleShot(500, this, &tst_QColorDialog::postKeyReturn);
    const QColor color = QColorDialog::getColor(QColor::fromRgba(0xffffffff), nullptr, QString(),
                                                QColorDialog::ShowAlphaChannel);
    QVERIFY(color.isValid());
}

void tst_QColorDialog::defaultOkButton()
{
#if defined(Q_OS_LINUX)
    QSKIP("This test crashes sometimes. Although rarely, but it happens. See QTBUG-50842.");
#endif
    QTimer::singleShot(4000, qApp, SLOT(quit()));
    QTimer::singleShot(0, this, SLOT(testGetRgba()));
    qApp->exec();
}

void tst_QColorDialog::task247349_alpha()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    int alpha = 0x17;
    dialog.setCurrentColor(QColor(0x01, 0x02, 0x03, alpha));
    QCOMPARE(alpha, dialog.currentColor().alpha());
    QCOMPARE(alpha, qAlpha(dialog.currentColor().rgba()));
}

void tst_QColorDialog::QTBUG_43548_initialColor()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.setCurrentColor(QColor(Qt::red));
    QColor a(Qt::red);
    QCOMPARE(a, dialog.currentColor());
}

void tst_QColorDialog::noCrashWhenParentIsDeleted()
{
    QPointer<QWidget> mainWindow = new QWidget();
    QTimer::singleShot(1000, mainWindow, [mainWindow]
                       { if (mainWindow.get()) mainWindow->deleteLater(); });
    const QColor color = QColorDialog::getColor(QColor::fromRgba(0xffffffff), mainWindow.get(),
                                                QString(), QColorDialog::DontUseNativeDialog);

    QVERIFY(!color.isValid());
    QVERIFY(!mainWindow.get());
}

void tst_QColorDialog::hexColor_data()
{
    QTest::addColumn<const QString>("colorString");
    QTest::addColumn<const QString>("expectedHexColor");
    QTest::addColumn<const int>("expectedSignalCount");

    QTest::newRow("White-#") << "#FFFFFE" << "#FFFFFE" << 1;
    QTest::newRow("White") << "FFFFFD" << "#FFFFFD" << 1;
    QTest::newRow("Blue-#") << "#77fffb" << "#77fffb" << 2;
    QTest::newRow("Blue") << "77fffa" << "#77fffa" << 2;
}

void tst_QColorDialog::hexColor()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    QLineEdit *lineEdit = dialog.findChild<QLineEdit *>("qt_colorname_lineedit",
                                                        Qt::FindChildrenRecursively);
    QVERIFY2(lineEdit, "QLineEdit for color not found. Adapt this test.");
    QVERIFY(lineEdit); // eliminate compiler warning

    QFETCH(const QString, colorString);
    QFETCH(const QString, expectedHexColor);
    QFETCH(const int, expectedSignalCount);

    QSignalSpy spy(&dialog, &QColorDialog::currentColorChanged);

    // Delete existing color
    lineEdit->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(lineEdit));
    for (int i = 0; i < 8; ++i)
        QTest::keyEvent(QTest::KeyAction::Click, lineEdit, Qt::Key_Backspace);
    QVERIFY(lineEdit->text().isEmpty());

    // Enter new color
    for (const QChar &key : colorString)
        QTest::keyEvent(QTest::KeyAction::Click, lineEdit, key.toLatin1());
    QCOMPARE(lineEdit->text().toLower(), expectedHexColor.toLower());

    // Consume all color change signals
    QTRY_COMPARE(spy.count(), expectedSignalCount);

    const QColor color = qvariant_cast<QColor>(spy.last().at(0));
    QCOMPARE(color.name(QColor::HexRgb), expectedHexColor.toLower());
}

void tst_QColorDialog::hideNativeByDestruction()
{
    QWidget window;
    QWidget *child = new QWidget(&window);
    QPointer<QColorDialog> dialog = new QColorDialog(child);
    // Make it application modal so that we don't end up with a sheet on macOS
    dialog->setWindowModality(Qt::ApplicationModal);
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    dialog->open();

    // We test that the dialog opens and closes by watching the activation of the
    // transient parent window. If it doesn't deactivate, then we have to skip.
    const auto windowActive = [&window]{ return window.isActiveWindow(); };
    const auto windowInactive = [&window]{ return !window.isActiveWindow(); };
    if (!QTest::qWaitFor(windowInactive, 2000))
        QSKIP("Dialog didn't activate");

    // This should destroy the dialog and close the native window
    child->deleteLater();
    QTRY_VERIFY(!dialog);
    // If the native window is still open, then the transient parent can't become
    // active
    window.activateWindow();
    QVERIFY(QTest::qWaitFor(windowActive));
}

QTEST_MAIN(tst_QColorDialog)
#include "tst_qcolordialog.moc"
