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
#include <QtTest/QtTest>
#include <QMessageBox>
#include <QDebug>
#include <QPair>
#include <QList>
#include <QPointer>
#include <QTimer>
#include <QApplication>
#include <QPushButton>
#include <QDialogButtonBox>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>

#define CONVENIENCE_FUNC_SYMS(func) \
    { \
        int x1 = QMessageBox::func(0, "Foo", "Bar"); \
        int x3 = QMessageBox::func(0, "Foo", "Bar", "Save"); \
        int x6 = QMessageBox::func(0, "Foo", "Bar", "Save", "Save As"); \
        int x7 = QMessageBox::func(0, "Foo", "Bar", "Save", "Save As", "Dont Save"); \
        int x8 = QMessageBox::func(0, "Foo", "Bar", "Save", "Save As", "Dont Save", 1); \
        int x9 = QMessageBox::func(0, "Foo", "Bar", "Save", "Save As", "Dont Save", 1, 2); \
        int x10 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::YesAll, QMessageBox::Yes); \
        int x11 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::YesAll, QMessageBox::Yes, \
                                    QMessageBox::No); \
        qDebug("%d %d %d %d %d %d %d %d", x1, x3, x6, x7, x8, x9, x10, x11); \
        { \
        int x4 = QMessageBox::func(0, "Foo", "Bar", (int)QMessageBox::Yes, (int)QMessageBox::No); \
        int x5 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes, (int)QMessageBox::No); \
        int x6 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes | QMessageBox::Default, (int)QMessageBox::No); \
        int x7 = QMessageBox::func(0, "Foo", "Bar", (int)QMessageBox::Yes, QMessageBox::No); \
        int x8 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes, QMessageBox::No); \
        int x9 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No); \
        int x10 = QMessageBox::func(0, "Foo", "Bar", (int)QMessageBox::Yes, (int)QMessageBox::No, (int)QMessageBox::Ok); \
        int x11 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes, (int)QMessageBox::No, (int)QMessageBox::Ok); \
        int x12 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes | QMessageBox::Default, (int)QMessageBox::No, (int)QMessageBox::Ok); \
        int x13 = QMessageBox::func(0, "Foo", "Bar", (int)QMessageBox::Yes, QMessageBox::No, (int)QMessageBox::Ok); \
        int x14 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes, QMessageBox::No, (int)QMessageBox::Ok); \
        int x15 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, (int)QMessageBox::Ok); \
        int x16 = QMessageBox::func(0, "Foo", "Bar", (int)QMessageBox::Yes, (int)QMessageBox::No, QMessageBox::Ok); \
        int x17 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes, (int)QMessageBox::No, QMessageBox::Ok); \
        int x18 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes | QMessageBox::Default, (int)QMessageBox::No, QMessageBox::Ok); \
        int x19 = QMessageBox::func(0, "Foo", "Bar", (int)QMessageBox::Yes, QMessageBox::No, QMessageBox::Ok); \
        int x20 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes, QMessageBox::No, QMessageBox::Ok); \
        int x21 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Ok); \
        qDebug("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21); \
        } \
    }

#define CONVENIENCE_FUNC_SYMS_EXTRA(func) \
    { \
        int x1 = QMessageBox::func(0, "Foo", "Bar", (int)QMessageBox::Yes); \
        int x2 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes); \
        int x3 = QMessageBox::func(0, "Foo", "Bar", QMessageBox::Yes | QMessageBox::Default); \
        qDebug("%d %d %d", x1, x2, x3); \
    }

class tst_QMessageBox : public QObject
{
    Q_OBJECT

private slots:
    void sanityTest();
    void defaultButton();
    void escapeButton();
    void button();
    void statics();
    void about();
    void detailsText();
    void detailsButtonText();
    void expandDetails_QTBUG_32473();

#ifndef Q_OS_MAC
    void shortcut();
#endif

    void staticSourceCompat();
    void instanceSourceCompat();

