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


#include <QFont>
#include <QtTest/QtTest>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWizard>
#include <QTreeWidget>
#include <QScreen>

Q_DECLARE_METATYPE(QWizard::WizardButton);

static QImage grabWidget(QWidget *window)
{
    return window->grab().toImage();
}

class tst_QWizard : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void buttonText();
    void setButtonLayout();
    void setButton();
    void setTitleFormatEtc();
    void setPixmap();
    void setDefaultProperty();
    void addPage();
    void setPage();
    void setStartId();
    void setOption_IndependentPages();
    void setOption_IgnoreSubTitles();
    void setOption_ExtendedWatermarkPixmap();
    void setOption_NoDefaultButton();
    void setOption_NoBackButtonOnStartPage();
    void setOption_NoBackButtonOnLastPage();
    void setOption_DisabledBackButtonOnLastPage();
    void setOption_HaveNextButtonOnLastPage();
    void setOption_HaveFinishButtonOnEarlyPages();
    void setOption_NoCancelButton();
    void setOption_NoCancelButtonOnLastPage();
    void setOption_CancelButtonOnLeft();
    void setOption_HaveHelpButton();
    void setOption_HelpButtonOnRight();
    void setOption_HaveCustomButtonX();
    void combinations_data();
    void combinations();
    void showCurrentPageOnly();
    void setButtonText();
    void setCommitPage();
    void setWizardStyle();
    void removePage();
    void sideWidget();
    void objectNames_data();
    void objectNames();

    // task-specific tests below me:
    void task177716_disableCommitButton();
    void task183550_stretchFactor();
    void task161658_alignments();
    void task177022_setFixedSize();
    void task248107_backButton();
    void task255350_fieldObjectDestroyed();
    void taskQTBUG_25691_fieldObjectDestroyed2();
    void taskQTBUG_46894_nextButtonShortcut();

    /*
        Things that could be added:

        1. Test virtual functions that are called, signals that are
           emitted, etc.

        2. Test QWizardPage more thorougly.

        3. Test the look and field a bit more (especially the
           different wizard styles, and how they interact with
           pixmaps, titles, subtitles, etc.).

        4. Test minimum sizes, sizes, maximum sizes, resizing, etc.

        5. Try setting various options and wizard styles in various
           orders and check that the results are the same every time,
           no matter the order in which the properties were set.

           -> Initial version done (tst_QWizard::combinations())

        6. Test done() and restart().

        7. Test default properties of built-in widgets.

        8. Test mutual exclusiveness of Next and Commit buttons.
    */
};

void tst_QWizard::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QWizard::buttonText()
{
    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);

    // Check the buttons' original text in Classic and Modern styles.
    for (int pass = 0; pass < 2; ++pass) {
        QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("< &Back"));
        QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
        QVERIFY(wizard.buttonText(QWizard::FinishButton).endsWith("Finish"));
        QVERIFY(wizard.buttonText(QWizard::CancelButton).endsWith("Cancel"));
        QVERIFY(wizard.buttonText(QWizard::HelpButton).endsWith("Help"));

        QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::CustomButton2).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

        // robustness
        QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

        wizard.setWizardStyle(QWizard::ModernStyle);
    }

    // Check the buttons' original text in Mac style.
    wizard.setWizardStyle(QWizard::MacStyle);

    QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("Go Back"));
    QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("Continue"));
    QCOMPARE(wizard.buttonText(QWizard::FinishButton), QString("Done"));
    QCOMPARE(wizard.buttonText(QWizard::CancelButton), QString("Cancel"));
    QCOMPARE(wizard.buttonText(QWizard::HelpButton), QString("Help"));

    QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::CustomButton2).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

    // robustness
    QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

    // Modify the buttons' text and see what happens.
    wizard.setButtonText(QWizard::NextButton, "N&este");
    wizard.setButtonText(QWizard::CustomButton2, "&Cucu");
    wizard.setButtonText(QWizard::Stretch, "Stretch");

    QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("Go Back"));
    QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("N&este"));
    QCOMPARE(wizard.buttonText(QWizard::FinishButton), QString("Done"));
    QCOMPARE(wizard.buttonText(QWizard::CancelButton), QString("Cancel"));
    QCOMPARE(wizard.buttonText(QWizard::HelpButton), QString("Help"));

    QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
    QCOMPARE(wizard.buttonText(QWizard::CustomButton2), QString("&Cucu"));
    QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

    // robustness
    QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

    // Switch back to Classic style and see what happens.
    wizard.setWizardStyle(QWizard::ClassicStyle);

    for (int pass = 0; pass < 2; ++pass) {
        QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("< &Back"));
        QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("N&este"));
        QVERIFY(wizard.buttonText(QWizard::FinishButton).endsWith("Finish"));
        QVERIFY(wizard.buttonText(QWizard::CancelButton).endsWith("Cancel"));
        QVERIFY(wizard.buttonText(QWizard::HelpButton).endsWith("Help"));

        QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
        QCOMPARE(wizard.buttonText(QWizard::CustomButton2), QString("&Cucu"));
        QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

        // robustness
        QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

        wizard.setOptions(QWizard::NoDefaultButton
                          | QWizard::NoBackButtonOnStartPage
                          | QWizard::NoBackButtonOnLastPage
                          | QWizard::DisabledBackButtonOnLastPage
                          | QWizard::NoCancelButton
                          | QWizard::CancelButtonOnLeft
                          | QWizard::HaveHelpButton
                          | QWizard::HelpButtonOnRight
                          | QWizard::HaveCustomButton1
                          | QWizard::HaveCustomButton2
                          | QWizard::HaveCustomButton3);
    }
}

void tst_QWizard::setButtonLayout()
{
    QList<QWizard::WizardButton> layout;

    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);
    wizard.setOptions(0);
    wizard.setButtonLayout(layout);
    wizard.show();
    qApp->processEvents();

    // if these crash, this means there's a bug in QWizard
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.button(QWizard::BackButton)->text().contains("Back"));
    QVERIFY(wizard.button(QWizard::FinishButton)->text().contains("Finish"));
    QVERIFY(wizard.button(QWizard::CancelButton)->text().contains("Cancel"));
    QVERIFY(wizard.button(QWizard::HelpButton)->text().contains("Help"));
    QVERIFY(wizard.button(QWizard::CustomButton1)->text().isEmpty());
    QVERIFY(wizard.button(QWizard::CustomButton2)->text().isEmpty());
    QVERIFY(wizard.button(QWizard::CustomButton3)->text().isEmpty());
    QVERIFY(!wizard.button(QWizard::Stretch));
    QVERIFY(!wizard.button(QWizard::NoButton));

    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());

    layout << QWizard::NextButton << QWizard::HelpButton;
    wizard.setButtonLayout(layout);
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

    wizard.restart();
    qApp->processEvents();

    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

    layout.clear();
    layout << QWizard::NextButton << QWizard::HelpButton << QWizard::BackButton
           << QWizard::FinishButton << QWizard::CancelButton << QWizard::Stretch
           << QWizard::CustomButton2;

    // Turn on all the button-related wizard options. Some of these
    // should have no impact on a custom layout; others should.
    wizard.setButtonLayout(layout);
    wizard.setOptions(QWizard::NoDefaultButton
                      | QWizard::NoBackButtonOnStartPage
                      | QWizard::NoBackButtonOnLastPage
                      | QWizard::DisabledBackButtonOnLastPage
                      | QWizard::HaveNextButtonOnLastPage
                      | QWizard::HaveFinishButtonOnEarlyPages
                      | QWizard::NoCancelButton
                      | QWizard::CancelButtonOnLeft
                      | QWizard::HaveHelpButton
                      | QWizard::HelpButtonOnRight
                      | QWizard::HaveCustomButton1
                      | QWizard::HaveCustomButton2
                      | QWizard::HaveCustomButton3);
    qApp->processEvents();

    // we're on first page
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());
    QVERIFY(wizard.button(QWizard::CancelButton)->isVisible()); // NoCancelButton overridden
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::CustomButton1)->isVisible());
    QVERIFY(wizard.button(QWizard::CustomButton2)->isVisible());    // HaveCustomButton2 overridden
    QVERIFY(!wizard.button(QWizard::CustomButton3)->isVisible());

    wizard.next();
    qApp->processEvents();

    // we're on last page
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());
    QVERIFY(wizard.button(QWizard::FinishButton)->isEnabled());
    QVERIFY(wizard.button(QWizard::CancelButton)->isVisible()); // NoCancelButton overridden
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::CustomButton1)->isVisible());
    QVERIFY(wizard.button(QWizard::CustomButton2)->isVisible());    // HaveCustomButton2 overridden
    QVERIFY(!wizard.button(QWizard::CustomButton3)->isVisible());

    // Check that the buttons are in the right order on screen.
    for (int pass = 0; pass < 2; ++pass) {
        wizard.setLayoutDirection(pass == 0 ? Qt::LeftToRight : Qt::RightToLeft);
        qApp->processEvents();

        int sign = (pass == 0) ? +1 : -1;

        int p[5];
        p[0] = sign * wizard.button(QWizard::NextButton)->x();
        p[1] = sign * wizard.button(QWizard::HelpButton)->x();
        p[2] = sign * wizard.button(QWizard::FinishButton)->x();
        p[3] = sign * wizard.button(QWizard::CancelButton)->x();
        p[4] = sign * wizard.button(QWizard::CustomButton2)->x();

        QVERIFY(p[0] < p[1]);
        QVERIFY(p[1] < p[2]);
        QVERIFY(p[2] < p[3]);
        QVERIFY(p[3] < p[4]);
    }

    layout.clear();
    wizard.setButtonLayout(layout);
    qApp->processEvents();

    for (int i = -1; i < 50; ++i) {
        QAbstractButton *button = wizard.button(QWizard::WizardButton(i));
        QVERIFY(!button || !button->isVisible());
    }
}

void tst_QWizard::setButton()
{
    QPointer<QToolButton> toolButton = new QToolButton;

    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);
    wizard.setButton(QWizard::NextButton, toolButton);
    wizard.setButton(QWizard::CustomButton2, new QCheckBox("Kustom 2"));

    QVERIFY(qobject_cast<QToolButton *>(wizard.button(QWizard::NextButton)));
    QVERIFY(qobject_cast<QCheckBox *>(wizard.button(QWizard::CustomButton2)));
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::CustomButton1)));

    QVERIFY(toolButton != 0);

    // resetting the same button does nothing
    wizard.setButton(QWizard::NextButton, toolButton);
    QVERIFY(toolButton != 0);

    // revert to default button
    wizard.setButton(QWizard::NextButton, 0);
    QVERIFY(toolButton.isNull());
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton)));
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
}

