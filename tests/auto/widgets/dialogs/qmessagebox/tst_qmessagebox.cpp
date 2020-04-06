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
    void expandDetailsWithoutMoving();

#ifndef Q_OS_MAC
    void shortcut();
#endif

    void staticSourceCompat();
    void instanceSourceCompat();

    void incorrectDefaultButton();
    void updateSize();

    void setInformativeText();
    void iconPixmap();

    // QTBUG-44131
    void acceptedRejectedSignals();
    void acceptedRejectedSignals_data();

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

    explicit ExecCloseHelper(QObject *parent = nullptr)
        : QObject(parent), m_key(0), m_timerId(0), m_testCandidate(nullptr) { }

    void start(int key, QWidget *testCandidate = nullptr)
    {
        m_key = key;
        m_testCandidate = testCandidate;
        m_timerId = startTimer(50);
    }

    bool done() const { return !m_timerId; }

protected:
    void timerEvent(QTimerEvent *te) override;

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
        m_testCandidate = nullptr;
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
#if defined(Q_OS_MACOS)
    if (QSysInfo::productVersion() == QLatin1String("10.12")) {
        QSKIP("Test hangs on macOS 10.12 -- QTQAINFRA-1362");
        return;
    }
#elif defined(Q_OS_WINRT)
    QSKIP("Test hangs on winrt -- QTBUG-68297");