    void testSymbols();
    void incorrectDefaultButton();
    void updateSize();

    void setInformativeText();
    void iconPixmap();

    void cleanup();
};

class tst_ResizingMessageBox : public QMessageBox
{
public:
    tst_ResizingMessageBox() : QMessageBox(), resized(false) { }
    bool resized;

protected:
    void resizeEvent ( QResizeEvent * event ) {
        resized = true;
        QMessageBox::resizeEvent(event);
    }
};

// ExecCloseHelper: Closes a modal QDialog during its exec() function by either
// sending a key event or closing it (CloseWindow) once it becomes the active
// modal window. Pass nullptr to "autodetect" the instance for static methods.
class ExecCloseHelper : public QObject
{
public:
    enum { CloseWindow = -1 };

    explicit ExecCloseHelper(QObject *parent = Q_NULLPTR)
        : QObject(parent), m_key(0), m_timerId(0), m_testCandidate(Q_NULLPTR) { }

    void start(int key, QWidget *testCandidate = Q_NULLPTR)
    {
        m_key = key;
        m_testCandidate = testCandidate;
        m_timerId = startTimer(50);
    }

    bool done() const { return !m_timerId; }

protected:
    void timerEvent(QTimerEvent *te) Q_DECL_OVERRIDE;

private:
    int m_key;
    int m_timerId;
    QWidget *m_testCandidate;
};

void ExecCloseHelper::timerEvent(QTimerEvent *te)
{
    if (te->timerId() != m_timerId)
        return;

    QWidget *modalWidget = QApplication::activeModalWidget();

    if (!m_testCandidate && modalWidget)
        m_testCandidate = modalWidget;

    if (m_testCandidate && m_testCandidate == modalWidget) {
        if (m_key == CloseWindow) {
            m_testCandidate->close();
        } else {
            QKeyEvent *ke = new QKeyEvent(QEvent::KeyPress, m_key, Qt::NoModifier);
            QCoreApplication::postEvent(m_testCandidate, ke);
        }
        m_testCandidate = Q_NULLPTR;
        killTimer(m_timerId);
        m_timerId = m_key = 0;
    }
}

void tst_QMessageBox::cleanup()
{
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty()); // OS X requires TRY
}

void tst_QMessageBox::sanityTest()
{
    QMessageBox msgBox;
    msgBox.setText("This is insane");
    for (int i = 0; i < 10; i++)
        msgBox.setIcon(QMessageBox::Icon(i));
    msgBox.setIconPixmap(QPixmap());
    msgBox.setIconPixmap(QPixmap("whatever.png"));
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setTextFormat(Qt::PlainText);
    ExecCloseHelper closeHelper;
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox);
    msgBox.exec();
}

void tst_QMessageBox::button()
{
    QMessageBox msgBox;
    msgBox.addButton("retry", QMessageBox::DestructiveRole);
    QVERIFY(msgBox.button(QMessageBox::Ok) == 0); // not added yet
    QPushButton *b1 = msgBox.addButton(QMessageBox::Ok);
    QCOMPARE(msgBox.button(QMessageBox::Ok), (QAbstractButton *)b1);  // just added
    QCOMPARE(msgBox.standardButton(b1), QMessageBox::Ok);
    msgBox.addButton(QMessageBox::Cancel);
    QCOMPARE(msgBox.standardButtons(), QMessageBox::Ok | QMessageBox::Cancel);

    // remove the cancel, should not exist anymore
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    QVERIFY(!msgBox.button(QMessageBox::Cancel));
    QVERIFY(msgBox.button(QMessageBox::Yes) != 0);

    // should not crash
    QPushButton *b4 = new QPushButton;
    msgBox.addButton(b4, QMessageBox::DestructiveRole);
    msgBox.addButton(0, QMessageBox::ActionRole);
}