void tst_QWizard::setTitleFormatEtc()
{
    QWizard wizard;
    QCOMPARE(wizard.titleFormat(), Qt::AutoText);
    QCOMPARE(wizard.subTitleFormat(), Qt::AutoText);

    wizard.setTitleFormat(Qt::RichText);
    QCOMPARE(wizard.titleFormat(), Qt::RichText);
    QCOMPARE(wizard.subTitleFormat(), Qt::AutoText);

    wizard.setSubTitleFormat(Qt::PlainText);
    QCOMPARE(wizard.titleFormat(), Qt::RichText);
    QCOMPARE(wizard.subTitleFormat(), Qt::PlainText);
}

void tst_QWizard::setPixmap()
{
    QPixmap p1(1, 1);
    QPixmap p2(2, 2);
    QPixmap p3(3, 3);
    QPixmap p4(4, 4);
    QPixmap p5(5, 5);

    QWizard wizard;
    QWizardPage *page = new QWizardPage;
    QWizardPage *page2 = new QWizardPage;

    wizard.addPage(page);
    wizard.addPage(page2);

    QVERIFY(wizard.pixmap(QWizard::BannerPixmap).isNull());
    QVERIFY(wizard.pixmap(QWizard::LogoPixmap).isNull());
    QVERIFY(wizard.pixmap(QWizard::WatermarkPixmap).isNull());
#ifdef Q_OS_OSX
    QVERIFY(!wizard.pixmap(QWizard::BackgroundPixmap).isNull());
#else
    QVERIFY(wizard.pixmap(QWizard::BackgroundPixmap).isNull());
#endif

    QVERIFY(page->pixmap(QWizard::BannerPixmap).isNull());
    QVERIFY(page->pixmap(QWizard::LogoPixmap).isNull());
    QVERIFY(page->pixmap(QWizard::WatermarkPixmap).isNull());
#ifdef Q_OS_OSX
    QVERIFY(!wizard.pixmap(QWizard::BackgroundPixmap).isNull());
#else
    QVERIFY(page->pixmap(QWizard::BackgroundPixmap).isNull());
#endif
    wizard.setPixmap(QWizard::BannerPixmap, p1);
    wizard.setPixmap(QWizard::LogoPixmap, p2);
    wizard.setPixmap(QWizard::WatermarkPixmap, p3);
    wizard.setPixmap(QWizard::BackgroundPixmap, p4);

    page->setPixmap(QWizard::LogoPixmap, p5);

    QCOMPARE(wizard.pixmap(QWizard::BannerPixmap).size(), p1.size());
    QCOMPARE(wizard.pixmap(QWizard::LogoPixmap).size(), p2.size());
    QCOMPARE(wizard.pixmap(QWizard::WatermarkPixmap).size(), p3.size());
    QCOMPARE(wizard.pixmap(QWizard::BackgroundPixmap).size(), p4.size());

    QCOMPARE(page->pixmap(QWizard::BannerPixmap).size(), p1.size());
    QCOMPARE(page->pixmap(QWizard::LogoPixmap).size(), p5.size());
    QCOMPARE(page->pixmap(QWizard::WatermarkPixmap).size(), p3.size());
    QCOMPARE(page->pixmap(QWizard::BackgroundPixmap).size(), p4.size());

    QCOMPARE(page2->pixmap(QWizard::BannerPixmap).size(), p1.size());
    QCOMPARE(page2->pixmap(QWizard::LogoPixmap).size(), p2.size());
    QCOMPARE(page2->pixmap(QWizard::WatermarkPixmap).size(), p3.size());
    QCOMPARE(page2->pixmap(QWizard::BackgroundPixmap).size(), p4.size());
}

class MyPage1 : public QWizardPage
{
public:
    MyPage1() {
        edit1 = new QLineEdit("Bla 1", this);

        edit2 = new QLineEdit("Bla 2", this);
        edit2->setInputMask("Mask");

        edit3 = new QLineEdit("Bla 3", this);
        edit3->setMaxLength(25);

        edit4 = new QLineEdit("Bla 4", this);
    }

    void registerField(const QString &name, QWidget *widget,
                       const char *property = 0,
                       const char *changedSignal = 0)
        { QWizardPage::registerField(name, widget, property, changedSignal); }

    QLineEdit *edit1;
    QLineEdit *edit2;
    QLineEdit *edit3;
    QLineEdit *edit4;
};

void tst_QWizard::setDefaultProperty()
{
    QWizard wizard;
    MyPage1 *page = new MyPage1;
    wizard.addPage(page);

    page->registerField("edit1", page->edit1);

    wizard.setDefaultProperty("QLineEdit", "inputMask", 0);
    page->registerField("edit2", page->edit2);

    wizard.setDefaultProperty("QLineEdit", "maxLength", 0);
    page->registerField("edit3", page->edit3);

    wizard.setDefaultProperty("QLineEdit", "text", SIGNAL(textChanged(QString)));
    page->registerField("edit3bis", page->edit3);

    wizard.setDefaultProperty("QWidget", "enabled", 0); // less specific, i.e. ignored
    page->registerField("edit4", page->edit4);
    QTest::ignoreMessage(QtWarningMsg,"QWizard::setField: Couldn't write to property 'customProperty'");
    wizard.setDefaultProperty("QLineEdit", "customProperty", 0);
    page->registerField("edit4bis", page->edit4);

    QCOMPARE(wizard.field("edit1").toString(), QString("Bla 1"));
    QCOMPARE(wizard.field("edit2").toString(), page->edit2->inputMask());
    QCOMPARE(wizard.field("edit3").toInt(), 25);
    QCOMPARE(wizard.field("edit3bis").toString(), QString("Bla 3"));
    QCOMPARE(wizard.field("edit4").toString(), QString("Bla 4"));
    QCOMPARE(wizard.field("edit4bis").toString(), QString());

    wizard.setField("edit1", "Alpha");
    wizard.setField("edit2", "Beta");
    wizard.setField("edit3", 50);
    wizard.setField("edit3bis", "Gamma");
    wizard.setField("edit4", "Delta");
    wizard.setField("edit4bis", "Epsilon");

    QCOMPARE(wizard.field("edit1").toString(), QString("Alpha"));
    QVERIFY(wizard.field("edit2").toString().contains("Beta"));
    QCOMPARE(wizard.field("edit3").toInt(), 50);
    QCOMPARE(wizard.field("edit3bis").toString(), QString("Gamma"));
    QCOMPARE(wizard.field("edit4").toString(), QString("Delta"));
    QCOMPARE(wizard.field("edit4bis").toString(), QString("Epsilon"));

    // make sure the data structure is reasonable
    for (int i = 0; i < 200000; ++i) {
        wizard.setDefaultProperty("QLineEdit", QByteArray('x' + QByteArray::number(i)).constData(), 0);
        wizard.setDefaultProperty("QLabel", QByteArray('y' + QByteArray::number(i)).constData(), 0);
    }
}

void tst_QWizard::addPage()
{
    QWidget *parent = new QWidget;
    QWizard wizard;
    const int N = 100;
    QWizardPage *pages[N];
    QSignalSpy spy(&wizard, SIGNAL(pageAdded(int)));

    for (int i = 0; i < N; ++i) {
        pages[i] = new QWizardPage(parent);
        QCOMPARE(wizard.addPage(pages[i]), i);
        QCOMPARE(pages[i]->window(), (QWidget *)&wizard);
        QCOMPARE(wizard.startId(), 0);
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toInt(), i);
    }

    for (int i = 0; i < N; ++i) {
        QCOMPARE(pages[i], wizard.page(i));
    }
    QVERIFY(!wizard.page(-1));
    QVERIFY(!wizard.page(N));
    QVERIFY(!wizard.page(N + 1));

    wizard.setPage(N + 50, new QWizardPage);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), N + 50);
    wizard.setPage(-3000, new QWizardPage);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), -3000);

    QWizardPage *pageX = new QWizardPage;
    QCOMPARE(wizard.addPage(pageX), N + 51);
    QCOMPARE(wizard.page(N + 51), pageX);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), N + 51);

    QCOMPARE(wizard.addPage(new QWizardPage), N + 52);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), N + 52);

    QTest::ignoreMessage(QtWarningMsg,"QWizard::setPage: Cannot insert null page");
    wizard.addPage(0); // generates a warning
    QCOMPARE(spy.count(), 0);
    delete parent;
}

#define CHECK_VISITED(wizard, list) \
    do { \
        QList<int> myList = list; \
        QCOMPARE((wizard).visitedPages(), myList); \
        Q_FOREACH(int id, myList) \
            QVERIFY((wizard).hasVisitedPage(id)); \
    } while (0)