#endif
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
    QVERIFY(msgBox.button(QMessageBox::Ok) == nullptr); // not added yet
    QPushButton *b1 = msgBox.addButton(QMessageBox::Ok);
    QCOMPARE(msgBox.button(QMessageBox::Ok), static_cast<QAbstractButton *>(b1));  // just added
    QCOMPARE(msgBox.standardButton(b1), QMessageBox::Ok);
    msgBox.addButton(QMessageBox::Cancel);
    QCOMPARE(msgBox.standardButtons(), QMessageBox::Ok | QMessageBox::Cancel);

    // remove the cancel, should not exist anymore
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    QVERIFY(!msgBox.button(QMessageBox::Cancel));
    QVERIFY(msgBox.button(QMessageBox::Yes) != nullptr);

    // should not crash
    QPushButton *b4 = new QPushButton;
    msgBox.addButton(b4, QMessageBox::DestructiveRole);
    msgBox.addButton(nullptr, QMessageBox::ActionRole);
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
    QVERIFY(msgBox.defaultButton() == nullptr); // we have not added it yet
    QPushButton *retryButton = msgBox.addButton(QMessageBox::Retry);
    msgBox.setDefaultButton(retryButton);
    QCOMPARE(msgBox.defaultButton(), retryButton);
    ExecCloseHelper closeHelper;
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), msgBox.button(QMessageBox::Cancel));

    closeHelper.start(Qt::Key_Enter, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), static_cast<QAbstractButton *>(retryButton));

    QAbstractButton *okButton = msgBox.button(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    QCOMPARE(msgBox.defaultButton(), static_cast<QPushButton *>(okButton));
    closeHelper.start(Qt::Key_Enter, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), okButton);
    msgBox.setDefaultButton(QMessageBox::Yes); // its not in there!
    QCOMPARE(msgBox.defaultButton(), okButton);
    msgBox.removeButton(okButton);
    delete okButton;
    okButton = nullptr;
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
    QCOMPARE(msgBox.escapeButton(), static_cast<QAbstractButton *>(retryButton));

    // with escape
    closeHelper.start(Qt::Key_Escape, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), retryButton);

    // with close
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox);
    msgBox.exec();
    QCOMPARE(msgBox.clickedButton(), static_cast<QAbstractButton *>(retryButton));

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
    okButton = nullptr;
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

    QMessageBox msgBox3;
    msgBox3.setDetailedText("Details");
    closeHelper.start(ExecCloseHelper::CloseWindow, &msgBox3);
    msgBox3.exec();
    QVERIFY(msgBox3.clickedButton() == msgBox3.button(QMessageBox::Ok)); // auto detected
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
        QMessageBox::StandardButton sb = (*statics[i])(nullptr, "caption",
            "text", QMessageBox::Yes | QMessageBox::No | QMessageBox::Help | QMessageBox::Cancel,
           QMessageBox::NoButton);
        QCOMPARE(sb, QMessageBox::Cancel);
        QVERIFY(closeHelper.done());

        closeHelper.start(ExecCloseHelper::CloseWindow);
        sb = (*statics[i])(nullptr, "caption",
           "text", QMessageBox::Yes | QMessageBox::No | QMessageBox::Help | QMessageBox::Cancel,
           QMessageBox::NoButton);
        QCOMPARE(sb, QMessageBox::Cancel);
        QVERIFY(closeHelper.done());

        closeHelper.start(Qt::Key_Enter);
        sb = (*statics[i])(nullptr, "caption",
           "text", QMessageBox::Yes | QMessageBox::No | QMessageBox::Help,
           QMessageBox::Yes);
        QCOMPARE(sb, QMessageBox::Yes);
        QVERIFY(closeHelper.done());

        closeHelper.start(Qt::Key_Enter);
        sb = (*statics[i])(nullptr, "caption",
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
    QMessageBox::about(nullptr, "Caption", "This is an auto test");
    // On Mac, about and aboutQt are not modal, so we need to
    // explicitly run the event loop
#ifdef Q_OS_MAC
    QTRY_VERIFY(closeHelper.done());
#else
    QVERIFY(closeHelper.done());
#endif

    const int keyToSend = Qt::Key_Enter;

    closeHelper.start(keyToSend);
    QMessageBox::aboutQt(nullptr, "Caption");
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
    ret = QMessageBox::information(nullptr, "title", "text", QMessageBox::Yes, QMessageBox::No);
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
    ret = QMessageBox::information(nullptr, "title", "text", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);
    QCOMPARE(ret, int(QMessageBox::Yes));
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(nullptr, "title", "text", QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    QCOMPARE(ret, int(QMessageBox::No));
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(nullptr, "title", "text", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape);
    QCOMPARE(ret, int(QMessageBox::Yes));
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(nullptr, "title", "text", QMessageBox::Yes | QMessageBox::Escape, QMessageBox::No | QMessageBox::Default);
    QCOMPARE(ret, int(QMessageBox::No));
    QVERIFY(closeHelper.done());

    // the button text versions
    closeHelper.start(Qt::Key_Enter);
    ret = QMessageBox::information(nullptr, "title", "text", "Yes", "No", QString(), 1);
    QCOMPARE(ret, 1);
    QVERIFY(closeHelper.done());

    if (0) { // don't run these tests since the dialog won't close!
        closeHelper.start(Qt::Key_Escape);
        ret = QMessageBox::information(nullptr, "title", "text", "Yes", "No", QString(), 1);
        QCOMPARE(ret, -1);
        QVERIFY(closeHelper.done());

        closeHelper.start(Qt::Key_Escape);
        ret = QMessageBox::information(nullptr, "title", "text", "Yes", "No", QString(), 0, 1);
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

void tst_QMessageBox::detailsText()
{
    QMessageBox box;
    QString text("This is the details text.");
    box.setDetailedText(text);
    QCOMPARE(box.detailedText(), text);
    box.show();
    QVERIFY(QTest::qWaitForWindowExposed(&box));
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

    auto list = bb->buttons();
    for (auto btn : list) {
        if (btn && (btn->inherits("QPushButton"))) {
            if (btn->text().remove(QLatin1Char('&')) != QMessageBox::tr("OK")
                && btn->text() != QMessageBox::tr("Show Details...")) {
                QFAIL(qPrintable(QString("Unexpected messagebox button text: %1").arg(btn->text())));
            }
        }
    }
}

void tst_QMessageBox::expandDetailsWithoutMoving() // QTBUG-32473
{
    tst_ResizingMessageBox box;
    box.setDetailedText("bla");
    box.show();
    QApplication::postEvent(&box, new QEvent(QEvent::LanguageChange));
    QApplication::processEvents();
    QDialogButtonBox* bb = box.findChild<QDialogButtonBox*>("qt_msgbox_buttonbox");
    QVERIFY(bb);

    auto list = bb->buttons();
    auto it = std::find_if(list.rbegin(), list.rend(), [&](QAbstractButton *btn) {
                               return btn && bb->buttonRole(btn) == QDialogButtonBox::ActionRole; });
    QVERIFY(it != list.rend());
    auto moreButton = *it;

    QVERIFY(QTest::qWaitForWindowExposed(&box));
    QTRY_VERIFY2(!box.geometry().topLeft().isNull(), "window manager is expected to decorate and position the dialog");
    QRect geom = box.geometry();
    box.resized = false;
    // now click the "more" button, and verify that the dialog resizes but does not move
    moreButton->click();
    QTRY_VERIFY(box.resized);
    QVERIFY(box.geometry().height() > geom.height());
    QCOMPARE(box.geometry().topLeft(), geom.topLeft());
}

void tst_QMessageBox::incorrectDefaultButton()
{
    ExecCloseHelper closeHelper;
    closeHelper.start(Qt::Key_Escape);
    //Do not crash here
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    QMessageBox::question(nullptr, "", "I've been hit!",QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Save);
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Escape);
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    QMessageBox::question(nullptr, "", "I've been hit!",QFlag(QMessageBox::Ok | QMessageBox::Cancel),QMessageBox::Save);
    QVERIFY(closeHelper.done());

    closeHelper.start(Qt::Key_Escape);
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    QTest::ignoreMessage(QtWarningMsg, "QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    //do not crash here -> call old function of QMessageBox in this case
    QMessageBox::question(nullptr, "", "I've been hit!",QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Save | QMessageBox::Cancel,QMessageBox::Ok);
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

using SignalSignature = void(QDialog::*)();
Q_DECLARE_METATYPE(SignalSignature);
Q_DECLARE_METATYPE(QMessageBox::ButtonRole)

using ButtonsCreator = std::function<QVector<QPushButton*>(QMessageBox &)>;
Q_DECLARE_METATYPE(ButtonsCreator);

using RoleSet = QSet<QMessageBox::ButtonRole>;
Q_DECLARE_METATYPE(RoleSet);

void tst_QMessageBox::acceptedRejectedSignals()
{
    QMessageBox messageBox(QMessageBox::Information, "Test window", "Test text");

    QFETCH(ButtonsCreator, buttonsCreator);
    QVERIFY(buttonsCreator);

    const auto buttons = buttonsCreator(messageBox);
    QVERIFY(!buttons.isEmpty());

    QFETCH(RoleSet, roles);
    QFETCH(SignalSignature, signalSignature);
    for (auto button: buttons) {
        QVERIFY(button);

        messageBox.show();
        QVERIFY(QTest::qWaitForWindowExposed(&messageBox));

        QSignalSpy spy(&messageBox, signalSignature);
        QVERIFY(spy.isValid());
        button->click();

        if (roles.contains(messageBox.buttonRole(button)))
            QCOMPARE(spy.count(), 1);
        else
            QVERIFY(spy.isEmpty());
    }
}

static void addAcceptedRow(const char *title, ButtonsCreator bc)
{
    QTest::newRow(title) << (RoleSet() << QMessageBox::AcceptRole << QMessageBox::YesRole)
                         << &QDialog::accepted << bc;
}

static void addRejectedRow(const char *title, ButtonsCreator bc)
{
    QTest::newRow(title) << (RoleSet() << QMessageBox::RejectRole << QMessageBox::NoRole)
                         << &QDialog::rejected << bc;
}

static void addCustomButtonsData()
{
    ButtonsCreator buttonsCreator = [](QMessageBox &messageBox) {
        QVector<QPushButton*> buttons(QMessageBox::NRoles);
        for (int i = QMessageBox::AcceptRole; i < QMessageBox::NRoles; ++i) {
            buttons[i] = messageBox.addButton(
                QString("Button role: %1").arg(i), QMessageBox::ButtonRole(i));
        }

        return buttons;
    };

    addAcceptedRow("Accepted_CustomButtons", buttonsCreator);
    addRejectedRow("Rejected_CustomButtons", buttonsCreator);
}

static void addStandardButtonsData()
{
    ButtonsCreator buttonsCreator = [](QMessageBox &messageBox) {
        QVector<QPushButton*> buttons;
        for (int i = QMessageBox::FirstButton; i <= QMessageBox::LastButton; i <<= 1)
            buttons << messageBox.addButton(QMessageBox::StandardButton(i));

        return buttons;
    };

    addAcceptedRow("Accepted_StandardButtons", buttonsCreator);
    addRejectedRow("Rejected_StandardButtons", buttonsCreator);
}

void tst_QMessageBox::acceptedRejectedSignals_data()
{
    QTest::addColumn<RoleSet>("roles");
    QTest::addColumn<SignalSignature>("signalSignature");
    QTest::addColumn<ButtonsCreator>("buttonsCreator");

    addStandardButtonsData();
    addCustomButtonsData();
}

QTEST_MAIN(tst_QMessageBox)
#include "tst_qmessagebox.moc"