void tst_QMessageBox::defaultButton()
{
    QMessageBox msgBox;
    QVERIFY(!msgBox.defaultButton());
    msgBox.addButton(QMessageBox::Ok);
    msgBox.addButton(QMessageBox::Cancel);
    QVERIFY(!msgBox.defaultButton());
    QPushButton pushButton;
    msgBox.setDefaultButton(&pushButton);
    QVERIFY(msgBox.defaultButton() == 0); // we have not added it yet
    QPushButton *retryButton = msgBox.addButton(QMessageBox::Retry);
    msgBox.setDefaultButton(retryButton);
    QCOMPARE(msgBox.defaultButton(), retryButton);
    ExecCloseHelper closeHelper;
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), msgBox.button(QMessageBox::Cancel));

    closeHelper.start(Qt::Key_Enter, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), (QAbstractButton *)retryButton);

    QAbstractButton *okButton = msgBox.button(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    QCOMPARE(msgBox.defaultButton(), (QPushButton *)okButton);
    closeHelper.start(Qt::Key_Enter, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), okButton);
    msgBox.setDefaultButton(QMessageBox::Yes); // its not in there!
    QCOMPARE(msgBox.defaultButton(), okButton);
    msgBox.removeButton(okButton);
    delete okButton;
    okButton = 0;
    QVERIFY(!msgBox.defaultButton());
    msgBox.setDefaultButton(QMessageBox::Ok);
    QVERIFY(!msgBox.defaultButton());
}

void tst_QMessageBox::escapeButton()
{
    QMessageBox msgBox;
    QVERIFY(!msgBox.escapeButton());
    msgBox.addButton(QMessageBox::Ok);
    ExecCloseHelper closeHelper;
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox);
    msgBox.exec();
    QVERIFY(msgBox.clickedButton() == msgBox.button(QMessageBox::Ok)); // auto detected (one button only)
    msgBox.addButton(QMessageBox::Cancel);
    QVERIFY(!msgBox.escapeButton());
    QPushButton invalidButton;
    msgBox.setEscapeButton(&invalidButton);
    QVERIFY(!msgBox.escapeButton());
    QAbstractButton *retryButton = msgBox.addButton(QMessageBox::Retry);

    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox);
    msgBox.exec();
    QVERIFY(msgBox.clickedButton() == msgBox.button(QMessageBox::Cancel)); // auto detected (cancel)

    msgBox.setEscapeButton(retryButton);
    QCOMPARE(msgBox.escapeButton(), (QAbstractButton *)retryButton);

    // with escape
    closeHelper.start(Qt::Key_Escape, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), retryButton);

    // with close
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), (QAbstractButton *)retryButton);

    QAbstractButton *okButton = msgBox.button(QMessageBox::Ok);
    msgBox.setEscapeButton(QMessageBox::Ok);
    QCOMPARE(msgBox.escapeButton(), okButton);
    closeHelper.start(Qt::Key_Escape, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), okButton);
    msgBox.setEscapeButton(QMessageBox::Yes); // its not in there!
    QCOMPARE(msgBox.escapeButton(), okButton);
    msgBox.removeButton(okButton);
    delete okButton;
    okButton = 0;
    QVERIFY(!msgBox.escapeButton());
    msgBox.setEscapeButton(QMessageBox::Ok);
    QVERIFY(!msgBox.escapeButton());

    QMessageBox msgBox2;
    msgBox2.addButton(QMessageBox::Yes);
    msgBox2.addButton(QMessageBox::No);
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox2);
    msgBox2.exec();
    QVERIFY(msgBox2.clickedButton() == msgBox2.button(QMessageBox::No)); // auto detected (one No button only)

    QPushButton *rejectButton = new QPushButton;
    msgBox2.addButton(rejectButton, QMessageBox::RejectRole);
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox2);
    msgBox2.exec();
    QVERIFY(msgBox2.clickedButton() == rejectButton); // auto detected (one reject button only)

    msgBox2.addButton(new QPushButton, QMessageBox::RejectRole);
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox2);
    msgBox2.exec();
    QVERIFY(msgBox2.clickedButton() == msgBox2.button(QMessageBox::No)); // auto detected (one No button only)
}