void tst_QWizard::setPage()
{
    QWidget *parent = new QWidget;
    QWizard wizard;
    QWizardPage *page;
    QSignalSpy spy(&wizard, SIGNAL(pageAdded(int)));

    QCOMPARE(wizard.startId(), -1);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);

    page = new QWizardPage(parent);
    QTest::ignoreMessage(QtWarningMsg,"QWizard::setPage: Cannot insert page with ID -1");
    wizard.setPage(-1, page);   // gives a warning and does nothing
    QCOMPARE(spy.count(), 0);
    QVERIFY(!wizard.page(-2));
    QVERIFY(!wizard.page(-1));
    QVERIFY(!wizard.page(0));
    QCOMPARE(wizard.startId(), -1);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>());

    page = new QWizardPage(parent);
    wizard.setPage(0, page);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 0);
    QCOMPARE(page->window(), (QWidget *)&wizard);
    QCOMPARE(wizard.page(0), page);
    QCOMPARE(wizard.startId(), 0);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>());

    page = new QWizardPage(parent);
    wizard.setPage(-2, page);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), -2);
    QCOMPARE(page->window(), (QWidget *)&wizard);
    QCOMPARE(wizard.page(-2), page);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>());

    wizard.restart();
    QCOMPARE(wizard.page(-2), page);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QCOMPARE(wizard.currentPage(), page);
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    page = new QWizardPage(parent);
    wizard.setPage(2, page);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 2);
    QCOMPARE(wizard.page(2), page);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QCOMPARE(wizard.currentPage(), wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    wizard.restart();
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QCOMPARE(wizard.currentPage(), wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    page = new QWizardPage(parent);
    wizard.setPage(-3, page);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), -3);
    QCOMPARE(wizard.page(-3), page);
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -2);
    QCOMPARE(wizard.currentPage(), wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    wizard.restart();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -3);
    QCOMPARE(wizard.currentPage(), wizard.page(-3));
    QCOMPARE(wizard.nextId(), -2);
    CHECK_VISITED(wizard, QList<int>() << -3);

    wizard.next();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -2);
    QCOMPARE(wizard.currentPage(), wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2);

    wizard.next();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), 0);
    QCOMPARE(wizard.currentPage(), wizard.page(0));
    QCOMPARE(wizard.nextId(), 2);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2 << 0);

    for (int i = 0; i < 100; ++i) {
        wizard.next();
        QCOMPARE(wizard.startId(), -3);
        QCOMPARE(wizard.currentId(), 2);
        QCOMPARE(wizard.currentPage(), wizard.page(2));
        QCOMPARE(wizard.nextId(), -1);
        CHECK_VISITED(wizard, QList<int>() << -3 << -2 << 0 << 2);
    }

    wizard.back();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), 0);
    QCOMPARE(wizard.currentPage(), wizard.page(0));
    QCOMPARE(wizard.nextId(), 2);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2 << 0);

    wizard.back();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -2);
    QCOMPARE(wizard.currentPage(), wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2);

    for (int i = 0; i < 100; ++i) {
        wizard.back();
        QCOMPARE(wizard.startId(), -3);
        QCOMPARE(wizard.currentId(), -3);
        QCOMPARE(wizard.currentPage(), wizard.page(-3));
        QCOMPARE(wizard.nextId(), -2);
        CHECK_VISITED(wizard, QList<int>() << -3);
    }

    for (int i = 0; i < 100; ++i) {
        wizard.restart();
        QCOMPARE(wizard.startId(), -3);
        QCOMPARE(wizard.currentId(), -3);
        QCOMPARE(wizard.currentPage(), wizard.page(-3));
        QCOMPARE(wizard.nextId(), -2);
        CHECK_VISITED(wizard, QList<int>() << -3);
    }
    QCOMPARE(spy.count(), 0);
    delete parent;
}

void tst_QWizard::setStartId()
{
    QWizard wizard;
    QCOMPARE(wizard.startId(), -1);

    wizard.setPage(INT_MIN, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(-2, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(0, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(1, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(INT_MAX, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    QTest::ignoreMessage(QtWarningMsg,"QWizard::setStartId: Invalid page ID 123");
    wizard.setStartId(123);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setStartId(-1);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setStartId(-2);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.nextId(), -1);

    wizard.setStartId(-1);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setStartId(-2);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.nextId(), -1);

    wizard.restart();
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QCOMPARE(wizard.currentPage(), wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);

    wizard.next();
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), 0);
    QCOMPARE(wizard.currentPage(), wizard.page(0));
    QCOMPARE(wizard.nextId(), 1);

    wizard.setStartId(INT_MIN);
    QCOMPARE(wizard.startId(), INT_MIN);
    QCOMPARE(wizard.currentId(), 0);
    QCOMPARE(wizard.currentPage(), wizard.page(0));
    QCOMPARE(wizard.nextId(), 1);

    wizard.next();
    QCOMPARE(wizard.startId(), INT_MIN);
    QCOMPARE(wizard.currentId(), 1);
    QCOMPARE(wizard.currentPage(), wizard.page(1));
    QCOMPARE(wizard.nextId(), INT_MAX);

    wizard.next();
    QCOMPARE(wizard.startId(), INT_MIN);
    QCOMPARE(wizard.currentId(), INT_MAX);
    QCOMPARE(wizard.currentPage(), wizard.page(INT_MAX));
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>() << -2 << 0 << 1 << INT_MAX);
}

struct MyPage2 : public QWizardPage
{
public:
    MyPage2() : init(0), cleanup(0), validate(0) {}

    void initializePage() { ++init; QWizardPage::initializePage(); }
    void cleanupPage() { ++cleanup; QWizardPage::cleanupPage(); }
    bool validatePage() { ++validate; return QWizardPage::validatePage(); }

    bool sanityCheck(int init, int cleanup)
    {
        return init == this->init
            && cleanup == this->cleanup
            && (this->init == this->cleanup || this->init - 1 == this->cleanup);
    }

    int init;
    int cleanup;
    int validate;
};

#define CHECK_PAGE_INIT(i0, c0, i1, c1, i2, c2) \
    QVERIFY(page0->sanityCheck((i0), (c0))); \
    QVERIFY(page1->sanityCheck((i1), (c1))); \
    QVERIFY(page2->sanityCheck((i2), (c2)));

void tst_QWizard::setOption_IndependentPages()
{
    MyPage2 *page0 = new MyPage2;
    MyPage2 *page1 = new MyPage2;
    MyPage2 *page2 = new MyPage2;

    QWizard wizard;
    wizard.addPage(page0);
    wizard.addPage(page1);
    wizard.addPage(page2);

    QVERIFY(!wizard.testOption(QWizard::IndependentPages));

    wizard.restart();

    // Make sure initializePage() and cleanupPage() are called are
    // they should when the
    // wizard.testOption(QWizard::IndependentPages option is off.
    for (int i = 0; i < 10; ++i) {
        CHECK_PAGE_INIT(i + 1, i, i, i, i, i);

        wizard.next();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i, i);

        wizard.next();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i + 1, i);

        wizard.next();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i + 1, i);

        wizard.back();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i + 1, i + 1);

        wizard.back();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i + 1, i + 1, i + 1);

        wizard.back();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i + 1, i + 1, i + 1);

        wizard.restart();
    }

    CHECK_PAGE_INIT(11, 10, 10, 10, 10, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    // Now, turn on the option and check that they're called at the
    // appropriate times (which aren't the same).
    wizard.setOption(QWizard::IndependentPages, true);
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    wizard.back();
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 11, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 11, 10);

    wizard.back();
    CHECK_PAGE_INIT(11, 10, 11, 10, 11, 10);

    wizard.setStartId(2);

    wizard.restart();
    CHECK_PAGE_INIT(11, 11, 11, 11, 12, 11);

    wizard.back();
    CHECK_PAGE_INIT(11, 11, 11, 11, 12, 11);

    wizard.next();
    CHECK_PAGE_INIT(11, 11, 11, 11, 12, 11);

    wizard.setStartId(0);
    wizard.restart();
    CHECK_PAGE_INIT(12, 11, 11, 11, 12, 12);

    wizard.next();
    CHECK_PAGE_INIT(12, 11, 12, 11, 12, 12);

    wizard.next();
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 12);

    wizard.back();
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 12);

    // Fun stuff here.

    wizard.setOption(QWizard::IndependentPages, false);
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 13);

    wizard.setOption(QWizard::IndependentPages, true);
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 13);

    wizard.setOption(QWizard::IndependentPages, false);
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 13);

    wizard.back();
    CHECK_PAGE_INIT(12, 11, 12, 12, 13, 13);

    wizard.back();
    CHECK_PAGE_INIT(12, 11, 12, 12, 13, 13);
}

void tst_QWizard::setOption_IgnoreSubTitles()
{
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const int kPixels = (availableGeometry.width() + 500) / 1000;
    const int frame = 50 * kPixels;
    const int size = 400 * kPixels;
    QWizard wizard1;
#ifdef Q_OS_WIN
    wizard1.setWizardStyle(QWizard::ClassicStyle); // Avoid Vista style focus animations, etc.
#endif
    wizard1.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    wizard1.resize(size, size);
    wizard1.move(availableGeometry.left() + frame, availableGeometry.top() + frame);
    QVERIFY(!wizard1.testOption(QWizard::IgnoreSubTitles));
    QWizardPage *page11 = new QWizardPage;
    page11->setTitle("Page X");
    page11->setSubTitle("Some subtitle");

    QWizardPage *page12 = new QWizardPage;
    page12->setTitle("Page X");

    wizard1.addPage(page11);
    wizard1.addPage(page12);

    QWizard wizard2;
#ifdef Q_OS_WIN
    wizard2.setWizardStyle(QWizard::ClassicStyle); // Avoid Vista style focus animations, etc.
#endif
    wizard2.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    wizard2.resize(size, size);
    wizard2.move(availableGeometry.left() + 2 * frame + size, availableGeometry.top() + frame);
    wizard2.setOption(QWizard::IgnoreSubTitles, true);
    QWizardPage *page21 = new QWizardPage;
    page21->setTitle("Page X");
    page21->setSubTitle("Some subtitle");

    QWizardPage *page22 = new QWizardPage;
    page22->setTitle("Page X");

    wizard2.addPage(page21);
    wizard2.addPage(page22);

    wizard1.show();
    wizard2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wizard2));

    // Check that subtitles are shown when they should (i.e.,
    // they're set and IgnoreSubTitles is off).

    qApp->setActiveWindow(0); // ensure no focus rectangle around cancel button
    QImage i11 = grabWidget(&wizard1);
    QImage i21 = grabWidget(&wizard2);
    QVERIFY(i11 != i21);

    wizard1.next();
    wizard2.next();

    QImage i12 = grabWidget(&wizard1);
    QImage i22 = grabWidget(&wizard2);
    QCOMPARE(i12, i22);
    QCOMPARE(i21, i22);

    wizard1.back();
    wizard2.back();

    QImage i13 = grabWidget(&wizard1);
    QImage i23 = grabWidget(&wizard2);
    QCOMPARE(i13, i11);
    QCOMPARE(i23, i21);

    wizard1.setOption(QWizard::IgnoreSubTitles, true);
    wizard2.setOption(QWizard::IgnoreSubTitles, false);

    QImage i14 = grabWidget(&wizard1);
    QImage i24 = grabWidget(&wizard2);
    QCOMPARE(i14, i21);
    QCOMPARE(i24, i11);

    // Check the impact of subtitles on the rest of the layout, by
    // using a subtitle that looks empty (but that isn't). In
    // Classic and Modern styles, this should be enough to trigger a
    // "header"; in Mac style, this only creates a QLabel, with no
    // text, i.e. it doesn't affect the layout.

    page11->setSubTitle("<b></b>");    // not quite empty, but looks empty

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            wizard1.setOption(QWizard::IgnoreSubTitles, j == 0);

            wizard1.setWizardStyle(i == 0 ? QWizard::ClassicStyle
                                   : i == 1 ? QWizard::ModernStyle
                                            : QWizard::MacStyle);
            wizard1.restart();
            QImage i1 = grabWidget(&wizard1);

            wizard1.next();
            QImage i2 = grabWidget(&wizard1);

            if (j == 0 || wizard1.wizardStyle() == QWizard::MacStyle) {
                QCOMPARE(i1, i2);
            } else {
                QVERIFY(i1 != i2);
            }
        }
    }
}

void tst_QWizard::setOption_ExtendedWatermarkPixmap()
{
    QPixmap watermarkPixmap(200, 400);
    watermarkPixmap.fill(Qt::black);

    QWizard wizard1;
    wizard1.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    QVERIFY(!wizard1.testOption(QWizard::ExtendedWatermarkPixmap));
    QWizardPage *page11 = new QWizardPage;
    page11->setTitle("Page X");
    page11->setPixmap(QWizard::WatermarkPixmap, watermarkPixmap);

    QWizardPage *page12 = new QWizardPage;
    page12->setTitle("Page X");

    wizard1.addPage(page11);
    wizard1.addPage(page12);

    QWizard wizard2;
    wizard2.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    wizard2.setOption(QWizard::ExtendedWatermarkPixmap, true);
    QWizardPage *page21 = new QWizardPage;
    page21->setTitle("Page X");
    page21->setPixmap(QWizard::WatermarkPixmap, watermarkPixmap);

    QWizardPage *page22 = new QWizardPage;
    page22->setTitle("Page X");

    wizard2.addPage(page21);
    wizard2.addPage(page22);

    wizard1.show();
    wizard2.show();

    // Check the impact of watermark pixmaps on the rest of the layout.

    for (int i = 0; i < 3; ++i) {
        QImage i1[2];
        QImage i2[2];
        for (int j = 0; j < 2; ++j) {
            wizard1.setOption(QWizard::ExtendedWatermarkPixmap, j == 0);

            wizard1.setWizardStyle(i == 0 ? QWizard::ClassicStyle
                                   : i == 1 ? QWizard::ModernStyle
                                            : QWizard::MacStyle);
            wizard1.restart();
            wizard1.setMaximumSize(1000, 1000);
            wizard1.resize(600, 600);
            i1[j] = grabWidget(&wizard1);

            wizard1.next();
            wizard1.setMaximumSize(1000, 1000);
            wizard1.resize(600, 600);
            i2[j] = grabWidget(&wizard1);
        }

        if (wizard1.wizardStyle() == QWizard::MacStyle) {
            QCOMPARE(i1[0], i1[1]);
            QCOMPARE(i2[0], i2[1]);
            QCOMPARE(i1[0], i2[0]);
        } else {
            QVERIFY(i1[0] != i1[1]);
            QCOMPARE(i2[0], i2[1]);
            QVERIFY(i1[0] != i2[0]);
            QVERIFY(i1[1] != i2[1]);
        }
    }
}

void tst_QWizard::setOption_NoDefaultButton()
{
    QWizard wizard;
    wizard.setOption(QWizard::NoDefaultButton, false);
    wizard.setOption(QWizard::HaveFinishButtonOnEarlyPages, true);
    wizard.addPage(new QWizardPage);
    wizard.page(0)->setFinalPage(true);
    wizard.addPage(new QWizardPage);

    if (QPushButton *pb = qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton)))
        pb->setAutoDefault(false);
    if (QPushButton *pb = qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton)))
        pb->setAutoDefault(false);

    wizard.show();
    qApp->processEvents();
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(wizard.button(QWizard::FinishButton)->isEnabled());

    wizard.next();
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.back();
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());

    wizard.setOption(QWizard::NoDefaultButton, true);
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.next();
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.back();
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.setOption(QWizard::NoDefaultButton, false);
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
}

void tst_QWizard::setOption_NoBackButtonOnStartPage()
{
    QWizard wizard;
    wizard.setOption(QWizard::NoBackButtonOnStartPage, true);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);

    wizard.setStartId(1);
    wizard.show();
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, true);
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

    wizard.next();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, true);
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.back();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
}

void tst_QWizard::setOption_NoBackButtonOnLastPage()
{
    for (int i = 0; i < 2; ++i) {
        QWizard wizard;
        wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
        wizard.setOption(QWizard::NoBackButtonOnLastPage, true);
        wizard.setOption(QWizard::DisabledBackButtonOnLastPage, i == 0);    // changes nothing
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.page(1)->setFinalPage(true);     // changes nothing (final != last in general)
        wizard.addPage(new QWizardPage);

        wizard.setStartId(1);
        wizard.show();
        qApp->processEvents();

        QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.back();
        qApp->processEvents();
        QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.setOption(QWizard::NoBackButtonOnLastPage, false);
        QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

        wizard.setOption(QWizard::NoBackButtonOnLastPage, true);
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.addPage(new QWizardPage);
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());  // this is maybe wrong
    }
}

void tst_QWizard::setOption_DisabledBackButtonOnLastPage()
{
    QWizard wizard;
    wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
    wizard.setOption(QWizard::DisabledBackButtonOnLastPage, true);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.page(1)->setFinalPage(true);     // changes nothing (final != last in general)
    wizard.addPage(new QWizardPage);

    wizard.setStartId(1);
    wizard.show();
    qApp->processEvents();

    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.back();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.setOption(QWizard::DisabledBackButtonOnLastPage, false);
    QVERIFY(wizard.button(QWizard::BackButton)->isEnabled());

    wizard.setOption(QWizard::DisabledBackButtonOnLastPage, true);
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.addPage(new QWizardPage);
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());  // this is maybe wrong
}

void tst_QWizard::setOption_HaveNextButtonOnLastPage()
{
    QWizard wizard;
    wizard.setOption(QWizard::HaveNextButtonOnLastPage, false);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.page(1)->setFinalPage(true);     // changes nothing (final != last in general)
    wizard.addPage(new QWizardPage);

    wizard.setStartId(1);
    wizard.show();
    qApp->processEvents();

    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());

    wizard.next();
    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());

    wizard.setOption(QWizard::HaveNextButtonOnLastPage, true);
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());

    wizard.next();
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());

    wizard.back();
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());

    wizard.setOption(QWizard::HaveNextButtonOnLastPage, false);
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());

    wizard.next();
    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());

    wizard.setOption(QWizard::HaveNextButtonOnLastPage, true);
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());
}

void tst_QWizard::setOption_HaveFinishButtonOnEarlyPages()
{
    QWizard wizard;
    QVERIFY(!wizard.testOption(QWizard::HaveFinishButtonOnEarlyPages));
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.page(1)->setFinalPage(true);
    wizard.addPage(new QWizardPage);

    wizard.show();
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::FinishButton)->isVisible());

    wizard.next();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.next();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.back();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::FinishButton)->isVisible());

    wizard.setOption(QWizard::HaveFinishButtonOnEarlyPages, true);
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.next();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.setOption(QWizard::HaveFinishButtonOnEarlyPages, false);
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::FinishButton)->isVisible());
}

void tst_QWizard::setOption_NoCancelButton()
{
    for (int i = 0; i < 2; ++i) {
        QWizard wizard;
        wizard.setOption(QWizard::NoCancelButton, true);
        wizard.setOption(QWizard::CancelButtonOnLeft, i == 0);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.next();
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.setOption(QWizard::NoCancelButton, false);
        QVERIFY(wizard.button(QWizard::CancelButton)->isVisible());

        wizard.back();
        QVERIFY(wizard.button(QWizard::CancelButton)->isVisible());

        wizard.setOption(QWizard::NoCancelButton, true);
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());
    }
}

void tst_QWizard::setOption_NoCancelButtonOnLastPage()
{
    for (int i = 0; i < 2; ++i) {
        QWizard wizard;
        wizard.setOption(QWizard::NoCancelButton, false);
        wizard.setOption(QWizard::NoCancelButtonOnLastPage, true);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.page(1)->setFinalPage(true);     // changes nothing (final != last in general)
        wizard.addPage(new QWizardPage);

        wizard.setStartId(1);
        wizard.show();
        qApp->processEvents();

        QVERIFY(wizard.button(QWizard::CancelButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.back();
        qApp->processEvents();
        QVERIFY(wizard.button(QWizard::CancelButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.setOption(QWizard::NoCancelButtonOnLastPage, false);
        QVERIFY(wizard.button(QWizard::CancelButton)->isVisible());

        wizard.setOption(QWizard::NoCancelButtonOnLastPage, true);
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.addPage(new QWizardPage);
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());  // this is maybe wrong
    }
}

void tst_QWizard::setOption_CancelButtonOnLeft()
{
    for (int i = 0; i < 2; ++i) {
        int sign = (i == 0) ? +1 : -1;

        QWizard wizard;
        wizard.setLayoutDirection(i == 0 ? Qt::LeftToRight : Qt::RightToLeft);
        wizard.setOption(QWizard::NoCancelButton, false);
        wizard.setOption(QWizard::CancelButtonOnLeft, true);
        wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        const QAbstractButton *refButton = wizard.button((wizard.wizardStyle() == QWizard::AeroStyle)
            ? QWizard::NextButton : QWizard::BackButton);
        const QAbstractButton *refButton2 = wizard.button((wizard.wizardStyle() == QWizard::AeroStyle)
            ? QWizard::FinishButton : QWizard::BackButton);

        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x() < sign * refButton->x());

        wizard.next();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x() < sign * refButton->x());

        wizard.setOption(QWizard::CancelButtonOnLeft, false);
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x() > sign * refButton2->x());

        wizard.back();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x() > sign * refButton->x());
    }
}

void tst_QWizard::setOption_HaveHelpButton()
{
    for (int i = 0; i < 2; ++i) {
        QWizard wizard;
        QVERIFY(!wizard.testOption(QWizard::HaveHelpButton));
        wizard.setOption(QWizard::HaveHelpButton, false);
        wizard.setOption(QWizard::HelpButtonOnRight, i == 0);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());

        wizard.next();
        QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());

        wizard.setOption(QWizard::HaveHelpButton, true);
        QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

        wizard.back();
        QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

        wizard.setOption(QWizard::HaveHelpButton, false);
        QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());
    }
}