void tst_QMessageBox::statics()
{
    QMessageBox::StandardButton (*statics[4])(QWidget *, const QString &,
         const QString&, QMessageBox::StandardButtons buttons,
         QMessageBox::StandardButton);

    statics[0] = QMessageBox::information;
    statics[1] = QMessageBox::critical;
    statics[2] = QMessageBox::question;
    statics[3] = QMessageBox::warning;

    ExecCloseHelper closeHelper;
    for (int i = 0; i < 4; i++) {
        closeHelper.start(Qt::Key_Escape);
        QMessageBox::StandardButton sb = (*statics[i])(0, "caption",
            "text", QMessageBox::Yes | QMessageBox::No | QMessageBox::Help | QMessageBox::Cancel,
           QMessageBox::NoButton);
        QCOMPARE(sb, QMessageBox::Cancel);
        QVERIFY(closeHelper.done());

        closeHelper.start(ExecCloseHelper::CloseWindow);
        sb = (*statics[i])(0, "caption",
           "text", QMessageBox::Yes | QMessageBox::No | QMessageBox::Help | QMessageBox::Cancel,
           QMessageBox::NoButton);
        QCOMPARE(sb, QMessageBox::Cancel);
        QVERIFY(closeHelper.done());

        closeHelper.start(Qt::Key_Enter);
        sb = (*statics[i])(0, "caption",
           "text", QMessageBox::Yes | QMessageBox::No | QMessageBox::Help,
           QMessageBox::Yes);
        QCOMPARE(sb, QMessageBox::Yes);
        QVERIFY(closeHelper.done());

        closeHelper.start(Qt::Key_Enter);
        sb = (*statics[i])(0, "caption",
           "text", QMessageBox::Yes | QMessageBox::No | QMessageBox::Help,
            QMessageBox::No);
        QCOMPARE(sb, QMessageBox::No);
        QVERIFY(closeHelper.done());
    }
}

// shortcuts are not used on OS X
#ifndef Q_OS_MAC
void tst_QMessageBox::shortcut()
{
    QMessageBox msgBox;
    msgBox.addButton("O&k", QMessageBox::YesRole);
    msgBox.addButton("&No", QMessageBox::YesRole);
    msgBox.addButton("&Maybe", QMessageBox::YesRole);
    ExecCloseHelper closeHelper;
    closeHelper.start(Qt::Key_M, &msgBox);
    QCOMPARE(msgBox.exec(), 2);
}
#endif

void tst_QMessageBox::about()
{
    ExecCloseHelper closeHelper;
    closeHelper.start(Qt::Key_Escape);
    QMessageBox::about(0, "Caption", "This is an auto test");
    // On Mac, about and aboutQt are not modal, so we need to
    // explicitly run the event loop
#ifdef Q_OS_MAC
    QTRY_VERIFY(closeHelper.done());
#else
    QVERIFY(closeHelper.done());
#endif

#if !defined(Q_OS_WINCE)
    const int keyToSend = Qt::Key_Enter;
#else
    const keyToSend = Qt::Key_Escape;
#endif
    closeHelper.start(keyToSend);
    QMessageBox::aboutQt(0, "Caption");
#ifdef Q_OS_MAC
    QTRY_VERIFY(closeHelper.done());
#else
    QVERIFY(closeHelper.done());
#endif
}