void tst_QWizard::setOption_HelpButtonOnRight()
{
    for (int i = 0; i < 2; ++i) {
        int sign = (i == 0) ? +1 : -1;

        QWizard wizard;
        wizard.setLayoutDirection(i == 0 ? Qt::LeftToRight : Qt::RightToLeft);
        wizard.setOption(QWizard::HaveHelpButton, true);
        wizard.setOption(QWizard::HelpButtonOnRight, false);
        wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        const QAbstractButton *refButton = wizard.button((wizard.wizardStyle() == QWizard::AeroStyle)
            ? QWizard::NextButton : QWizard::BackButton);

        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x() < sign * refButton->x());

        wizard.next();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x() < sign * refButton->x());

        wizard.setOption(QWizard::HelpButtonOnRight, true);
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x() > sign * refButton->x());

        wizard.back();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x() > sign * refButton->x());
    }
}

void tst_QWizard::setOption_HaveCustomButtonX()
{
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                QWizard wizard;
                wizard.setLayoutDirection(Qt::LeftToRight);
                wizard.addPage(new QWizardPage);
                wizard.addPage(new QWizardPage);
                wizard.show();

                wizard.setButtonText(QWizard::CustomButton1, "Foo");
                wizard.setButton(QWizard::CustomButton2, new QCheckBox("Bar"));
                wizard.button(QWizard::CustomButton3)->setText("Baz");

                wizard.setOption(QWizard::HaveCustomButton1, i == 0);
                wizard.setOption(QWizard::HaveCustomButton2, j == 0);
                wizard.setOption(QWizard::HaveCustomButton3, k == 0);

                QVERIFY(wizard.button(QWizard::CustomButton1)->isHidden() == (i != 0));
                QVERIFY(wizard.button(QWizard::CustomButton2)->isHidden() == (j != 0));
                QVERIFY(wizard.button(QWizard::CustomButton3)->isHidden() == (k != 0));

                if (i + j + k == 0) {
                    qApp->processEvents();
                    QVERIFY(wizard.button(QWizard::CustomButton1)->x()
                            < wizard.button(QWizard::CustomButton2)->x());
                    QVERIFY(wizard.button(QWizard::CustomButton2)->x()
                            < wizard.button(QWizard::CustomButton3)->x());
                }
            }
        }
    }
}

class Operation
{
public:
    virtual void apply(QWizard *) const = 0;
    virtual QString describe() const = 0;
protected:
    virtual ~Operation() {}
};

class SetPage : public Operation
{
    void apply(QWizard *wizard) const
    {
        wizard->restart();
        for (int j = 0; j < page; ++j)
            wizard->next();
    }
    QString describe() const { return QLatin1String("set page ") + QString::number(page); }
    int page;
public:
    static QSharedPointer<SetPage> create(int page)
    {
        QSharedPointer<SetPage> o = QSharedPointer<SetPage>::create();
        o->page = page;
        return o;
    }
};

class SetStyle : public Operation
{
    void apply(QWizard *wizard) const { wizard->setWizardStyle(style); }
    QString describe() const { return QLatin1String("set style ") + QString::number(style); }
    QWizard::WizardStyle style;
public:
    static QSharedPointer<SetStyle> create(QWizard::WizardStyle style)
    {
        QSharedPointer<SetStyle> o = QSharedPointer<SetStyle>::create();
        o->style = style;
        return o;
    }
};

class SetOption : public Operation
{
    void apply(QWizard *wizard) const { wizard->setOption(option, on); }
    QString describe() const;
    QWizard::WizardOption option;
    bool on;
public:
    static QSharedPointer<SetOption> create(QWizard::WizardOption option, bool on)
    {
        QSharedPointer<SetOption> o = QSharedPointer<SetOption>::create();
        o->option = option;
        o->on = on;
        return o;
    }
};

class OptionInfo
{
    OptionInfo()
    {
        tags[QWizard::IndependentPages]             = "0/IPP";
        tags[QWizard::IgnoreSubTitles]              = "1/IST";
        tags[QWizard::ExtendedWatermarkPixmap]      = "2/EWP";
        tags[QWizard::NoDefaultButton]              = "3/NDB";
        tags[QWizard::NoBackButtonOnStartPage]      = "4/BSP";
        tags[QWizard::NoBackButtonOnLastPage]       = "5/BLP";
        tags[QWizard::DisabledBackButtonOnLastPage] = "6/DLP";
        tags[QWizard::HaveNextButtonOnLastPage]     = "7/NLP";
        tags[QWizard::HaveFinishButtonOnEarlyPages] = "8/FEP";
        tags[QWizard::NoCancelButton]               = "9/NCB";
        tags[QWizard::CancelButtonOnLeft]           = "10/CBL";
        tags[QWizard::HaveHelpButton]               = "11/HHB";
        tags[QWizard::HelpButtonOnRight]            = "12/HBR";
        tags[QWizard::HaveCustomButton1]            = "13/CB1";
        tags[QWizard::HaveCustomButton2]            = "14/CB2";
        tags[QWizard::HaveCustomButton3]            = "15/CB3";

        for (int i = 0; i < 2; ++i) {
            QMap<QWizard::WizardOption, QSharedPointer<Operation> > operations_;
            foreach (QWizard::WizardOption option, tags.keys())
                operations_[option] = SetOption::create(option, i == 1);
            operations << operations_;
        }
    }
    OptionInfo(OptionInfo const&);
    OptionInfo& operator=(OptionInfo const&);
    QMap<QWizard::WizardOption, QString> tags;
    QList<QMap<QWizard::WizardOption, QSharedPointer<Operation> > > operations;
public:
    static OptionInfo &instance()
    {
        static OptionInfo optionInfo;
        return optionInfo;
    }

    QString tag(QWizard::WizardOption option) const { return tags.value(option); }
    QSharedPointer<Operation> operation(QWizard::WizardOption option, bool on) const
    { return operations.at(on).value(option); }
    QList<QWizard::WizardOption> options() const { return tags.keys(); }
};

QString SetOption::describe() const
{
    return QLatin1String("set opt ") + OptionInfo::instance().tag(option)
        + QLatin1Char(on ? '1' : '0');
}

Q_DECLARE_METATYPE(QVector<QSharedPointer<Operation> >)

class TestGroup
{
public:
    enum Type {Equality, NonEquality};

    TestGroup(const QString &name = QString("no name"), Type type = Equality)
        : name(name), type(type), nRows_(0) {}

    void reset(const QString &name, Type type = Equality)
    {
        this->name = name;
        this->type = type;
        combinations.clear();
    }

    QVector<QSharedPointer<Operation> > &add()
    {
        combinations.resize(combinations.size() + 1);
        return combinations.last();
    }

    void createTestRows()
    {
        for (int i = 0; i < combinations.count(); ++i) {
            QTest::newRow((name.toLatin1() + ", row " + QByteArray::number(i)).constData())
                << (i == 0) << (type == Equality) << combinations.at(i);
            ++nRows_;
        }
    }

    int nRows() const { return nRows_; }

private:
    QString name;
    Type type;
    int nRows_;
    QVector<QVector<QSharedPointer<Operation> > > combinations;
};

class IntroPage : public QWizardPage
{
    Q_OBJECT
public:
    IntroPage()
    {
        setTitle(tr("Intro"));
        setSubTitle(tr("Intro Subtitle"));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(new QLabel(tr("Intro Label")));
        setLayout(layout);
    }
};

class MiddlePage : public QWizardPage
{
    Q_OBJECT
public:
    MiddlePage()
    {
        setTitle(tr("Middle"));
        setSubTitle(tr("Middle Subtitle"));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(new QLabel(tr("Middle Label")));
        setLayout(layout);
    }
};

class ConclusionPage : public QWizardPage
{
    Q_OBJECT
public:
    ConclusionPage()
    {
        setTitle(tr("Conclusion"));
        setSubTitle(tr("Conclusion Subtitle"));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(new QLabel(tr("Conclusion Label")));
        setLayout(layout);
    }
};

class TestWizard : public QWizard
{
    Q_OBJECT
    QList<int> pageIds;
    QString opsDescr;
public:
    TestWizard()
    {
        setPixmap(QWizard::BannerPixmap, QPixmap(":/images/banner.png"));
        setPixmap(QWizard::BackgroundPixmap, QPixmap(":/images/background.png"));
        setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));
        setPixmap(QWizard::LogoPixmap, QPixmap(":/images/logo.png"));
        setButtonText(QWizard::CustomButton1, "custom 1");
        setButtonText(QWizard::CustomButton2, "custom 2");
        setButtonText(QWizard::CustomButton3, "custom 3");
        pageIds << addPage(new IntroPage);
        pageIds << addPage(new MiddlePage);
        pageIds << addPage(new ConclusionPage);

        // Disable antialiased font rendering since this may sometimes result in tiny
        // and (apparent) non-deterministic pixel variations between images expected to be
        // identical. This may only be a problem on X11.
        QFont f = font();
        f.setStyleStrategy(QFont::NoAntialias);
        setFont(f);

        // ### Required to work with a deficiency(?) in QWizard:
//        setFixedSize(800, 600);
    }

    ~TestWizard()
    {
        foreach (int id, pageIds) {
            QWizardPage *page_to_delete = page(id);
            removePage(id);
            delete page_to_delete;
        }
    }

    void applyOperations(const QVector<QSharedPointer<Operation> > &operations)
    {
        foreach (const QSharedPointer<Operation> &op, operations) {
            if (op) {
                op->apply(this);
                opsDescr += QLatin1Char('(') + op->describe() + QLatin1String(") ");
            }
        }
    }

    QImage createImage() const
    {
        return const_cast<TestWizard *>(this)->grab()
               .toImage().convertToFormat(QImage::Format_ARGB32);
    }

    QString operationsDescription() const { return opsDescr; }
};

class CombinationsTestData
{
    TestGroup testGroup;
    QVector<QSharedPointer<Operation> > pageOps;
    QVector<QSharedPointer<Operation> > styleOps;
    QMap<bool, QVector<QSharedPointer<Operation> > > setAllOptions;
public:
    CombinationsTestData()
    {
        QTest::addColumn<bool>("ref");
        QTest::addColumn<bool>("testEquality");
        QTest::addColumn<QVector<QSharedPointer<Operation> > >("operations");
        pageOps << SetPage::create(0) << SetPage::create(1) << SetPage::create(2);
        styleOps << SetStyle::create(QWizard::ClassicStyle) << SetStyle::create(QWizard::ModernStyle)
                 << SetStyle::create(QWizard::MacStyle);
#define SETPAGE(page) pageOps.at(page)
#define SETSTYLE(style) styleOps.at(style)
#define OPT(option, on) OptionInfo::instance().operation(option, on)
#define CLROPT(option) OPT(option, false)
#define SETOPT(option) OPT(option, true)
        foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
            setAllOptions[false] << CLROPT(option);
            setAllOptions[true]  << SETOPT(option);
        }
#define CLRALLOPTS setAllOptions.value(false)
#define SETALLOPTS setAllOptions.value(true)
    }

    int nRows() const { return testGroup.nRows(); }

    // Creates "all" possible test rows. (WARNING: This typically makes the test take too long!)
    void createAllTestRows()
    {
        testGroup.reset("testAll 1.1");
        testGroup.add(); // i.e. no operations applied!
        testGroup.add() << SETPAGE(0);
        testGroup.add() << SETSTYLE(0);
        testGroup.add() << SETPAGE(0) << SETSTYLE(0);
        testGroup.add() << SETSTYLE(0) << SETPAGE(0);
        testGroup.createTestRows();

        testGroup.reset("testAll 2.1");
        testGroup.add();
        testGroup.add() << CLRALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.2");
        testGroup.add() << SETALLOPTS;
        testGroup.add() << SETALLOPTS << SETALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.3");
        testGroup.add() << CLRALLOPTS;
        testGroup.add() << CLRALLOPTS << CLRALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.4");
        testGroup.add() << CLRALLOPTS;
        testGroup.add() << SETALLOPTS << CLRALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.5");
        testGroup.add() << SETALLOPTS;
        testGroup.add() << CLRALLOPTS << SETALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.6");
        testGroup.add() << SETALLOPTS;
        testGroup.add() << SETALLOPTS << CLRALLOPTS << SETALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.7");
        testGroup.add() << CLRALLOPTS;
        testGroup.add() << CLRALLOPTS << SETALLOPTS << CLRALLOPTS;
        testGroup.createTestRows();

        for (int i = 0; i < 2; ++i) {
            QVector<QSharedPointer<Operation> > setOptions = setAllOptions.value(i == 1);

            testGroup.reset("testAll 3.1");
            testGroup.add() << setOptions;
            testGroup.add() << SETPAGE(0) << setOptions;
            testGroup.add() << setOptions << SETPAGE(0);
            testGroup.add() << SETSTYLE(0) << setOptions;
            testGroup.add() << setOptions << SETSTYLE(0);
            testGroup.add() << setOptions << SETPAGE(0) << SETSTYLE(0);
            testGroup.add() << SETPAGE(0) << setOptions << SETSTYLE(0);
            testGroup.add() << SETPAGE(0) << SETSTYLE(0) << setOptions;
            testGroup.add() << setOptions << SETSTYLE(0) << SETPAGE(0);
            testGroup.add() << SETSTYLE(0) << setOptions << SETPAGE(0);
            testGroup.add() << SETSTYLE(0) << SETPAGE(0) << setOptions;
            testGroup.createTestRows();
        }

        foreach (const QSharedPointer<Operation> &pageOp, pageOps) {
            testGroup.reset("testAll 4.1");
            testGroup.add() << pageOp;
            testGroup.add() << pageOp << pageOp;
            testGroup.createTestRows();

            for (int i = 0; i < 2; ++i) {
                QVector<QSharedPointer<Operation> > optionOps = setAllOptions.value(i == 1);
                testGroup.reset("testAll 4.2");
                testGroup.add() << optionOps << pageOp;
                testGroup.add() << pageOp << optionOps;
                testGroup.createTestRows();

                foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
                    QSharedPointer<Operation> optionOp = OPT(option, i == 1);
                    testGroup.reset("testAll 4.3");
                    testGroup.add() << optionOp << pageOp;
                    testGroup.add() << pageOp << optionOp;
                    testGroup.createTestRows();
                }
            }
        }

        foreach (const QSharedPointer<Operation> &styleOp, styleOps) {
            testGroup.reset("testAll 5.1");
            testGroup.add() << styleOp;
            testGroup.add() << styleOp << styleOp;
            testGroup.createTestRows();

            for (int i = 0; i < 2; ++i) {
                QVector<QSharedPointer<Operation> > optionOps = setAllOptions.value(i == 1);
                testGroup.reset("testAll 5.2");
                testGroup.add() << optionOps << styleOp;
                testGroup.add() << styleOp << optionOps;
                testGroup.createTestRows();

                foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
                    QSharedPointer<Operation> optionOp = OPT(option, i == 1);
                    testGroup.reset("testAll 5.3");
                    testGroup.add() << optionOp << styleOp;
                    testGroup.add() << styleOp << optionOp;
                    testGroup.createTestRows();
                }
            }
        }

        foreach (const QSharedPointer<Operation> &pageOp, pageOps) {
            foreach (const QSharedPointer<Operation> &styleOp, styleOps) {

                testGroup.reset("testAll 6.1");
                testGroup.add() << pageOp;
                testGroup.add() << pageOp << pageOp;
                testGroup.createTestRows();

                testGroup.reset("testAll 6.2");
                testGroup.add() << styleOp;
                testGroup.add() << styleOp << styleOp;
                testGroup.createTestRows();

                testGroup.reset("testAll 6.3");
                testGroup.add() << pageOp << styleOp;
                testGroup.add() << styleOp << pageOp;
                testGroup.createTestRows();

                for (int i = 0; i < 2; ++i) {
                    QVector<QSharedPointer<Operation> > optionOps = setAllOptions.value(i == 1);
                    testGroup.reset("testAll 6.4");
                    testGroup.add() << optionOps << pageOp << styleOp;
                    testGroup.add() << pageOp << optionOps << styleOp;
                    testGroup.add() << pageOp << styleOp << optionOps;
                    testGroup.add() << optionOps << styleOp << pageOp;
                    testGroup.add() << styleOp << optionOps << pageOp;
                    testGroup.add() << styleOp << pageOp << optionOps;
                    testGroup.createTestRows();

                    foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
                        QSharedPointer<Operation> optionOp = OPT(option, i == 1);
                        testGroup.reset("testAll 6.5");
                        testGroup.add() << optionOp << pageOp << styleOp;
                        testGroup.add() << pageOp << optionOp << styleOp;
                        testGroup.add() << pageOp << styleOp << optionOp;
                        testGroup.add() << optionOp << styleOp << pageOp;
                        testGroup.add() << styleOp << optionOp << pageOp;
                        testGroup.add() << styleOp << pageOp << optionOp;
                        testGroup.createTestRows();
                    }
                }
            }
        }

        testGroup.reset("testAll 7.1", TestGroup::NonEquality);
        testGroup.add() << SETPAGE(0);
        testGroup.add() << SETPAGE(1);
        testGroup.add() << SETPAGE(2);
        testGroup.createTestRows();

        testGroup.reset("testAll 7.2", TestGroup::NonEquality);
        testGroup.add() << SETSTYLE(0);
        testGroup.add() << SETSTYLE(1);
        testGroup.add() << SETSTYLE(2);
        testGroup.createTestRows();

        // more to follow ...
    }

    // Creates a "small" number of interesting test rows.
    void createTestRows1()
    {
        testGroup.reset("test1 1");
        testGroup.add() << SETPAGE(0) << SETOPT(QWizard::HaveCustomButton3);
        testGroup.add() << SETOPT(QWizard::HaveCustomButton3);
        testGroup.createTestRows();

        testGroup.reset("test1 2");
        testGroup.add() << SETOPT(QWizard::HaveFinishButtonOnEarlyPages) << SETPAGE(0);
        testGroup.add() << SETPAGE(0) << SETOPT(QWizard::HaveFinishButtonOnEarlyPages);
        testGroup.createTestRows();

        testGroup.reset("test1 3");
        testGroup.add() << SETPAGE(2) << SETOPT(QWizard::HaveNextButtonOnLastPage);
        testGroup.add() << SETOPT(QWizard::HaveNextButtonOnLastPage) << SETPAGE(2);
        testGroup.createTestRows();
    }
};

void tst_QWizard::combinations_data()
{
    CombinationsTestData combTestData;
//    combTestData.createAllTestRows();
    combTestData.createTestRows1();

//    qDebug() << "test rows:" << combTestData.nRows();
}

void tst_QWizard::combinations()
{
    QFETCH(bool, ref);
    QFETCH(bool, testEquality);
    QFETCH(QVector<QSharedPointer<Operation> >, operations);

    TestWizard wizard;
#if !defined(QT_NO_STYLE_WINDOWSVISTA)
    if (wizard.wizardStyle() == QWizard::AeroStyle)
        return; // ### TODO: passes/fails in a unpredictable way, so disable for now
#endif
    wizard.applyOperations(operations);
    wizard.show(); // ### TODO: Required, but why? Should wizard.createImage() care?

    static QImage refImage;
    static QSize refMinSize;
    static QString refDescr;

    if (ref) {
        refImage = wizard.createImage();
        refMinSize = wizard.minimumSizeHint();
        refDescr = wizard.operationsDescription();
        return;
    }

    QImage image = wizard.createImage();

    bool minSizeTest = wizard.minimumSizeHint() != refMinSize;
    bool imageTest = image != refImage;
    QLatin1String otor("!=");
    QLatin1String reason("differ");

    if (!testEquality) {
        minSizeTest = false; // the image test is sufficient!
        imageTest = !imageTest;
        otor = QLatin1String("==");
        reason = QLatin1String("are equal");
    }

    if (minSizeTest)
        qDebug() << "minimum sizes" << reason.latin1() << ';' << wizard.minimumSizeHint()
                 << otor.latin1() << refMinSize;

    if (imageTest)
        qDebug() << "images" << reason.latin1();

    if (minSizeTest || imageTest) {
        qDebug() << "\t      row 0 operations:" << refDescr.toLatin1();
        qDebug() << "\tcurrent row operations:" << wizard.operationsDescription().toLatin1();
        QVERIFY(false);
    }
}