void tst_QMessageBox::staticSourceCompat()
{
    int ret;

    // source compat tests for < 4.2
    ExecCloseHelper closeHelper;
    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(0, "title", "text", QMessageBox::Yes, QMessageBox::No);
    int expectedButton = int(QMessageBox::Yes);
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const int dialogButtonBoxLayout = theme->themeHint(QPlatformTheme::DialogButtonBoxLayout).toInt();
        if (dialogButtonBoxLayout == QDialogButtonBox::MacLayout
            || dialogButtonBoxLayout == QDialogButtonBox::GnomeLayout)
            expectedButton = int(QMessageBox::No);
    }
    QCOMPARE(ret, expectedButton);
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(0, "title", "text", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);
    QCOMPARE(ret, int(QMessageBox::Yes));
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(0, "title", "text", QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    QCOMPARE(ret, int(QMessageBox::No));
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(0, "title", "text", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape);
    QCOMPARE(ret, int(QMessageBox::Yes));
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(0, "title", "text", QMessageBox::Yes | QMessageBox::Escape, QMessageBox::No | QMessageBox::Default);
    QCOMPARE(ret, int(QMessageBox::No));
    QVERIFY(closeHelper.done());

    // the button text versions
    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(0, "title", "text", "Yes", "No", QString(), 1);
    QCOMPARE(ret, 1);
    QVERIFY(closeHelper.done());

    if (0) { // don't run these tests since the dialog won't close!
        closeHelper.start(Qt::Key_Escape);
        ret = QMessageBox::information(0, "title", "text", "Yes", "No", QString(), 1);
        QCOMPARE(ret, -1);
        QVERIFY(closeHelper.done());

        closeHelper.start(Qt::Key_Escape);
        ret = QMessageBox::information(0, "title", "text", "Yes", "No", QString(), 0, 1);
        QCOMPARE(ret, 1);
        QVERIFY(closeHelper.done());
    }
}

void tst_QMessageBox::instanceSourceCompat()
{
     QMessageBox mb("Application name here",
                    "Saving the file will overwrite the original file on the disk.\n"
                    "Do you really want to save?",
                    QMessageBox::Information,
                    QMessageBox::Yes | QMessageBox::Default,
                    QMessageBox::No,
                    QMessageBox::Cancel | QMessageBox::Escape);
    mb.setButtonText(QMessageBox::Yes, "Save");
    mb.setButtonText(QMessageBox::No, "Discard");
    mb.addButton("&Revert", QMessageBox::RejectRole);
    mb.addButton("&Zoo", QMessageBox::ActionRole);

    ExecCloseHelper closeHelper;
    closeHelper.start(Qt::Key_Enter, &mb);
    QCOMPARE(mb.exec(), int(QMessageBox::Yes));
    closeHelper.start(Qt::Key_Escape, &mb);
    QCOMPARE(mb.exec(), int(QMessageBox::Cancel));
#ifndef Q_OS_MAC
    // mnemonics are not used on OS X
    closeHelper.start(Qt::ALT + Qt::Key_R, &mb);
    QCOMPARE(mb.exec(), 0);
    closeHelper.start(Qt::ALT + Qt::Key_Z, &mb);
    QCOMPARE(mb.exec(), 1);
#endif
}

void tst_QMessageBox::testSymbols()
{
    return;

    QMessageBox::Icon icon;
    icon = QMessageBox::NoIcon;
    icon = QMessageBox::Information;
    icon = QMessageBox::Warning;
    icon = QMessageBox::Critical;
    icon = QMessageBox::Question;

    QMessageBox mb1;
    QMessageBox mb2(0);
    QMessageBox mb3(&mb1);
    QMessageBox mb3b("title", "text", QMessageBox::Critical, int(QMessageBox::Yes),
                     int(QMessageBox::No), int(QMessageBox::Cancel), &mb1, Qt::Dialog);

    QMessageBox::Button button = QMessageBox::NoButton;
    button = QMessageBox::Ok;
    button = QMessageBox::Cancel;
    button = QMessageBox::Yes;
    button = QMessageBox::No;
    button = QMessageBox::Abort;
    button = QMessageBox::Retry;
    button = QMessageBox::Ignore;
    button = QMessageBox::YesAll;
    button = QMessageBox::NoAll;
    button = QMessageBox::ButtonMask;
    button = QMessageBox::Default;
    button = QMessageBox::Escape;
    button = QMessageBox::FlagMask;
    QVERIFY(button);

    const QString text = QStringLiteral("Foo");
    mb1.setText(text);
    QCOMPARE(mb1.text(), text);

    icon = mb1.icon();
    QCOMPARE(icon, QMessageBox::NoIcon);
    mb1.setIcon(QMessageBox::Question);
    QCOMPARE(mb1.icon(), QMessageBox::Question);

    QPixmap iconPixmap = mb1.iconPixmap();
    mb1.setIconPixmap(iconPixmap);
    QCOMPARE(mb1.icon(), QMessageBox::NoIcon);

    QCOMPARE(mb1.buttonText(QMessageBox::Ok), QLatin1String("OK"));
    QCOMPARE(mb1.buttonText(QMessageBox::Cancel), QString());
    QCOMPARE(mb1.buttonText(QMessageBox::Ok | QMessageBox::Default), QString());

    const QString button1 = QStringLiteral("Bar");
    mb2.setButtonText(QMessageBox::Cancel, QStringLiteral("Foo"));
    mb2.setButtonText(QMessageBox::Ok, button1);
    mb2.setButtonText(QMessageBox::Ok | QMessageBox::Default, QStringLiteral("Baz"));

    QCOMPARE(mb2.buttonText(QMessageBox::Cancel), QString());
    QCOMPARE(mb2.buttonText(QMessageBox::Ok), button1);

    QVERIFY(mb3b.buttonText(QMessageBox::Yes).endsWith("Yes"));
    QCOMPARE(mb3b.buttonText(QMessageBox::YesAll), QString());
    QCOMPARE(mb3b.buttonText(QMessageBox::Ok), QString());

    const QString button2 = QStringLiteral("Blah");
    mb3b.setButtonText(QMessageBox::Yes, button2);
    mb3b.setButtonText(QMessageBox::YesAll, QStringLiteral("Zoo"));
    mb3b.setButtonText(QMessageBox::Ok, QStringLiteral("Zoo"));

    QCOMPARE(mb3b.buttonText(QMessageBox::Yes), button2);
    QCOMPARE(mb3b.buttonText(QMessageBox::YesAll), QString());
    QCOMPARE(mb3b.buttonText(QMessageBox::Ok), QString());

    QCOMPARE(mb1.textFormat(), Qt::AutoText);
    mb1.setTextFormat(Qt::PlainText);
    QCOMPARE(mb1.textFormat(), Qt::PlainText);

    CONVENIENCE_FUNC_SYMS(information);
    CONVENIENCE_FUNC_SYMS_EXTRA(information);
    CONVENIENCE_FUNC_SYMS(question);
    CONVENIENCE_FUNC_SYMS_EXTRA(question);
    CONVENIENCE_FUNC_SYMS(warning);
    CONVENIENCE_FUNC_SYMS(critical);

    QSize sizeHint = mb1.sizeHint();
    QVERIFY(sizeHint.width() > 20 && sizeHint.height() > 20);

    QMessageBox::about(&mb1, "title", "text");
    QMessageBox::aboutQt(&mb1);
    QMessageBox::aboutQt(&mb1, "title");
}

void tst_QMessageBox::detailsText()
{
    QMessageBox box;
    QString text("This is the details text.");
    box.setDetailedText(text);
    QCOMPARE(box.detailedText(), text);
    box.show();
    QTest::qWaitForWindowExposed(&box);
    // QTBUG-39334, the box should now have the default "Ok" button as well as
    // the "Show Details.." button.
    QCOMPARE(box.findChildren<QAbstractButton *>().size(), 2);
}