class WizardPage : public QWizardPage
{
    Q_OBJECT
    bool shown_;
    void showEvent(QShowEvent *) { shown_ = true; }
    void hideEvent(QHideEvent *) { shown_ = false; }
public:
    WizardPage() : shown_(false) {}
    bool shown() const { return shown_; }
};

class WizardPages
{
    QList<WizardPage *> pages;
public:
    void add(WizardPage *page) { pages << page; }
    QList<WizardPage *> all() const { return pages; }
    QList<WizardPage *> shown() const
    {
        QList<WizardPage *> result;
        foreach (WizardPage *page, pages)
            if (page->shown())
                result << page;
        return result;
    }
};

void tst_QWizard::showCurrentPageOnly()
{
    QWizard wizard;
    WizardPages pages;
    for (int i = 0; i < 5; ++i) {
        pages.add(new WizardPage);
        wizard.addPage(pages.all().last());
    }

    wizard.show();

    QCOMPARE(pages.shown().count(), 1);
    QCOMPARE(pages.shown().first(), pages.all().first());

    const int steps = 2;
    for (int i = 0; i < steps; ++i)
        wizard.next();

    QCOMPARE(pages.shown().count(), 1);
    QCOMPARE(pages.shown().first(), pages.all().at(steps));

    wizard.restart();

    QCOMPARE(pages.shown().count(), 1);
    QCOMPARE(pages.shown().first(), pages.all().first());
}

void tst_QWizard::setButtonText()
{
    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);
    QWizardPage* page1 = new QWizardPage;
    QWizardPage* page2 = new QWizardPage;
    wizard.addPage(page1);
    wizard.addPage(page2);

    wizard.show();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page2->buttonText(QWizard::NextButton).contains("Next"));

    page2->setButtonText(QWizard::NextButton, "Page2");
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.next();
    qApp->processEvents();
    QCOMPARE(wizard.button(QWizard::NextButton)->text(), QString("Page2"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.back();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.setButtonText(QWizard::NextButton, "Wizard");
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Wizard"));
    QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("Wizard"));
    QCOMPARE(page1->buttonText(QWizard::NextButton), QString("Wizard"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.next();
    qApp->processEvents();
    QCOMPARE(wizard.button(QWizard::NextButton)->text(), QString("Page2"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Wizard"));
    QCOMPARE(page1->buttonText(QWizard::NextButton), QString("Wizard"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));
}

void tst_QWizard::setCommitPage()
{
    QWizard wizard;
    QWizardPage* page1 = new QWizardPage;
    QWizardPage* page2 = new QWizardPage;
    wizard.addPage(page1);
    wizard.addPage(page2);
    wizard.show();
    qApp->processEvents();

    QVERIFY(!page1->isCommitPage());
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    QVERIFY(wizard.button(QWizard::BackButton)->isEnabled());

    page1->setCommitPage(true);
    QVERIFY(page1->isCommitPage());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    page1->setCommitPage(false);
    QVERIFY(!page1->isCommitPage());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    QVERIFY(wizard.button(QWizard::BackButton)->isEnabled());

    // ### test relabeling of the Cancel button to "Close" once this is implemented
}

void tst_QWizard::setWizardStyle()
{
    QWizard wizard;
    wizard.addPage(new QWizardPage);
    wizard.show();
    qApp->processEvents();

    // defaults
    const bool styleHintMatch =
        wizard.wizardStyle() ==
        QWizard::WizardStyle(wizard.style()->styleHint(QStyle::SH_WizardStyle, 0, &wizard));
#if !defined(QT_NO_STYLE_WINDOWSVISTA)
    QVERIFY(styleHintMatch || wizard.wizardStyle() == QWizard::AeroStyle);
#else
    QVERIFY(styleHintMatch);
#endif

    // set/get consistency
    for (int wstyle = 0; wstyle < QWizard::NStyles; ++wstyle) {
        wizard.setWizardStyle((QWizard::WizardStyle)wstyle);
        QCOMPARE((int)wizard.wizardStyle(), wstyle);
    }
}

void tst_QWizard::removePage()
{
    QWizard wizard;
    QWizardPage *page0 = new QWizardPage;
    QWizardPage *page1 = new QWizardPage;
    QWizardPage *page2 = new QWizardPage;
    QWizardPage *page3 = new QWizardPage;
    QSignalSpy spy(&wizard, SIGNAL(pageRemoved(int)));

    wizard.setPage(0, page0);
    wizard.setPage(1, page1);
    wizard.setPage(2, page2);
    wizard.setPage(3, page3);

    wizard.restart();
    QCOMPARE(wizard.pageIds().size(), 4);
    QCOMPARE(wizard.visitedPages().size(), 1);
    QCOMPARE(spy.count(), 0);

    // Removing a non-existent page
    wizard.removePage(4);
    QCOMPARE(wizard.pageIds().size(), 4);
    QCOMPARE(spy.count(), 0);

    // Removing and then reinserting a page
    QCOMPARE(wizard.pageIds().size(), 4);
    QVERIFY(wizard.pageIds().contains(2));
    wizard.removePage(2);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 2);
    QCOMPARE(wizard.pageIds().size(), 3);
    QVERIFY(!wizard.pageIds().contains(2));
    wizard.setPage(2, page2);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(wizard.pageIds().size(), 4);
    QVERIFY(wizard.pageIds().contains(2));

    // Removing the same page twice
    wizard.removePage(2); // restore
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 2);
    QCOMPARE(wizard.pageIds().size(), 3);
    QVERIFY(!wizard.pageIds().contains(2));
    wizard.removePage(2);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(wizard.pageIds().size(), 3);
    QVERIFY(!wizard.pageIds().contains(2));

    // Removing a page not in the history
    wizard.setPage(2, page2); // restore
    wizard.restart();
    wizard.next();
    QCOMPARE(wizard.visitedPages().size(), 2);
    QCOMPARE(wizard.currentPage(), page1);
    QCOMPARE(spy.count(), 0);
    wizard.removePage(2);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 2);
    QCOMPARE(wizard.visitedPages().size(), 2);
    QVERIFY(!wizard.pageIds().contains(2));
    QCOMPARE(wizard.currentPage(), page1);

    // Removing a page in the history before the current page
    wizard.setPage(2, page2); // restore
    wizard.restart();
    wizard.next();
    QCOMPARE(spy.count(), 0);
    QCOMPARE(wizard.visitedPages().size(), 2);
    QCOMPARE(wizard.currentPage(), page1);
    wizard.removePage(0);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 0);
    QCOMPARE(wizard.visitedPages().size(), 1);
    QVERIFY(!wizard.visitedPages().contains(0));
    QVERIFY(!wizard.pageIds().contains(0));
    QCOMPARE(wizard.currentPage(), page1);

    // Remove the current page which is not the first one in the history
    wizard.setPage(0, page0); // restore
    wizard.restart();
    wizard.next();
    QCOMPARE(spy.count(), 0);
    QCOMPARE(wizard.visitedPages().size(), 2);
    QCOMPARE(wizard.currentPage(), page1);
    wizard.removePage(1);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 1);
    QCOMPARE(wizard.visitedPages().size(), 1);
    QVERIFY(!wizard.visitedPages().contains(1));
    QVERIFY(!wizard.pageIds().contains(1));
    QCOMPARE(wizard.currentPage(), page0);

    // Remove the current page which is the first (and only) one in the history
    wizard.removePage(0);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 0);
    QCOMPARE(wizard.visitedPages().size(), 1);
    QVERIFY(!wizard.visitedPages().contains(0));
    QCOMPARE(wizard.pageIds().size(), 2);
    QVERIFY(!wizard.pageIds().contains(0));
    QCOMPARE(wizard.currentPage(), page2);
    //
    wizard.removePage(2);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 2);
    QCOMPARE(wizard.visitedPages().size(), 1);
    QVERIFY(!wizard.visitedPages().contains(2));
    QCOMPARE(wizard.pageIds().size(), 1);
    QVERIFY(!wizard.pageIds().contains(2));
    QCOMPARE(wizard.currentPage(), page3);
    //
    wizard.removePage(3);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 3);
    QVERIFY(wizard.visitedPages().empty());
    QVERIFY(wizard.pageIds().empty());
    QCOMPARE(wizard.currentPage(), nullptr);
}

void tst_QWizard::sideWidget()
{
    QWizard wizard;

    wizard.setSideWidget(0);
    QVERIFY(!wizard.sideWidget());
    QScopedPointer<QWidget> w1(new QWidget(&wizard));
    wizard.setSideWidget(w1.data());
    QCOMPARE(wizard.sideWidget(), w1.data());
    QWidget *w2 = new QWidget(&wizard);
    wizard.setSideWidget(w2);
    QCOMPARE(wizard.sideWidget(), w2);
    QVERIFY(w1->parent() != 0);
    QCOMPARE(w1->window(), static_cast<QWidget *>(&wizard));
    QCOMPARE(w2->window(), static_cast<QWidget *>(&wizard));
    w1->setParent(0);
    wizard.setSideWidget(0);
    QVERIFY(!wizard.sideWidget());
}

void tst_QWizard::objectNames_data()
{
    QTest::addColumn<QWizard::WizardButton>("wizardButton");
    QTest::addColumn<QString>("buttonName");

    QTest::newRow("BackButton")    << QWizard::BackButton    << QStringLiteral("__qt__passive_wizardbutton0");
    QTest::newRow("NextButton")    << QWizard::NextButton    << QStringLiteral("__qt__passive_wizardbutton1");
    QTest::newRow("CommitButton")  << QWizard::CommitButton  << QStringLiteral("qt_wizard_commit");
    QTest::newRow("FinishButton")  << QWizard::FinishButton  << QStringLiteral("qt_wizard_finish");
    QTest::newRow("CancelButton")  << QWizard::CancelButton  << QStringLiteral("qt_wizard_cancel");
    QTest::newRow("HelpButton")    << QWizard::HelpButton    << QStringLiteral("__qt__passive_wizardbutton5");
    QTest::newRow("CustomButton1") << QWizard::CustomButton1 << QStringLiteral("__qt__passive_wizardbutton6");
    QTest::newRow("CustomButton2") << QWizard::CustomButton2 << QStringLiteral("__qt__passive_wizardbutton7");
    QTest::newRow("CustomButton3") << QWizard::CustomButton3 << QStringLiteral("__qt__passive_wizardbutton8");
}

void tst_QWizard::objectNames()
{
    QFETCH(QWizard::WizardButton, wizardButton);
    QFETCH(QString, buttonName);

    QWizard wizard;
    QList<QWizard::WizardButton> buttons = QList<QWizard::WizardButton>()
        << QWizard::BackButton
        << QWizard::NextButton
        << QWizard::CommitButton
        << QWizard::FinishButton
        << QWizard::CancelButton
        << QWizard::HelpButton
        << QWizard::CustomButton1
        << QWizard::CustomButton2
        << QWizard::CustomButton3
      ;
    QVERIFY(buttons.contains(wizardButton));
    QVERIFY(wizard.button(wizardButton));
    QCOMPARE(wizard.button(wizardButton)->objectName(), buttonName);
}

class task177716_CommitPage : public QWizardPage
{
    Q_OBJECT
public:
    task177716_CommitPage()
    {
        setCommitPage(true);
        QVBoxLayout *layout = new QVBoxLayout;
        ledit = new QLineEdit(this);
        registerField("foo*", ledit);
        layout->addWidget(ledit);
        setLayout(layout);
    }
    QLineEdit *ledit;
};

void tst_QWizard::task177716_disableCommitButton()
{
    QWizard wizard;
    task177716_CommitPage *commitPage = new task177716_CommitPage;
    wizard.addPage(commitPage);
    // the following page must be there to prevent the first page from replacing the Commit button
    // with the Finish button:
    wizard.addPage(new QWizardPage);
    wizard.show();
    QVERIFY(!wizard.button(QWizard::CommitButton)->isEnabled());
    commitPage->ledit->setText("some non-empty text");
    QVERIFY(wizard.button(QWizard::CommitButton)->isEnabled());
    commitPage->ledit->setText("");
    QVERIFY(!wizard.button(QWizard::CommitButton)->isEnabled());
}

class WizardPage_task183550 : public QWizardPage
{
public:
    WizardPage_task183550(QWidget *parent = 0)
        : QWizardPage(parent)
        , treeWidget(new QTreeWidget)
        , verticalPolicy(QSizePolicy::MinimumExpanding) {}
    void enableVerticalExpansion() { verticalPolicy = QSizePolicy::MinimumExpanding; }
    void disableVerticalExpansion() { verticalPolicy = QSizePolicy::Preferred; }
    int treeWidgetHeight() const { return treeWidget->height(); }
    int treeWidgetSizeHintHeight() const { return treeWidget->sizeHint().height(); }

private:
    QTreeWidget *treeWidget;
    QSizePolicy::Policy verticalPolicy;

    void initializePage()
    {
        if (layout())
            delete layout();
        if (treeWidget)
            delete treeWidget;

        QLayout *layout_ = new QVBoxLayout(this);
        layout_->addWidget(treeWidget = new QTreeWidget);

        QSizePolicy policy = sizePolicy();
        policy.setVerticalPolicy(verticalPolicy);
        treeWidget->setSizePolicy(policy);
    }
};

void tst_QWizard::task183550_stretchFactor()
{
    QWizard wizard;
    WizardPage_task183550 *page1 = new WizardPage_task183550;
    WizardPage_task183550 *page2 = new WizardPage_task183550;
    wizard.addPage(page1);
    wizard.addPage(page2);
    wizard.resize(500, 2 * page2->treeWidgetSizeHintHeight());
    wizard.show();

    QCOMPARE(wizard.currentPage(), static_cast<QWizardPage*>(page1));

    // ----
    page2->disableVerticalExpansion();
    wizard.next();
    QCOMPARE(wizard.currentPage(), static_cast<QWizardPage*>(page2));
    QCOMPARE(page2->treeWidgetHeight(), page2->treeWidgetSizeHintHeight());

    wizard.back();
    QCOMPARE(wizard.currentPage(), static_cast<QWizardPage*>(page1));

    // ----
    page2->enableVerticalExpansion();
    wizard.next();
    QCOMPARE(wizard.currentPage(), static_cast<QWizardPage*>(page2));
    QVERIFY(page2->treeWidgetHeight() > page2->treeWidgetSizeHintHeight());

    wizard.back();
    QCOMPARE(wizard.currentPage(), static_cast<QWizardPage*>(page1));

    // ----
    page2->disableVerticalExpansion();
    wizard.next();
    QCOMPARE(wizard.currentPage(), static_cast<QWizardPage*>(page2));
    QCOMPARE(page2->treeWidgetHeight(), page2->treeWidgetSizeHintHeight());
}

void tst_QWizard::task161658_alignments()
{
    QWizard wizard;
    wizard.setWizardStyle(QWizard::MacStyle);

    QWizardPage page;
    page.setTitle("Title");
    page.setSubTitle("SUBTITLE#:  The subtitle bust be aligned with the rest of the widget");

    QLabel label1("Field:");
    QLineEdit lineEdit1;
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(&label1, 0, 0);
    layout->addWidget(&lineEdit1, 0, 1);
    page.setLayout(layout);

    int idx = wizard.addPage(&page);
    wizard.setStartId(idx);
    wizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wizard));

    foreach (QLabel *subtitleLabel, wizard.findChildren<QLabel *>()) {
        if (subtitleLabel->text().startsWith("SUBTITLE#")) {
            QCOMPARE(lineEdit1.mapToGlobal(lineEdit1.contentsRect().bottomRight()).x(),
                     subtitleLabel->mapToGlobal(subtitleLabel->contentsRect().bottomRight()).x());
            return;
        }
    }
    QFAIL("Subtitle label not found");
}

void tst_QWizard::task177022_setFixedSize()
{
    int width = 300;
    int height = 200;
    QWizard wiz;
    QWizardPage page1;
    QWizardPage page2;
    int page1_id = wiz.addPage(&page1);
    int page2_id = wiz.addPage(&page2);
    wiz.setFixedSize(width, height);
    if (wiz.wizardStyle() == QWizard::AeroStyle)
        QEXPECT_FAIL("", "this probably relates to non-client area hack for AeroStyle titlebar "
                     "effect; not sure if it qualifies as a bug or not", Continue);
    QCOMPARE(wiz.size(), QSize(width, height));
    QCOMPARE(wiz.minimumWidth(), width);
    QCOMPARE(wiz.minimumHeight(), height);
    QCOMPARE(wiz.maximumWidth(), width);
    QCOMPARE(wiz.maximumHeight(), height);

    wiz.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&wiz));

    QCOMPARE(wiz.size(), QSize(width, height));
    QCOMPARE(wiz.minimumWidth(), width);
    QCOMPARE(wiz.minimumHeight(), height);
    QCOMPARE(wiz.maximumWidth(), width);
    QCOMPARE(wiz.maximumHeight(), height);

    wiz.next();
    QTest::qWait(100);
    QCOMPARE(wiz.size(), QSize(width, height));
    QCOMPARE(wiz.minimumWidth(), width);
    QCOMPARE(wiz.minimumHeight(), height);
    QCOMPARE(wiz.maximumWidth(), width);
    QCOMPARE(wiz.maximumHeight(), height);

    wiz.removePage(page1_id);
    wiz.removePage(page2_id);
}

void tst_QWizard::task248107_backButton()
{
    QWizard wizard;
    QWizardPage page1;
    QWizardPage page2;
    QWizardPage page3;
    QWizardPage page4;
    wizard.addPage(&page1);
    wizard.addPage(&page2);
    wizard.addPage(&page3);
    wizard.addPage(&page4);

    wizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wizard));

    QCOMPARE(wizard.currentPage(), &page1);

    QTest::mouseClick(wizard.button(QWizard::NextButton), Qt::LeftButton);
    QCOMPARE(wizard.currentPage(), &page2);

    QTest::mouseClick(wizard.button(QWizard::NextButton), Qt::LeftButton);
    QCOMPARE(wizard.currentPage(), &page3);

    QTest::mouseClick(wizard.button(QWizard::NextButton), Qt::LeftButton);
    QCOMPARE(wizard.currentPage(), &page4);

    QTest::mouseClick(wizard.button(QWizard::BackButton), Qt::LeftButton);
    QCOMPARE(wizard.currentPage(), &page3);

    QTest::mouseClick(wizard.button(QWizard::BackButton), Qt::LeftButton);
    QCOMPARE(wizard.currentPage(), &page2);

    QTest::mouseClick(wizard.button(QWizard::BackButton), Qt::LeftButton);
    QCOMPARE(wizard.currentPage(), &page1);
}

class WizardPage_task255350 : public QWizardPage
{
public:
    QLineEdit *lineEdit;
    WizardPage_task255350()
        : lineEdit(new QLineEdit)
    {
        registerField("dummy*", lineEdit);
    }
};

void tst_QWizard::task255350_fieldObjectDestroyed()
{
    QWizard wizard;
    WizardPage_task255350 *page = new WizardPage_task255350;
    int id = wizard.addPage(page);
    delete page->lineEdit;
    wizard.removePage(id); // don't crash!
    delete page;
}

// Global taskQTBUG_25691_fieldObjectDestroyed2 is defined in
// tst_qwizard_2.cpp to avoid cluttering up this file with
// the QWizardPage subclasses, etc. required to complete this
// test.
void taskQTBUG_25691_fieldObjectDestroyed2(void);
void tst_QWizard::taskQTBUG_25691_fieldObjectDestroyed2()
{
    ::taskQTBUG_25691_fieldObjectDestroyed2();
}

void tst_QWizard::taskQTBUG_46894_nextButtonShortcut()
{
    for (int i = 0; i < QWizard::NStyles; ++i) {
        QWizard wizard;
        QWizard::WizardStyle style = static_cast<QWizard::WizardStyle>(i);
        wizard.setWizardStyle(style);
        wizard.show();
        QVERIFY(QTest::qWaitForWindowExposed(&wizard));

        if (wizard.button(QWizard::NextButton)->text() == "&Next") {
            QCOMPARE(wizard.button(QWizard::NextButton)->shortcut(),
                     QKeySequence(Qt::ALT | Qt::Key_Right));
        } else {
            QCOMPARE(wizard.button(QWizard::NextButton)->shortcut(),
                     QKeySequence::mnemonic(wizard.button(QWizard::NextButton)->text()));
        }
    }
}

QTEST_MAIN(tst_QWizard)
#include "tst_qwizard.moc"