void tst_QMessageBox::detailsButtonText()
{
    QMessageBox box;
    box.setDetailedText("bla");
    box.open();
    QApplication::postEvent(&box, new QEvent(QEvent::LanguageChange));
    QApplication::processEvents();
    QDialogButtonBox* bb = box.findChild<QDialogButtonBox*>("qt_msgbox_buttonbox");
    QVERIFY(bb); //get the detail button

    QList<QAbstractButton *> list = bb->buttons();
    QAbstractButton* btn = NULL;
    foreach(btn, list) {
        if (btn && (btn->inherits("QPushButton"))) {
            if (btn->text().remove(QLatin1Char('&')) != QMessageBox::tr("OK")
                && btn->text() != QMessageBox::tr("Show Details...")) {
                QFAIL(qPrintable(QString("Unexpected messagebox button text: %1").arg(btn->text())));
            }
        }
    }
}

void tst_QMessageBox::expandDetails_QTBUG_32473()
{
    tst_ResizingMessageBox box;
    box.setDetailedText("bla");
    box.show();
    QApplication::postEvent(&box, new QEvent(QEvent::LanguageChange));
    QApplication::processEvents();
    QDialogButtonBox* bb = box.findChild<QDialogButtonBox*>("qt_msgbox_buttonbox");
    QVERIFY(bb);

    QList<QAbstractButton *> list = bb->buttons();
    QAbstractButton* moreButton = NULL;
    foreach (QAbstractButton* btn, list)
        if (btn && bb->buttonRole(btn) == QDialogButtonBox::ActionRole)
            moreButton = btn;
    QVERIFY(moreButton);
    QVERIFY(QTest::qWaitForWindowExposed(&box));
    QRect geom = box.geometry();
    box.resized = false;
    moreButton->click();
    QTRY_VERIFY(box.resized);
    // After we receive the expose event for a second widget, it's likely
    // that the window manager is also done manipulating the first QMessageBox.
    QWidget fleece;
    fleece.show();
    QTest::qWaitForWindowExposed(&fleece);
    if (geom.topLeft() == box.geometry().topLeft())
        QTest::qWait(500);
    QCOMPARE(geom.topLeft(), box.geometry().topLeft());
}

void tst_QMessageBox::incorrectDefaultButton()
{
    ExecCloseHelper closeHelper;
    closeHelper.start(Qt::Key_Escape);
    //Do not crash here
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    QMessageBox::question( 0, "", "I've been hit!",QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Save );
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Escape);
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    QMessageBox::question( 0, "", "I've been hit!",QFlag(QMessageBox::Ok | QMessageBox::Cancel),QMessageBox::Save );
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Escape);
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    //do not crash here -> call old function of QMessageBox in this case
    QMessageBox::question( 0, "", "I've been hit!",QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Save | QMessageBox::Cancel,QMessageBox::Ok);
    QVERIFY(closeHelper.done());
}

void tst_QMessageBox::updateSize()
{
    QMessageBox box;
    box.setText("This is awesome");
    box.show();
    QSize oldSize = box.size();
    QString longText;
    for (int i = 0; i < 20; i++)
        longText += box.text();
    box.setText(longText);
    QVERIFY(box.size() != oldSize); // should have grown
    QVERIFY(box.width() > oldSize.width() || box.height() > oldSize.height());
    oldSize = box.size();
    box.setStandardButtons(QMessageBox::StandardButtons(0xFFFF));
    QVERIFY(box.size() != oldSize); // should have grown
    QVERIFY(box.width() > oldSize.width() || box.height() > oldSize.height());
}

void tst_QMessageBox::setInformativeText()
{
    QMessageBox msgbox(QMessageBox::Warning, "", "", QMessageBox::Ok);
    QString itext = "This is a very long message and it should make the dialog have enough width to fit this message in";
    msgbox.setInformativeText(itext);
    msgbox.show();
    QCOMPARE(msgbox.informativeText(), itext);
    QVERIFY2(msgbox.width() > 190, //verify it's big enough (task181688)
             qPrintable(QString("%1 > 190").arg(msgbox.width())));
}

void tst_QMessageBox::iconPixmap()
{
    QMessageBox messageBox;
    QCOMPARE(messageBox.iconPixmap(), QPixmap());
}

QTEST_MAIN(tst_QMessageBox)
#include "tst_qmessagebox.moc"
