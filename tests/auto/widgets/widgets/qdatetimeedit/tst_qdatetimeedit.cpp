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
#include <qapplication.h>
#include <qgroupbox.h>
#include <qlineedit.h>



#include <qdatetimeedit.h>
#include <qlocale.h>
#include <qlayout.h>
#include <qeventloop.h>
#include <qstyle.h>
#include <qstyle.h>
#include <QStyleOptionSpinBox>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QList>
#include <QDateTimeEdit>
#include <QCalendarWidget>
#include <QWidget>
#include <QLineEdit>
#include <QObject>
#include <QLocale>
#include <QString>
#include <QVariantList>
#include <QVariant>
#include <QApplication>
#include <QPoint>
#include <QVBoxLayout>
#include <QRect>
#include <QCursor>
#include <QEventLoop>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QTimeEdit>
#include <QMetaType>
#include <QDebug>
#include <QWheelEvent>
#include <QTest>
#include <QSignalSpy>
#include <QTestEventList>
#include <QDateEdit>

#include <private/qdatetimeedit_p.h>

#ifdef Q_OS_WIN
# include <windows.h>
# undef min
# undef max
#endif


Q_DECLARE_METATYPE(Qt::Key);
Q_DECLARE_METATYPE(Qt::KeyboardModifiers);
Q_DECLARE_METATYPE(Qt::KeyboardModifier);

class EditorDateEdit : public QDateTimeEdit
{
    Q_OBJECT
public:
    EditorDateEdit(QWidget *parent = 0) : QDateTimeEdit(parent) {}
    QLineEdit *lineEdit() { return QDateTimeEdit::lineEdit(); }
    friend class tst_QDateTimeEdit;
};

class tst_QDateTimeEdit : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void cachedDayTest();
    void getSetCheck();
    void constructor_qwidget();
    void constructor_qdatetime_data();
    void constructor_qdatetime();
    void constructor_qdate_data();
    void constructor_qdate();
    void constructor_qtime_data();
    void constructor_qtime();

    void sectionText_data();
    void sectionText();
    void dateTimeSignalChecking_data();
    void dateTimeSignalChecking();
    void mousePress();
    void stepHourAMPM_data();

    void stepHourAMPM();
    void displayedSections_data();
    void displayedSections();
    void currentSection_data();
    void currentSection();

    void setCurrentSection();
    void setCurrentSection_data();

    void minimumDate_data();
    void minimumDate();
    void maximumDate_data();
    void maximumDate();
    void clearMinimumDate_data();
    void clearMinimumDate();
    void clearMaximumDate_data();
    void clearMaximumDate();

    void minimumDateTime_data();
    void minimumDateTime();
    void maximumDateTime_data();
    void maximumDateTime();

    void clearMinimumDateTime_data();
    void clearMinimumDateTime();
    void clearMaximumDateTime_data();
    void clearMaximumDateTime();

    void displayFormat_data();
    void displayFormat();

    void specialValueText();
    void setRange_data();
    void setRange();

    void selectAndScrollWithKeys();
    void backspaceKey();
    void deleteKey();
    void tabKeyNavigation();
    void tabKeyNavigationWithPrefix();
    void tabKeyNavigationWithSuffix();
    void enterKey();

    void readOnly();

    void wrappingDate_data();
    void wrappingDate();

    void dateSignalChecking_data();
    void dateSignalChecking();

    void wrappingTime_data();
    void wrappingTime();

    void userKeyPress_Time_data();
    void userKeyPress_Time();

    void timeSignalChecking_data();
    void timeSignalChecking();

    void weirdCase();
    void newCase();
    void newCase2();
    void newCase3();
    void newCase4();
    void newCase5();
    void newCase6();

    void task98554();
    void task149097();

    void cursorPos();
    void calendarPopup();

    void hour12Test();
    void yyTest();
    void task108572();

    void task148725();
    void task148522();

    void setSelectedSection();
    void reverseTest();

    void ddMMMMyyyy();
#if QT_CONFIG(wheelevent)
    void wheelEvent();
#endif

    void specialValueCornerCase();
    void cursorPositionOnInit();

    void task118867();

    void nextPrevSection_data();
    void nextPrevSection();

    void dateEditTimeEditFormats();
    void timeSpec_data();
    void timeSpec();
    void timeSpecBug();
    void timeSpecInit();

    void monthEdgeCase();
    void setLocale();

    void potentialYYValueBug();
    void textSectionAtEnd();

    void keypadAutoAdvance_data();
    void keypadAutoAdvance();

    void task196924();
    void focusNextPrevChild();

    void taskQTBUG_12384_timeSpecShowTimeOnly();

    void deleteCalendarWidget();

    void setLocaleOnCalendarWidget();

#ifdef QT_BUILD_INTERNAL
    void dateEditCorrectSectionSize_data();
    void dateEditCorrectSectionSize();
#endif
private:
    EditorDateEdit* testWidget;
    QWidget *testFocusWidget;
};

typedef QList<QTime> TimeList;
typedef QList<Qt::Key> KeyList;

// Testing get/set functions
void tst_QDateTimeEdit::getSetCheck()
{
    QDateTimeEdit obj1;
    QCOMPARE(obj1.inputMethodQuery(Qt::ImHints), QVariant(int(Qt::ImhPreferNumbers)));
    obj1.setDisplayFormat("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z AP");
    // Section QDateTimeEdit::currentSection()
    // void QDateTimeEdit::setCurrentSection(Section)
    obj1.setCurrentSection(QDateTimeEdit::NoSection);
    QVERIFY(obj1.currentSection() != QDateTimeEdit::NoSection);
    obj1.setCurrentSection(QDateTimeEdit::AmPmSection);
    QCOMPARE(QDateTimeEdit::AmPmSection, obj1.currentSection());
    obj1.setCurrentSection(QDateTimeEdit::MSecSection);
    QCOMPARE(QDateTimeEdit::MSecSection, obj1.currentSection());
    obj1.setCurrentSection(QDateTimeEdit::SecondSection);
    QCOMPARE(QDateTimeEdit::SecondSection, obj1.currentSection());
    obj1.setCurrentSection(QDateTimeEdit::MinuteSection);
    QCOMPARE(QDateTimeEdit::MinuteSection, obj1.currentSection());
    obj1.setCurrentSection(QDateTimeEdit::HourSection);
    QCOMPARE(QDateTimeEdit::HourSection, obj1.currentSection());
    obj1.setCurrentSection(QDateTimeEdit::DaySection);
    QCOMPARE(QDateTimeEdit::DaySection, obj1.currentSection());
    obj1.setCurrentSection(QDateTimeEdit::MonthSection);
    QCOMPARE(QDateTimeEdit::MonthSection, obj1.currentSection());
    obj1.setCurrentSection(QDateTimeEdit::YearSection);
    QCOMPARE(QDateTimeEdit::YearSection, obj1.currentSection());

    QDateEdit dateEdit;
    QCOMPARE(dateEdit.inputMethodQuery(Qt::ImHints), QVariant(int(Qt::ImhPreferNumbers)));
    QTimeEdit timeEdit;
    QCOMPARE(timeEdit.inputMethodQuery(Qt::ImHints), QVariant(int(Qt::ImhPreferNumbers)));
}

void tst_QDateTimeEdit::initTestCase()
{
    QLocale system = QLocale::system();
    if (system.language() != QLocale::C && system.language() != QLocale::English)
        qWarning("Running under locale %s/%s -- this test may generate failures due to language differences",
                 qPrintable(QLocale::languageToString(system.language())),
                 qPrintable(QLocale::countryToString(system.country())));
    testWidget = new EditorDateEdit(0);
    testFocusWidget = new QWidget(0);
    testFocusWidget->resize(200, 100);
    testFocusWidget->show();
}

void tst_QDateTimeEdit::cleanupTestCase()
{
    delete testFocusWidget;
    testFocusWidget = 0;
    delete testWidget;
    testWidget = 0;
}

void tst_QDateTimeEdit::init()
{
    QLocale::setDefault(QLocale(QLocale::C));
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
#endif
    testWidget->setDisplayFormat("dd/MM/yyyy"); // Nice default to have
    testWidget->setDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0)));
    testWidget->show();
    testFocusWidget->move(-1000, -1000);
}

void tst_QDateTimeEdit::cleanup()
{
    testWidget->clearMinimumDateTime();
    testWidget->clearMaximumDateTime();
    testWidget->setTimeSpec(Qt::LocalTime);
    testWidget->setSpecialValueText(QString());
    testWidget->setWrapping(false);
}

void tst_QDateTimeEdit::constructor_qwidget()
{
    testWidget->hide();
    QDateTimeEdit dte(0);
    dte.show();
    QCOMPARE(dte.dateTime(), QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0, 0)));
    QCOMPARE(dte.minimumDate(), QDate(1752, 9, 14));
    QCOMPARE(dte.minimumTime(), QTime(0, 0, 0, 0));
    QCOMPARE(dte.maximumDate(), QDate(9999, 12, 31));
    QCOMPARE(dte.maximumTime(), QTime(23, 59, 59, 999));
}

void tst_QDateTimeEdit::constructor_qdatetime_data()
{
    QTest::addColumn<QDateTime>("parameter");
    QTest::addColumn<QDateTime>("displayDateTime");
    QTest::addColumn<QDate>("minimumDate");
    QTest::addColumn<QTime>("minimumTime");
    QTest::addColumn<QDate>("maximumDate");
    QTest::addColumn<QTime>("maximumTime");

    QTest::newRow("normal") << QDateTime(QDate(2004, 6, 16), QTime(13, 46, 32, 764))
                            << QDateTime(QDate(2004, 6, 16), QTime(13, 46, 32, 764))
                            << QDate(1752, 9, 14) << QTime(0, 0, 0, 0)
                            << QDate(9999, 12, 31) << QTime(23, 59, 59, 999);

    QTest::newRow("invalid") << QDateTime(QDate(9999, 99, 99), QTime(13, 46, 32, 764))
                             << QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0, 0))
                             << QDate(1752, 9, 14) << QTime(0, 0, 0, 0)
                             << QDate(9999, 12, 31) << QTime(23, 59, 59, 999);
}

void tst_QDateTimeEdit::constructor_qdatetime()
{
    QFETCH(QDateTime, parameter);
    QFETCH(QDateTime, displayDateTime);
    QFETCH(QDate, minimumDate);
    QFETCH(QTime, minimumTime);
    QFETCH(QDate, maximumDate);
    QFETCH(QTime, maximumTime);

    testWidget->hide();

    QDateTimeEdit dte(parameter);
    dte.show();
    QCOMPARE(dte.dateTime(), displayDateTime);
    QCOMPARE(dte.minimumDate(), minimumDate);
    QCOMPARE(dte.minimumTime(), minimumTime);
    QCOMPARE(dte.maximumDate(), maximumDate);
    QCOMPARE(dte.maximumTime(), maximumTime);
}

void tst_QDateTimeEdit::constructor_qdate_data()
{
    QTest::addColumn<QDate>("parameter");
    QTest::addColumn<QDateTime>("displayDateTime");
    QTest::addColumn<QDate>("minimumDate");
    QTest::addColumn<QTime>("minimumTime");
    QTest::addColumn<QDate>("maximumDate");
    QTest::addColumn<QTime>("maximumTime");

    QTest::newRow("normal") << QDate(2004, 6, 16)
                            << QDateTime(QDate(2004, 6, 16), QTime(0, 0, 0, 0))
                            << QDate(1752, 9, 14) << QTime(0, 0, 0, 0)
                            << QDate(9999, 12, 31) << QTime(23, 59, 59, 999);

    QTest::newRow("invalid") << QDate(9999, 99, 99)
                             << QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0, 0))
                             << QDate(1752, 9, 14) << QTime(0, 0, 0, 0)
                             << QDate(9999, 12, 31) << QTime(23, 59, 59, 999);
}

void tst_QDateTimeEdit::constructor_qdate()
{
    QFETCH(QDate, parameter);
    QFETCH(QDateTime, displayDateTime);
    QFETCH(QDate, minimumDate);
    QFETCH(QTime, minimumTime);
    QFETCH(QDate, maximumDate);
    QFETCH(QTime, maximumTime);

    testWidget->hide();

    QDateTimeEdit dte(parameter);
    dte.show();
    QCOMPARE(dte.dateTime(), displayDateTime);
    QCOMPARE(dte.minimumDate(), minimumDate);
    QCOMPARE(dte.minimumTime(), minimumTime);
    QCOMPARE(dte.maximumDate(), maximumDate);
    QCOMPARE(dte.maximumTime(), maximumTime);
}

void tst_QDateTimeEdit::constructor_qtime_data()
{
    QTest::addColumn<QTime>("parameter");
    QTest::addColumn<QDateTime>("displayDateTime");
    QTest::addColumn<QDate>("minimumDate");
    QTest::addColumn<QTime>("minimumTime");
    QTest::addColumn<QDate>("maximumDate");
    QTest::addColumn<QTime>("maximumTime");

    QTest::newRow("normal") << QTime(13, 46, 32, 764)
                            << QDateTime(QDate(2000, 1, 1), QTime(13, 46, 32, 764))
                            << QDate(2000, 1, 1) << QTime(0, 0, 0, 0)
                            << QDate(2000, 1, 1) << QTime(23, 59, 59, 999);

    QTest::newRow("invalid") << QTime(99, 99, 99, 5000)
                             << QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0, 0))
                             << QDate(2000, 1, 1) << QTime(0, 0, 0, 0)
                             << QDate(2000, 1, 1) << QTime(23, 59, 59, 999);
}

void tst_QDateTimeEdit::constructor_qtime()
{
    QFETCH(QTime, parameter);
    QFETCH(QDateTime, displayDateTime);
    QFETCH(QDate, minimumDate);
    QFETCH(QTime, minimumTime);
    QFETCH(QDate, maximumDate);
    QFETCH(QTime, maximumTime);

    testWidget->hide();

    QDateTimeEdit dte(parameter);
    dte.show();
    QCOMPARE(dte.dateTime(), displayDateTime);
    QCOMPARE(dte.minimumDate(), minimumDate);
    QCOMPARE(dte.minimumTime(), minimumTime);
    QCOMPARE(dte.maximumDate(), maximumDate);
    QCOMPARE(dte.maximumTime(), maximumTime);
}

void tst_QDateTimeEdit::minimumDate_data()
{
    QTest::addColumn<QDate>("minimumDate");
    QTest::addColumn<QDate>("expectedMinDate");

    QTest::newRow("normal-0") << QDate(2004, 5, 10) << QDate(2004, 5, 10);
    QTest::newRow("normal-1") << QDate(2002, 3, 15) << QDate(2002, 3, 15);
    QTest::newRow("normal-2") << QDate(9999, 12, 31) << QDate(9999, 12, 31);
    QTest::newRow("normal-3") << QDate(1753, 1, 1) << QDate(1753, 1, 1);
    QTest::newRow("invalid-0") << QDate(0, 0, 0) << QDate(1752, 9, 14);
    QTest::newRow("old") << QDate(1492, 8, 3) << QDate(1492, 8, 3);
}

void tst_QDateTimeEdit::minimumDate()
{
    QFETCH(QDate, minimumDate);
    QFETCH(QDate, expectedMinDate);

    testWidget->setMinimumDate(minimumDate);
    QCOMPARE(testWidget->minimumDate(), expectedMinDate);
}

void tst_QDateTimeEdit::minimumDateTime_data()
{
    QTest::addColumn<QDateTime>("minimumDateTime");
    QTest::addColumn<QDateTime>("expectedMinDateTime");

    QTest::newRow("normal-0") << QDateTime(QDate(2004, 5, 10), QTime(2, 3, 14))
                              << QDateTime(QDate(2004, 5, 10), QTime(2, 3, 14));

    QTest::newRow("normal-1") << QDateTime(QDate(2005, 5, 10), QTime(22, 33, 1))
                              << QDateTime(QDate(2005, 5, 10), QTime(22, 33, 1));
    QTest::newRow("normal-2") << QDateTime(QDate(2006, 5, 10), QTime(13, 31, 23))
                              << QDateTime(QDate(2006, 5, 10), QTime(13, 31, 23));
    QTest::newRow("normal-3") << QDateTime(QDate(2007, 5, 10), QTime(22, 23, 23))
                              << QDateTime(QDate(2007, 5, 10), QTime(22, 23, 23));
    QTest::newRow("normal-4") << QDateTime(QDate(2008, 5, 10), QTime(2, 3, 1))
                              << QDateTime(QDate(2008, 5, 10), QTime(2, 3, 1));
    QTest::newRow("invalid-0") << QDateTime() << QDateTime(QDate(1752, 9, 14), QTime(0, 0, 0));
    QTest::newRow("old") << QDateTime(QDate(1492, 8, 3), QTime(2, 3, 1))
                         << QDateTime(QDate(1492, 8, 3), QTime(2, 3, 1));
}

void tst_QDateTimeEdit::minimumDateTime()
{
    QFETCH(QDateTime, minimumDateTime);
    QFETCH(QDateTime, expectedMinDateTime);

    testWidget->setMinimumDateTime(minimumDateTime);
    QCOMPARE(testWidget->minimumDateTime(), expectedMinDateTime);
}

void tst_QDateTimeEdit::maximumDateTime_data()
{
    QTest::addColumn<QDateTime>("maximumDateTime");
    QTest::addColumn<QDateTime>("expectedMinDateTime");

    QTest::newRow("normal-0") << QDateTime(QDate(2004, 5, 10), QTime(2, 3, 14))
                              << QDateTime(QDate(2004, 5, 10), QTime(2, 3, 14));

    QTest::newRow("normal-1") << QDateTime(QDate(2005, 5, 10), QTime(22, 33, 1))
                              << QDateTime(QDate(2005, 5, 10), QTime(22, 33, 1));
    QTest::newRow("normal-2") << QDateTime(QDate(2006, 5, 10), QTime(13, 31, 23))
                              << QDateTime(QDate(2006, 5, 10), QTime(13, 31, 23));
    QTest::newRow("normal-3") << QDateTime(QDate(2007, 5, 10), QTime(22, 23, 23))
                              << QDateTime(QDate(2007, 5, 10), QTime(22, 23, 23));
    QTest::newRow("normal-4") << QDateTime(QDate(2008, 5, 10), QTime(2, 3, 1))
                              << QDateTime(QDate(2008, 5, 10), QTime(2, 3, 1));
    QTest::newRow("invalid-0") << QDateTime() << QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59, 999));
}

void tst_QDateTimeEdit::maximumDateTime()
{
    QFETCH(QDateTime, maximumDateTime);
    QFETCH(QDateTime, expectedMinDateTime);

    testWidget->setMaximumDateTime(maximumDateTime);
    QCOMPARE(testWidget->maximumDateTime(), expectedMinDateTime);
}

void tst_QDateTimeEdit::maximumDate_data()
{
    QTest::addColumn<QDate>("maximumDate");
    QTest::addColumn<QDate>("expectedMaxDate");

    QTest::newRow("normal-0") << QDate(2004, 05, 10) << QDate(2004, 5, 10);
    QTest::newRow("normal-1") << QDate(2002, 03, 15) << QDate(2002, 3, 15);
    QTest::newRow("normal-2") << QDate(9999, 12, 31) << QDate(9999, 12, 31);
    QTest::newRow("normal-3") << QDate(1753, 1, 1) << QDate(1753, 1, 1);
    QTest::newRow("invalid-0") << QDate(0, 0, 0) << QDate(9999, 12, 31);
}

void tst_QDateTimeEdit::maximumDate()
{
    QFETCH(QDate, maximumDate);
    QFETCH(QDate, expectedMaxDate);

    testWidget->setMaximumDate(maximumDate);
    QCOMPARE(testWidget->maximumDate(), expectedMaxDate);
}

void tst_QDateTimeEdit::clearMinimumDate_data()
{
    QTest::addColumn<QDate>("minimumDate");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QDate>("expectedMinDateAfterClear");

    QTest::newRow("normal-0") << QDate(2004, 05, 10) << true << QDate(1752, 9, 14);
    QTest::newRow("normal-1") << QDate(2002, 3, 15) << true << QDate(1752, 9, 14);
    QTest::newRow("normal-2") << QDate(9999, 12, 31) << true << QDate(1752, 9, 14);
    QTest::newRow("normal-3") << QDate(1753, 1, 1) << true << QDate(1752, 9, 14);
    QTest::newRow("invalid-0") << QDate(0, 0, 0) << false << QDate(1752, 9, 14);
}

void tst_QDateTimeEdit::clearMinimumDate()
{
    QFETCH(QDate, minimumDate);
    QFETCH(bool, valid);
    QFETCH(QDate, expectedMinDateAfterClear);

    testWidget->setMinimumDate(minimumDate);
    if (valid)
        QCOMPARE(testWidget->minimumDate(), minimumDate);
    testWidget->clearMinimumDate();
    QCOMPARE(testWidget->minimumDate(), expectedMinDateAfterClear);
}

void tst_QDateTimeEdit::clearMinimumDateTime_data()
{
    QTest::addColumn<QDateTime>("minimumDateTime");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QDateTime>("expectedMinDateTimeAfterClear");

    QTest::newRow("normal-0") << QDateTime(QDate(2004, 05, 10), QTime(12, 12, 12))
                              << true << QDateTime(QDate(1752, 9, 14), QTime(0, 0));
    QTest::newRow("normal-1") << QDateTime(QDate(2002, 3, 15), QTime(13, 13, 13))
                              << true << QDateTime(QDate(1752, 9, 14), QTime(0, 0));
    QTest::newRow("normal-2") << QDateTime(QDate(9999, 12, 31), QTime(14, 14, 14))
                              << true << QDateTime(QDate(1752, 9, 14), QTime(0, 0));
    QTest::newRow("normal-3") << QDateTime(QDate(1753, 1, 1), QTime(15, 15, 15))
                              << true << QDateTime(QDate(1752, 9, 14), QTime(0, 0));
    QTest::newRow("invalid-0") << QDateTime() << false << QDateTime(QDate(1752, 9, 14), QTime(0, 0));
    QTest::newRow("old") << QDateTime(QDate(1492, 8, 3), QTime(2, 3, 1)) << true
                         << QDateTime(QDate(1752, 9, 14), QTime(0, 0));
}

void tst_QDateTimeEdit::clearMinimumDateTime()
{
    QFETCH(QDateTime, minimumDateTime);
    QFETCH(bool, valid);
    QFETCH(QDateTime, expectedMinDateTimeAfterClear);

    testWidget->setMinimumDateTime(minimumDateTime);
    if (valid)
        QCOMPARE(testWidget->minimumDateTime(), minimumDateTime);
    testWidget->clearMinimumDateTime();
    QCOMPARE(testWidget->minimumDateTime(), expectedMinDateTimeAfterClear);
}

void tst_QDateTimeEdit::clearMaximumDateTime_data()
{
    QTest::addColumn<QDateTime>("maximumDateTime");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QDateTime>("expectedMinDateTimeAfterClear");

    QTest::newRow("normal-0") << QDateTime(QDate(2004, 05, 10), QTime(12, 12, 12))
                              << true << QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59, 999));
    QTest::newRow("normal-1") << QDateTime(QDate(2002, 3, 15), QTime(13, 13, 13))
                              << true << QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59, 999));
    QTest::newRow("normal-2") << QDateTime(QDate(9999, 12, 31), QTime(14, 14, 14))
                              << true << QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59, 999));
    QTest::newRow("normal-3") << QDateTime(QDate(1753, 1, 1), QTime(15, 15, 15))
                              << true << QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59, 999));
    QTest::newRow("invalid-0") << QDateTime()
                               << false << QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59, 999));
}

void tst_QDateTimeEdit::clearMaximumDateTime()
{
    QFETCH(QDateTime, maximumDateTime);
    QFETCH(bool, valid);
    QFETCH(QDateTime, expectedMinDateTimeAfterClear);

    testWidget->setMaximumDateTime(maximumDateTime);
    if (valid)
        QCOMPARE(testWidget->maximumDateTime(), maximumDateTime);
    testWidget->clearMaximumDateTime();
    QCOMPARE(testWidget->maximumDateTime(), expectedMinDateTimeAfterClear);
}

void tst_QDateTimeEdit::clearMaximumDate_data()
{
    QTest::addColumn<QDate>("maximumDate");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QDate>("expectedMaxDateAfterClear");

    QTest::newRow("normal-0") << QDate(2004, 05, 10) << true << QDate(9999, 12, 31);
    QTest::newRow("normal-1") << QDate(2002, 03, 15) << true << QDate(9999, 12, 31);
    QTest::newRow("normal-2") << QDate(9999, 12, 31) << true << QDate(9999, 12, 31);
    QTest::newRow("normal-3") << QDate(2000, 1, 1) << true << QDate(9999, 12, 31);
    QTest::newRow("invalid-0") << QDate(0, 0, 0) << false << QDate(9999, 12, 31);
}

void tst_QDateTimeEdit::clearMaximumDate()
{
    QFETCH(QDate, maximumDate);
    QFETCH(bool, valid);
    QFETCH(QDate, expectedMaxDateAfterClear);

    testWidget->setMaximumDate(maximumDate);
    if (valid)
        QCOMPARE(testWidget->maximumDate(), maximumDate);
    testWidget->clearMaximumDate();
    QCOMPARE(testWidget->maximumDate(), expectedMaxDateAfterClear);
}

void tst_QDateTimeEdit::displayFormat_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QDateTime>("date");

    const QDateTime dt(QDate(2999, 12, 31), QTime(3, 59, 59, 999));

    QTest::newRow("valid-0") << QString("yyyy MM dd") << true << QString("2999 12 31") << dt;
    QTest::newRow("valid-1") << QString("dd MM yyyy::ss:mm:hh") << true
                             << QString("31 12 2999::59:59:03") << dt;
    QTest::newRow("valid-2") << QString("hh-dd-mm-MM-yy") << true << QString("03-31-59-12-99") << dt;
    QTest::newRow("valid-3") << QString("ddd MM d yyyy::ss:mm:hh") << true
                             << QDate::shortDayName(2) + " 12 31 2999::59:59:03" << dt;
    QTest::newRow("valid-4") << QString("hh-dd-mm-MM-yyyy") << true << QString("03-31-59-12-2999") << dt;
    QTest::newRow("invalid-0") << QString("yyyy.MM.yy") << true << QString("2999.12.99") << dt;
    QTest::newRow("invalid-1") << QString("y") << false << QString() << dt;
    QTest::newRow("invalid-2") << QString("") << false << QString() << dt;
    QTest::newRow("quoted-1") << QString("'Midday is at:' dd") << true << QString("Midday is at: 31") << dt;
    QTest::newRow("leading1") << QString("h:hh:hhh") << true << QString("3:03:033") << dt;
    QTest::newRow("H1") << QString("HH:hh:ap") << true << QString("03:03:am") << dt;
    QTest::newRow("H2") << QString("HH:hh:ap") << true << QString("23:11:pm")
                        << QDateTime(dt.date(), QTime(23, 0, 0));
}

void tst_QDateTimeEdit::displayFormat()
{
    QFETCH(QString, format);
    QFETCH(bool, valid);
    QFETCH(QString, text);
    QFETCH(QDateTime, date);

    testWidget->setDateTime(date);

    QString compareFormat = format;
    if (!valid)
        compareFormat = testWidget->displayFormat();
    testWidget->setDisplayFormat(format);
    QCOMPARE(testWidget->displayFormat(), compareFormat);
    if (valid)
        QCOMPARE(testWidget->text(), text);
}

void tst_QDateTimeEdit::selectAndScrollWithKeys()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-23674");
    return;
#endif

    qApp->setActiveWindow(testWidget);
    testWidget->setDate(QDate(2004, 05, 11));
    testWidget->setDisplayFormat("dd/MM/yyyy");
    testWidget->show();
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("1"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/0"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/2"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/20"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/200"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/2004"));

    // Now the year part should be selected
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2005, 5, 11));
    QCOMPARE(testWidget->currentSection(), QDateTimeEdit::YearSection);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("2005"));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2004, 5, 11));
    QCOMPARE(testWidget->currentSection(), QDateTimeEdit::YearSection);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("2004"));


#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("4"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("04"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("2004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("/2004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("5/2004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("1/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11/05/2004"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif

    // Now the day part should be selected
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2004, 5, 12));
    QCOMPARE(testWidget->currentSection(), QDateTimeEdit::DaySection);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("12"));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2004, 5, 11));
    QCOMPARE(testWidget->currentSection(), QDateTimeEdit::DaySection);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));

#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif

    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
    // Now the day part should be selected
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2004, 05, 12));
}

void tst_QDateTimeEdit::backspaceKey()
{
    qApp->setActiveWindow(testWidget);
    testWidget->setDate(QDate(2004, 05, 11));
    testWidget->setDisplayFormat("d/MM/yyyy");
    testWidget->show();
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QCOMPARE(testWidget->text(), QString("11/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(testWidget->text(), QString("11/05/200"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/05/20"));
    // Check that moving into another field reverts it
    for (int i=0;i<3;i++)
        QTest::keyClick(testWidget, Qt::Key_Left);
    QCOMPARE(testWidget->text(), QString("11/05/2004"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    for (int i=0;i<4;i++)
        QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);

    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/05/"));
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/0/2004"));
    testWidget->interpretText();
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/05/200"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/05/20"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/05/2"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/05/"));
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11/0/2004"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("11//2004"));
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("1/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(testWidget->text(), QString("1/05/2004"));
}

void tst_QDateTimeEdit::deleteKey()
{
    qApp->setActiveWindow(testWidget);
    testWidget->setDate(QDate(2004, 05, 11));
    testWidget->setDisplayFormat("d/MM/yyyy");
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
    QTest::keyClick(testWidget, Qt::Key_Delete);
    QCOMPARE(testWidget->text(), QString("1/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Delete);
    QCOMPARE(testWidget->text(), QString("/05/2004"));
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QCOMPARE(testWidget->text(), QString("1/05/2004"));
}

void tst_QDateTimeEdit::tabKeyNavigation()
{
    qApp->setActiveWindow(testWidget);
    testWidget->setDate(QDate(2004, 05, 11));
    testWidget->setDisplayFormat("dd/MM/yyyy");
    testWidget->show();
    testWidget->setCurrentSection(QDateTimeEdit::DaySection);

    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("05"));
    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("2004"));
    QTest::keyClick(testWidget, Qt::Key_Tab, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("05"));
    QTest::keyClick(testWidget, Qt::Key_Tab, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
}

void tst_QDateTimeEdit::tabKeyNavigationWithPrefix()
{
    qApp->setActiveWindow(testWidget);
    testWidget->setDate(QDate(2004, 05, 11));
    testWidget->setDisplayFormat("prefix dd/MM/yyyy");

    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("05"));
    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("2004"));
    QTest::keyClick(testWidget, Qt::Key_Tab, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("05"));
    QTest::keyClick(testWidget, Qt::Key_Tab, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
}

void tst_QDateTimeEdit::tabKeyNavigationWithSuffix()
{
    qApp->setActiveWindow(testWidget);
    testWidget->setDate(QDate(2004, 05, 11));
    testWidget->setDisplayFormat("dd/MM/yyyy 'suffix'");

    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("05"));
    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("2004"));
    QTest::keyClick(testWidget, Qt::Key_Tab, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("05"));
    QTest::keyClick(testWidget, Qt::Key_Tab, Qt::ShiftModifier);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
}

void tst_QDateTimeEdit::enterKey()
{
    qApp->setActiveWindow(testWidget);
    testWidget->setDate(QDate(2004, 5, 11));
    testWidget->setDisplayFormat("prefix d/MM/yyyy 'suffix'");
    testWidget->lineEdit()->setFocus();

#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
    QTest::keyClick(testWidget, Qt::Key_Enter);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QVERIFY(!testWidget->lineEdit()->hasSelectedText());
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QVERIFY(!testWidget->lineEdit()->hasSelectedText());
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
    QTest::keyClick(testWidget, Qt::Key_Tab);
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
    QTest::keyClick(testWidget, Qt::Key_1);
    QTest::keyClick(testWidget, Qt::Key_5);

    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Left);

    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("15"));
    QCOMPARE(testWidget->date(), QDate(2004, 5, 15));

    QTest::keyClick(testWidget, Qt::Key_9);
    QTest::keyClick(testWidget, Qt::Key_Tab, Qt::ShiftModifier);
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("9"));
    QCOMPARE(testWidget->date(), QDate(2004, 5, 9));

    QTest::keyClick(testWidget, Qt::Key_0);
    QTest::keyClick(testWidget, Qt::Key_0);
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("9"));
    QCOMPARE(testWidget->date(), QDate(2004, 5, 9));

    // Current behaviour is that pressing the Enter key in a QDateTimeEdit
    // causes the dateChanged() signal to be emitted, even if the date
    // wasn't actually changed.  While this behaviour is questionable,
    // we include this test so a change to the behaviour can't go unnoticed.
    QSignalSpy enterSpy(testWidget, SIGNAL(dateChanged(QDate)));
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(enterSpy.count(), 1);
    QVariantList list = enterSpy.takeFirst();
    QCOMPARE(list.at(0).toDate(), QDate(2004, 5, 9));
}

void tst_QDateTimeEdit::specialValueText()
{
    testWidget->setDisplayFormat("dd/MM/yyyy");
    testWidget->setDateRange(QDate(2000, 1, 1), QDate(2001, 1, 1));
    testWidget->setDate(QDate(2000, 1, 2));
    testWidget->setSpecialValueText("fOo");
    testWidget->setCurrentSection(QDateTimeEdit::DaySection);
    QCOMPARE(testWidget->date(), QDate(2000, 1, 2));
    QCOMPARE(testWidget->text(), QString("02/01/2000"));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2000, 1, 1));
    QCOMPARE(testWidget->text(), QString("fOo"));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2000, 1, 1));
    QCOMPARE(testWidget->text(), QString("fOo"));
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2000, 1, 2));
    QCOMPARE(testWidget->text(), QString("02/01/2000"));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2000, 1, 1));
    QCOMPARE(testWidget->text(), QString("fOo"));

#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2000, 1, 2));
    QCOMPARE(testWidget->text(), QString("02/01/2000"));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->text(), QString("fOo"));

#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("fO"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString("f"));
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString());
    QTest::keyClick(testWidget, Qt::Key_F);
    QCOMPARE(testWidget->text(), QString("f"));
    QTest::keyClick(testWidget, Qt::Key_O); // will automatically uppercase
    QCOMPARE(testWidget->text(), QString("fO"));
    QTest::keyClick(testWidget, Qt::Key_O);
    QCOMPARE(testWidget->text(), QString("fOo"));
}

void tst_QDateTimeEdit::setRange_data()
{
    QTest::addColumn<QTime>("minTime");
    QTest::addColumn<QTime>("maxTime");
    QTest::addColumn<QDate>("minDate");
    QTest::addColumn<QDate>("maxDate");
    QTest::addColumn<QDateTime>("expectedMin");
    QTest::addColumn<QDateTime>("expectedMax");

    const QDate cdt = QDate::currentDate();

    QTest::newRow("data0") << QTime(0, 0) << QTime(14, 12, 0)
                           << cdt << cdt
                           << QDateTime(cdt, QTime(0, 0))
                           << QDateTime(cdt, QTime(14, 12, 0));

    QTest::newRow("data1") << QTime(10, 0) << QTime(1, 12, 0) << cdt.addDays(-1)
                           << cdt
                           << QDateTime(cdt.addDays(-1), QTime(10, 0))
                           << QDateTime(cdt, QTime(1, 12, 0));
}

void tst_QDateTimeEdit::setRange()
{
    QFETCH(QTime, minTime);
    QFETCH(QTime, maxTime);
    QFETCH(QDate, minDate);
    QFETCH(QDate, maxDate);
    QFETCH(QDateTime, expectedMin);
    QFETCH(QDateTime, expectedMax);
    testWidget->hide();

    {
        QDateTimeEdit dte(0);
        dte.setTimeRange(minTime, maxTime);
        QCOMPARE(dte.minimumTime(), expectedMin.time());
        QCOMPARE(dte.maximumTime(), expectedMax.time());
        dte.setDateRange(minDate, maxDate);
        QCOMPARE(dte.minimumDate(), expectedMin.date());
        QCOMPARE(dte.minimumDateTime(), expectedMin);
        QCOMPARE(dte.maximumDate(), expectedMax.date());
        QCOMPARE(dte.maximumDateTime(), expectedMax);
        QCOMPARE(dte.minimumTime(), expectedMin.time());
        QCOMPARE(dte.maximumTime(), expectedMax.time());
        dte.setDateTimeRange(QDateTime(minDate, minTime), QDateTime(maxDate, maxTime));
        QCOMPARE(dte.minimumDate(), expectedMin.date());
        QCOMPARE(dte.minimumDateTime(), expectedMin);
        QCOMPARE(dte.maximumDate(), expectedMax.date());
        QCOMPARE(dte.maximumDateTime(), expectedMax);
        QCOMPARE(dte.minimumTime(), expectedMin.time());
        QCOMPARE(dte.maximumTime(), expectedMax.time());
    }
    {

        QDateTimeEdit dte2(0);
        dte2.setDateRange(minDate, maxDate);
        dte2.setTimeRange(minTime, maxTime);

        QCOMPARE(dte2.minimumDate(), expectedMin.date());
        QCOMPARE(dte2.maximumDate(), expectedMax.date());
        QCOMPARE(dte2.minimumTime(), expectedMin.time());
        QCOMPARE(dte2.maximumTime(), expectedMax.time());
    }

    {
        QDateTimeEdit dte3(0);
        dte3.setMinimumTime(minTime);
        dte3.setMaximumTime(maxTime);
        dte3.setMinimumDate(minDate);
        dte3.setMaximumDate(maxDate);

        QCOMPARE(dte3.minimumDate(), expectedMin.date());
        QCOMPARE(dte3.maximumDate(), expectedMax.date());
        QCOMPARE(dte3.minimumTime(), expectedMin.time());
        QCOMPARE(dte3.maximumTime(), expectedMax.time());
    }

    {
        QDateTimeEdit dte4(0);
        dte4.setMinimumDate(minDate);
        dte4.setMaximumDate(maxDate);
        dte4.setMinimumTime(minTime);
        dte4.setMaximumTime(maxTime);

        QCOMPARE(dte4.minimumDate(), expectedMin.date());
        QCOMPARE(dte4.maximumDate(), expectedMax.date());
        QCOMPARE(dte4.minimumTime(), expectedMin.time());
        QCOMPARE(dte4.maximumTime(), expectedMax.time());
    }
}

void tst_QDateTimeEdit::wrappingTime_data()
{
    QTest::addColumn<bool>("startWithMin");
    QTest::addColumn<QTime>("minimumTime");
    QTest::addColumn<QTime>("maximumTime");
    QTest::addColumn<uint>("section");
    QTest::addColumn<QTime>("newTime");

    QTest::newRow("data0") << false << QTime(0,0,0) << QTime(2,2,2) << (uint)QDateTimeEdit::HourSection
                        << QTime(0,2,2);
    QTest::newRow("data1") << true << QTime(0,0,0) << QTime(2,2,2) << (uint)QDateTimeEdit::HourSection
                        << QTime(2,0,0);
    QTest::newRow("data2") << false << QTime(0,0,0) << QTime(2,2,2) << (uint)QDateTimeEdit::MinuteSection
                        << QTime(2,0,2);
    QTest::newRow("data3") << true << QTime(0,0,0) << QTime(2,2,2) << (uint)QDateTimeEdit::MinuteSection
                        << QTime(0,59,0);
    QTest::newRow("data4") << false << QTime(0,0,0) << QTime(2,2,2) << (uint)QDateTimeEdit::SecondSection
                        << QTime(2,2,0);
    QTest::newRow("data5") << true << QTime(0,0,0) << QTime(2,2,2) << (uint)QDateTimeEdit::SecondSection
                        << QTime(0,0,59);
    QTest::newRow("data6") << false << QTime(1,1,1) << QTime(22,22,22) << (uint)QDateTimeEdit::HourSection
                        << QTime(1,22,22);
    QTest::newRow("data7") << true << QTime(1,1,1) << QTime(22,22,22) << (uint)QDateTimeEdit::HourSection
                        << QTime(22,1,1);
    QTest::newRow("data8") << false << QTime(1,1,1) << QTime(22,22,22) << (uint)QDateTimeEdit::MinuteSection
                        << QTime(22,0,22);
    QTest::newRow("data9") << true << QTime(1,1,1) << QTime(22,22,22) << (uint)QDateTimeEdit::MinuteSection
                        << QTime(1,59,1);
    QTest::newRow("data10") << false << QTime(1,1,1) << QTime(22,22,22) << (uint)QDateTimeEdit::SecondSection
                         << QTime(22,22,0);
    QTest::newRow("data11") << true << QTime(1,1,1) << QTime(22,22,22) << (uint)QDateTimeEdit::SecondSection
                         << QTime(1,1,59);
    QTest::newRow("data12") << false << QTime(1,1,1) << QTime(1,2,1) << (uint)QDateTimeEdit::HourSection
                         << QTime(1,2,1);
    QTest::newRow("data13") << true << QTime(1,1,1) << QTime(1,2,1) << (uint)QDateTimeEdit::HourSection
                         << QTime(1,1,1);
    QTest::newRow("data14") << false << QTime(1,1,1) << QTime(1,2,1) << (uint)QDateTimeEdit::MinuteSection
                         << QTime(1,1,1);
    QTest::newRow("data15") << true << QTime(1,1,1) << QTime(1,2,1) << (uint)QDateTimeEdit::MinuteSection
                         << QTime(1,2,1);
    QTest::newRow("data16") << false << QTime(1,1,1) << QTime(1,2,1) << (uint)QDateTimeEdit::SecondSection
                         << QTime(1,2,0);
    QTest::newRow("data17") << true << QTime(1,1,1) << QTime(1,2,1) << (uint)QDateTimeEdit::SecondSection
                         << QTime(1,1,59);
}

void tst_QDateTimeEdit::wrappingTime()
{
    QFETCH(bool, startWithMin);
    QFETCH(QTime, minimumTime);
    QFETCH(QTime, maximumTime);
    QFETCH(uint, section);
    QFETCH(QTime, newTime);

    testWidget->setDisplayFormat("hh:mm:ss");
    testWidget->setMinimumTime(minimumTime);
    testWidget->setMaximumTime(maximumTime);
    testWidget->setWrapping(true);
    testWidget->setCurrentSection((QDateTimeEdit::Section)section);
    if (startWithMin) {
        testWidget->setTime(minimumTime);
        QTest::keyClick(testWidget, Qt::Key_Down);
    } else {
        testWidget->setTime(maximumTime);
        QTest::keyClick(testWidget, Qt::Key_Up);
    }
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(testWidget->time(), newTime);
}

void tst_QDateTimeEdit::userKeyPress_Time_data()
{
    QTest::addColumn<bool>("ampm");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<QTime>("expected_time");

    // ***************** test the hours ***************

    // use up/down keys to change hour in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 10, 0, 0 );
        QTest::newRow( "data0" ) << bool(true) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<5; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 6, 0, 0 );
        QTest::newRow( "data1" ) << bool(true) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<10; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 1, 0, 0 );
        QTest::newRow( "data2" ) << bool(true) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<12; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 23, 0, 0 );
        QTest::newRow( "data3" ) << bool(true) << keys << expected;
    }
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Up );
        QTime expected( 12, 0, 0 );
        QTest::newRow( "data4" ) << bool(true) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 13, 0, 0 );
        QTest::newRow( "data5" ) << bool(true) << keys << expected;
    }

    // use up/down keys to change hour in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 10, 0, 0 );
        QTest::newRow( "data6" ) << bool(false) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<5; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 6, 0, 0 );
        QTest::newRow( "data7" ) << bool(false) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<10; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 1, 0, 0 );
        QTest::newRow( "data8" ) << bool(false) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<12; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 23, 0, 0 );
        QTest::newRow( "data9" ) << bool(false) << keys << expected;
    }
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Up );
        QTime expected( 12, 0, 0 );
        QTest::newRow( "data10" ) << bool(false) << keys << expected;
    }
    {
        QTestEventList keys;
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 13, 0, 0 );
        QTest::newRow( "data11" ) << bool(false) << keys << expected;
    }

    // enter a one digit valid hour
    {
        QTestEventList keys;
        keys.addKeyClick( '5' );
        QTime expected( 5, 0, 0 );
        QTest::newRow( "data12" ) << bool(true) << keys << expected;
    }

    // entering a two digit valid hour
    {
        QTestEventList keys;
        keys.addKeyClick( '1' );
        keys.addKeyClick( '1' );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data13" ) << bool(true) << keys << expected;
    }

    // entering an invalid hour
    {
        QTestEventList keys;
        keys.addKeyClick( '2' );
        // the '5' creates an invalid hour (25) so it must be ignored
        keys.addKeyClick( '5' );
        QTime expected( 2, 0, 0 );
        QTest::newRow( "data14" ) << bool(true) << keys << expected;
    }

    // enter a value, in hour which causes a field change
    {
        QTestEventList keys;
        keys.addKeyClick( '0' );
        keys.addKeyClick( '2' );
        keys.addKeyClick( '1' );
        QTime expected( 2, 1, 0 );
        QTest::newRow( "data15" ) << bool(true) << keys << expected;
    }

    // enter a one digit valid hour in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( '5' );
        QTime expected( 5, 0, 0 );
        QTest::newRow( "data16" ) << bool(false) << keys << expected;
    }

    // enter a two digit valid hour in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( '1' );
        keys.addKeyClick( '1' );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data17" ) << bool(false) << keys << expected;
    }

    // enter a two digit valid hour (>12) in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( '1' );
        keys.addKeyClick( '5' );
        QTime expected( 15, 0, 0 );
        QTest::newRow( "data18" ) << bool(false) << keys << expected;
    }

    // enter a two digit valid hour (>20) in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( '2' );
        keys.addKeyClick( '1' );
        QTime expected( 21, 0, 0 );
        QTest::newRow( "data19" ) << bool(false) << keys << expected;
    }

    // enter a two digit invalid hour (>23) in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( '2' );
        keys.addKeyClick( '4' );
        QTime expected( 2, 0, 0 );
        QTest::newRow( "data20" ) << bool(false) << keys << expected;
    }

    // ***************** test the minutes ***************

    // use up/down keys to change the minutes in 12 hour mode
    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 2, 0 );
        QTest::newRow( "data21" ) << bool(true) << keys << expected;
    }
    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<16; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 16, 0 );
        QTest::newRow( "data22" ) << bool(true) << keys << expected;
    }
    { // test maximum value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<59; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 59, 0 );
        QTest::newRow( "data23" ) << bool(true) << keys << expected;
    }
    { // test 'overflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<60; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data24" ) << bool(true) << keys << expected;
    }
    { // test 'underflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 59, 0 );
        QTest::newRow( "data25" ) << bool(true) << keys << expected;
    }
    { // test valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 58, 0 );
        QTest::newRow( "data26" ) << bool(true) << keys << expected;
    }

    // use up/down keys to change the minutes in 24 hour mode

    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 2, 0 );
        QTest::newRow( "data27" ) << bool(false) << keys << expected;
    }
    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<16; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 16, 0 );
        QTest::newRow( "data28" ) << bool(false) << keys << expected;
    }
    { // test maximum value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<59; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 59, 0 );
        QTest::newRow( "data29" ) << bool(false) << keys << expected;
    }
    { // test 'overflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<60; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data30" ) << bool(false) << keys << expected;
    }
    { // test 'underflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 59, 0 );
        QTest::newRow( "data31" ) << bool(false) << keys << expected;
    }
    { // test valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 58, 0 );
        QTest::newRow( "data32" ) << bool(false) << keys << expected;
    }

    // enter a valid one digit minute in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Tab);
        keys.addKeyClick( '2' );
        QTime expected( 11, 2, 0 );
        QTest::newRow( "data33" ) << bool(true) << keys << expected;
    }

    // enter a valid two digit minutes in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Tab);
        keys.addKeyClick( '2' );
        keys.addKeyClick( '4' );
        QTime expected( 11, 24, 0 );
        QTest::newRow( "data34" ) << bool(true) << keys << expected;
    }

    // check the lower limit of the minutes in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Tab);
        keys.addKeyClick( '0' );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data35" ) << bool(true) << keys << expected;
    }

    // check the upper limit of the minutes in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Tab);
        keys.addKeyClick( '5' );
        keys.addKeyClick( '9' );
        QTime expected( 11, 59, 0 );
        QTest::newRow( "data36" ) << bool(true) << keys << expected;
    }

    // enter an invalid two digit minutes in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '6' );
        keys.addKeyClick( '0' );
        QTime expected( 11, 6, 0 );
        QTest::newRow( "data37" ) << bool(true) << keys << expected;
    }

    // test minutes in 24 hour motestWidget-> Behaviour should be exactly the same

    // enter a valid one digit minute in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '2' );
        QTime expected( 11, 2, 0 );
        QTest::newRow( "data38" ) << bool(false) << keys << expected;
    }

    // enter a valid two digit minutes in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '2' );
        keys.addKeyClick( '4' );
        QTime expected( 11, 24, 0 );
        QTest::newRow( "data39" ) << bool(false) << keys << expected;
    }

    // check the lower limit of the minutes in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '0' );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data40" ) << bool(false) << keys << expected;
    }

    // check the upper limit of the minutes in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '5' );
        keys.addKeyClick( '9' );
        QTime expected( 11, 59, 0 );
        QTest::newRow( "data41" ) << bool(false) << keys << expected;
    }

    // enter an invalid two digit minutes in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '6' );
        keys.addKeyClick( '0' );
        QTime expected( 11, 6, 0 );
        QTest::newRow( "data42" ) << bool(false) << keys << expected;
    }

    // ***************** test the seconds ***************

    // use up/down to edit the seconds...

    // use up/down keys to change the seconds in 12 hour mode
    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 2 );
        QTest::newRow( "data43" ) << bool(true) << keys << expected;
    }
    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<16; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 16 );
        QTest::newRow( "data44" ) << bool(true) << keys << expected;
    }
    { // test maximum value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<59; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 59 );
        QTest::newRow( "data45" ) << bool(true) << keys << expected;
    }
    { // test 'overflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<60; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data46" ) << bool(true) << keys << expected;
    }
    { // test 'underflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 0, 59 );
        QTest::newRow( "data47" ) << bool(true) << keys << expected;
    }
    { // test valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 0, 58 );
        QTest::newRow( "data48" ) << bool(true) << keys << expected;
    }

    // use up/down keys to change the seconds in 24 hour mode

    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 2 );
        QTest::newRow( "data49" ) << bool(false) << keys << expected;
    }
    { // test a valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<16; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 16 );
        QTest::newRow( "data50" ) << bool(false) << keys << expected;
    }
    { // test maximum value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<59; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 59 );
        QTest::newRow( "data51" ) << bool(false) << keys << expected;
    }
    { // test 'overflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<60; i++)
            keys.addKeyClick( Qt::Key_Up );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data52" ) << bool(false) << keys << expected;
    }
    { // test 'underflow'
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 0, 59 );
        QTest::newRow( "data53" ) << bool(false) << keys << expected;
    }
    { // test valid value
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        for (uint i=0; i<2; i++)
            keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 0, 58 );
        QTest::newRow( "data54" ) << bool(false) << keys << expected;
    }

    // enter a valid one digit second in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '2' );
        QTime expected( 11, 0, 2 );
        QTest::newRow( "data55" ) << bool(true) << keys << expected;
    }

    // enter a valid two digit seconds in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '2' );
        keys.addKeyClick( '4' );
        QTime expected( 11, 0, 24 );
        QTest::newRow( "data56" ) << bool(true) << keys << expected;
    }

    // check the lower limit of the seconds in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '0' );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data57" ) << bool(true) << keys << expected;
    }

    // check the upper limit of the seconds in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '5' );
        keys.addKeyClick( '9' );
        QTime expected( 11, 0, 59 );
        QTest::newRow( "data58" ) << bool(true) << keys << expected;
    }

    // enter an invalid two digit seconds in 12 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '6' );
        keys.addKeyClick( '0' );
        QTime expected( 11, 0, 6 );
        QTest::newRow( "data59" ) << bool(true) << keys << expected;
    }

    // test seconds in 24 hour mode. Behaviour should be exactly the same

    // enter a valid one digit minute in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '2' );
        QTime expected( 11, 0, 2 );
        QTest::newRow( "data60" ) << bool(false) << keys << expected;
    }

    // enter a valid two digit seconds in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '2' );
        keys.addKeyClick( '4' );
        QTime expected( 11, 0, 24 );
        QTest::newRow( "data61" ) << bool(false) << keys << expected;
    }

    // check the lower limit of the seconds in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '0' );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data62" ) << bool(false) << keys << expected;
    }

    // check the upper limit of the seconds in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '5' );
        keys.addKeyClick( '9' );
        QTime expected( 11, 0, 59 );
        QTest::newRow( "data63" ) << bool(false) << keys << expected;
    }

    // enter an invalid two digit seconds in 24 h mode
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( '6' );
        keys.addKeyClick( '0' );
        QTime expected( 11, 0, 6 );
        QTest::newRow( "data64" ) << bool(false) << keys << expected;
    }

    // Test the AMPM indicator
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Up );
        QTime expected( 23, 0, 0 );
        QTest::newRow( "data65" ) << bool(true) << keys << expected;
    }
    // Test the AMPM indicator
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 23, 0, 0 );
        QTest::newRow( "data66" ) << bool(true) << keys << expected;
    }
    // Test the AMPM indicator
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Down );
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data67" ) << bool(true) << keys << expected;
    }
    // Test the AMPM indicator
    {
        QTestEventList keys;
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Tab );
        keys.addKeyClick( Qt::Key_Up );
        keys.addKeyClick( Qt::Key_Down );
        QTime expected( 11, 0, 0 );
        QTest::newRow( "data68" ) << bool(true) << keys << expected;
    }
}

void tst_QDateTimeEdit::userKeyPress_Time()
{
    QFETCH(bool, ampm);
    QFETCH(QTestEventList, keys);
    QFETCH(QTime, expected_time);

    if (ampm)
        testWidget->setDisplayFormat("hh:mm:ss ap");
    else
        testWidget->setDisplayFormat("hh:mm:ss");

    testWidget->setTime(QTime(11, 0, 0));
    testWidget->setFocus();

    testWidget->setWrapping(true);

    QTest::keyClick(testWidget, Qt::Key_Enter); // Make sure the first section is now focused
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("11"));
    keys.simulate(testWidget);
    QTest::keyClick(testWidget, Qt::Key_Enter);

    QCOMPARE(testWidget->time(), expected_time);
}

void tst_QDateTimeEdit::wrappingDate_data()
{
    QTest::addColumn<bool>("startWithMin");
    QTest::addColumn<QDate>("minimumDate");
    QTest::addColumn<QDate>("maximumDate");
    QTest::addColumn<uint>("section");
    QTest::addColumn<QDate>("newDate");

    QTest::newRow("data0") << false << QDate(1999, 1, 1) << QDate(1999, 1, 31) << (uint)QDateTimeEdit::DaySection
                        << QDate(1999, 1, 1);
    QTest::newRow("data1") << true << QDate(1999, 1, 1) << QDate(1999, 1, 31) << (uint)QDateTimeEdit::DaySection
                        << QDate(1999, 1, 31);
    QTest::newRow("data2") << false << QDate(1999, 1, 1) << QDate(1999, 1, 31) << (uint)QDateTimeEdit::MonthSection
                        << QDate(1999, 1, 31);
    QTest::newRow("data3") << true << QDate(1999, 1, 1) << QDate(1999, 1, 31) << (uint)QDateTimeEdit::MonthSection
                        << QDate(1999, 1, 1);
    QTest::newRow("data4") << false << QDate(1999, 1, 1) << QDate(1999, 1, 31) << (uint)QDateTimeEdit::YearSection
                        << QDate(1999, 1, 31);
    QTest::newRow("data5") << true << QDate(1999, 1, 1) << QDate(1999, 1, 31) << (uint)QDateTimeEdit::YearSection
                        << QDate(1999, 1, 1);
    QTest::newRow("data6") << false << QDate(1999, 1, 1) << QDate(2000, 1, 31) << (uint)QDateTimeEdit::DaySection
                        << QDate(2000, 1, 1);
    QTest::newRow("data7") << true << QDate(1999, 1, 1) << QDate(2000, 1, 31) << (uint)QDateTimeEdit::DaySection
                        << QDate(1999, 1, 31);
    QTest::newRow("data8") << false << QDate(1999, 1, 1) << QDate(2000, 1, 31) << (uint)QDateTimeEdit::MonthSection
                        << QDate(2000, 1, 31);
    QTest::newRow("data9") << true << QDate(1999, 1, 1) << QDate(2000, 1, 31) << (uint)QDateTimeEdit::MonthSection
                        << QDate(1999, 12, 1);
    QTest::newRow("data10") << false << QDate(1999, 1, 1) << QDate(2000, 1, 31) << (uint)QDateTimeEdit::YearSection
                         << QDate(1999, 1, 31);
    QTest::newRow("data11") << true << QDate(1999, 1, 1) << QDate(2000, 1, 31) << (uint)QDateTimeEdit::YearSection
                         << QDate(2000, 1, 1);
}

void tst_QDateTimeEdit::wrappingDate()
{
    QFETCH(bool, startWithMin);
    QFETCH(QDate, minimumDate);
    QFETCH(QDate, maximumDate);
    QFETCH(uint, section);
    QFETCH(QDate, newDate);

    testWidget->setDisplayFormat("dd/MM/yyyy");
    testWidget->setMinimumDate(minimumDate);
    testWidget->setMaximumDate(maximumDate);
    testWidget->setWrapping(true);
    testWidget->setCurrentSection((QDateTimeEdit::Section)section);

    if (startWithMin) {
        testWidget->setDate(minimumDate);
        QTest::keyClick(testWidget, Qt::Key_Down);
    } else {
        testWidget->setDate(maximumDate);
        QTest::keyClick(testWidget, Qt::Key_Up);
    }
    if (testWidget->currentSection() == QDateTimeEdit::MonthSection)
        QCOMPARE(testWidget->date(), newDate);
}

void tst_QDateTimeEdit::dateSignalChecking_data()
{
    QTest::addColumn<QDate>("originalDate");
    QTest::addColumn<QDate>("newDate");
    QTest::addColumn<int>("timesEmitted");

    QTest::newRow("data0") << QDate(2004, 06, 22) << QDate(2004, 07, 23) << 1;
    QTest::newRow("data1") << QDate(2004, 06, 22) << QDate(2004, 06, 22) << 0;
}

void tst_QDateTimeEdit::dateSignalChecking()
{
    QFETCH(QDate, originalDate);
    QFETCH(QDate, newDate);
    QFETCH(int, timesEmitted);

    testWidget->setDate(originalDate);

    QSignalSpy dateSpy(testWidget, SIGNAL(dateChanged(QDate)));
    QSignalSpy dateTimeSpy(testWidget, SIGNAL(dateTimeChanged(QDateTime)));
    QSignalSpy timeSpy(testWidget, SIGNAL(timeChanged(QTime)));

    testWidget->setDate(newDate);
    QCOMPARE(dateSpy.count(), timesEmitted);

    if (timesEmitted > 0) {
        QList<QVariant> list = dateSpy.takeFirst();
        QDate d;
        d = qvariant_cast<QDate>(list.at(0));
        QCOMPARE(d, newDate);
    }
    QCOMPARE(dateTimeSpy.count(), timesEmitted);
    QCOMPARE(timeSpy.count(), 0);
}

void tst_QDateTimeEdit::timeSignalChecking_data()
{
    QTest::addColumn<QTime>("originalTime");
    QTest::addColumn<QTime>("newTime");
    QTest::addColumn<int>("timesEmitted");

    QTest::newRow("data0") << QTime(15, 55, 00) << QTime(15, 17, 12) << 1;
    QTest::newRow("data1") << QTime(15, 55, 00) << QTime(15, 55, 00) << 0;
}

void tst_QDateTimeEdit::timeSignalChecking()
{
    QFETCH(QTime, originalTime);
    QFETCH(QTime, newTime);
    QFETCH(int, timesEmitted);

    testWidget->setTime(originalTime);

    testWidget->setDisplayFormat("hh:mm:ss");
    QSignalSpy dateSpy(testWidget, SIGNAL(dateChanged(QDate)));
    QSignalSpy dateTimeSpy(testWidget, SIGNAL(dateTimeChanged(QDateTime)));
    QSignalSpy timeSpy(testWidget, SIGNAL(timeChanged(QTime)));

    testWidget->setTime(newTime);
    QCOMPARE(timeSpy.count(), timesEmitted);

    if (timesEmitted > 0) {
        QList<QVariant> list = timeSpy.takeFirst();
        QTime t;
        t = qvariant_cast<QTime>(list.at(0));
        QCOMPARE(t, newTime);
    }
    QCOMPARE(dateTimeSpy.count(), timesEmitted);
    QCOMPARE(dateSpy.count(), 0);
}

void tst_QDateTimeEdit::dateTimeSignalChecking_data()
{
    QTest::addColumn<QDateTime>("originalDateTime");
    QTest::addColumn<QDateTime>("newDateTime");
    QTest::addColumn<int>("timesDateEmitted");
    QTest::addColumn<int>("timesTimeEmitted");
    QTest::addColumn<int>("timesDateTimeEmitted");

    QTest::newRow("data0") << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 0))
                        << QDateTime(QDate(2004, 7, 23), QTime(15, 17, 12))
                        << 1 << 1 << 1;
    QTest::newRow("data1") << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 0))
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 17, 12))
                        << 0 << 1 << 1;
    QTest::newRow("data2") << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 0))
                        << QDateTime(QDate(2004, 7, 23), QTime(15, 55, 0))
                        << 1 << 0 << 1;
    QTest::newRow("data3") << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 0))
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 0))
                        << 0 << 0 << 0;
}

void tst_QDateTimeEdit::dateTimeSignalChecking()
{
    QFETCH(QDateTime, originalDateTime);
    QFETCH(QDateTime, newDateTime);
    QFETCH(int, timesDateEmitted);
    QFETCH(int, timesTimeEmitted);
    QFETCH(int, timesDateTimeEmitted);

    testWidget->setDisplayFormat("dd/MM/yyyy hh:mm:ss");
    testWidget->setDateTime(originalDateTime);

    QSignalSpy dateSpy(testWidget, SIGNAL(dateChanged(QDate)));
    QSignalSpy timeSpy(testWidget, SIGNAL(timeChanged(QTime)));
    QSignalSpy dateTimeSpy(testWidget, SIGNAL(dateTimeChanged(QDateTime)));

    testWidget->setDateTime(newDateTime);
    QCOMPARE(dateSpy.count(), timesDateEmitted);
    if (timesDateEmitted > 0) {
        QCOMPARE(timesDateEmitted, 1);
        QList<QVariant> list = dateSpy.takeFirst();
        QDate d;
        d = qvariant_cast<QDate>(list.at(0));
        QCOMPARE(d, newDateTime.date());
    }
    QCOMPARE(timeSpy.count(), timesTimeEmitted);
    if (timesTimeEmitted > 0) {
        QList<QVariant> list = timeSpy.takeFirst();
        QTime t;
        t = qvariant_cast<QTime>(list.at(0));
        QCOMPARE(t, newDateTime.time());
    }
    QCOMPARE(dateTimeSpy.count(), timesDateTimeEmitted);
    if (timesDateTimeEmitted > 0) {
        QList<QVariant> list = dateTimeSpy.takeFirst();
        QDateTime dt;
        dt = qvariant_cast<QDateTime>(list.at(0));
        QCOMPARE(dt, newDateTime);
    }
}

void tst_QDateTimeEdit::sectionText_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<uint>("section");
    QTest::addColumn<QString>("sectionText");

    QTest::newRow("data0") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::NoSection << QString();
    QTest::newRow("data1") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::AmPmSection << QString("pm");
    QTest::newRow("data2") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::MSecSection << QString("789");
    QTest::newRow("data3") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::SecondSection << QString("03");
    QTest::newRow("data4") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::MinuteSection << QString("55");
    QTest::newRow("data5") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::HourSection << QString("03");
    QTest::newRow("data6") << QString("dd/MM/yyyy hh:mm:ss zzz")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::HourSection << QString("15");
    QTest::newRow("data7") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::DaySection << QString("22");
    QTest::newRow("data8") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::MonthSection << QString("06");
    QTest::newRow("data9") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                        << (uint)QDateTimeEdit::YearSection << QString("2004");
    QTest::newRow("data10") << QString("dd/MM/yyyy hh:mm:ss zzz AP")
                         << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                         << (uint)QDateTimeEdit::AmPmSection << QString("PM");
    QTest::newRow("data11") << QString("dd/MM/yyyy hh:mm:ss ap")
                         << QDateTime(QDate(2004, 6, 22), QTime(15, 55, 3, 789))
                         << (uint)QDateTimeEdit::MSecSection << QString();
}

void tst_QDateTimeEdit::sectionText()
{
    QFETCH(QString, format);
    QFETCH(QDateTime, dateTime);
    QFETCH(uint, section);
    QFETCH(QString, sectionText);

    testWidget->setDisplayFormat(format);
    testWidget->setDateTime(dateTime);
    QCOMPARE(testWidget->sectionText((QDateTimeEdit::Section)section), sectionText);
//    QApplication::setLayoutDirection(Qt::RightToLeft);
//    testWidget->setDisplayFormat(format);
//    QCOMPARE(format, testWidget->displayFormat());
//     testWidget->setDateTime(dateTime);
//     QCOMPARE(testWidget->sectionText((QDateTimeEdit::Section)section), sectionText);
//     QApplication::setLayoutDirection(Qt::LeftToRight);
}

void tst_QDateTimeEdit::mousePress()
{
    testWidget->setDate(QDate(2004, 6, 23));
    testWidget->setCurrentSection(QDateTimeEdit::YearSection);
    QCOMPARE(testWidget->currentSection(), QDateTimeEdit::YearSection);

    // Ask the SC_SpinBoxUp button location from style
    QStyleOptionSpinBox so;
    so.rect = testWidget->rect();
    QRect rectUp = testWidget->style()->subControlRect(QStyle::CC_SpinBox, &so, QStyle::SC_SpinBoxUp, testWidget);

    // Send mouseClick to center of SC_SpinBoxUp
    QTest::mouseClick(testWidget, Qt::LeftButton, 0, rectUp.center());
    QCOMPARE(testWidget->date().year(), 2005);
}

void tst_QDateTimeEdit::stepHourAMPM_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<KeyList>("keys");
    QTest::addColumn<TimeList>("expected");
    QTest::addColumn<QTime>("start");
    QTest::addColumn<QTime>("min");
    QTest::addColumn<QTime>("max");

    {
        KeyList keys;
        TimeList expected;
        keys << Qt::Key_Up;
        expected << QTime(1, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(2, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(3, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(4, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(5, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(6, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(7, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(8, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(9, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(10, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(11, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(12, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(13, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(14, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(15, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(16, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(17, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(18, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(19, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(20, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(21, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(22, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(23, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(22, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(21, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(20, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(19, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(18, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(17, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(16, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(15, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(14, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(13, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(12, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(11, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(10, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(9, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(8, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(7, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(6, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(5, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(4, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(3, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(2, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(1, 0, 0);
        keys << Qt::Key_Down;
        expected << QTime(0, 0, 0);

        QTest::newRow("hh 1") << QString("hh") << keys << expected << QTime(0, 0)
                              << QTime(0, 0) << QTime(23, 59, 59);
        QTest::newRow("hh:ap 1") << QString("hh:ap") << keys << expected
                                 << QTime(0, 0) << QTime(0, 0)
                                 << QTime(23, 59, 59);

        QTest::newRow("HH:ap 2") << QString("HH:ap") << keys << expected
                                 << QTime(0, 0) << QTime(0, 0)
                                 << QTime(23, 59, 59);
    }
    {
        KeyList keys;
        TimeList expected;
        keys << Qt::Key_Down;
        expected << QTime(2, 0, 0);
        QTest::newRow("hh 2") << QString("hh") << keys << expected << QTime(0, 0) << QTime(2, 0, 0) << QTime(23, 59, 59);
        QTest::newRow("hh:ap 2") << QString("hh:ap") << keys << expected << QTime(0, 0) << QTime(2, 0, 0) << QTime(23, 59, 59);
    }
    {
        KeyList keys;
        TimeList expected;
        keys << Qt::Key_Up;
        expected << QTime(23, 0, 0);
        keys << Qt::Key_Up;
        expected << QTime(23, 0, 0);
        QTest::newRow("hh 3") << QString("hh") << keys << expected << QTime(0, 0) << QTime(22, 0, 0)
                              << QTime(23, 59, 59);
        QTest::newRow("hh:ap 3") << QString("hh:ap") << keys << expected << QTime(0, 0)
                                 << QTime(22, 0, 0) << QTime(23, 59, 59);
    }
    {
        KeyList keys;
        TimeList expected;
        keys << Qt::Key_Up;
        expected << QTime(15, 31, 0);
        QTest::newRow("hh:mm:ap 3") << QString("hh:mm:ap") << keys << expected << QTime(15, 31, 0)
                                    << QTime(9, 0, 0) << QTime(16, 0, 0);
        QTest::newRow("hh:mm 3") << QString("hh:mm") << keys << expected << QTime(15, 31, 0)
                                 << QTime(9, 0, 0) << QTime(16, 0, 0);
    }
}

void tst_QDateTimeEdit::stepHourAMPM()
{
    QFETCH(QString, format);
    QFETCH(KeyList, keys);
    QFETCH(TimeList, expected);
    QFETCH(QTime, start);
    QFETCH(QTime, min);
    QFETCH(QTime, max);

    testWidget->setDisplayFormat(format);
    testWidget->setTime(start);
    testWidget->setMinimumTime(min);
    testWidget->setMaximumTime(max);
    if (keys.size() != expected.size()) {
        qWarning("%s:%d Test broken", __FILE__, __LINE__);
        QCOMPARE(keys.size(), expected.size());
    }

    for (int i=0; i<keys.size(); ++i) {
        QTest::keyClick(testWidget, keys.at(i));
        QCOMPARE(testWidget->time(), expected.at(i));
    }
}

void tst_QDateTimeEdit::displayedSections_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<uint>("section");

    QTest::newRow("data0") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)(QDateTimeEdit::DaySection | QDateTimeEdit::MonthSection
                                  | QDateTimeEdit::YearSection | QDateTimeEdit::HourSection
                                  | QDateTimeEdit::MinuteSection | QDateTimeEdit::SecondSection
                                  | QDateTimeEdit::MSecSection | QDateTimeEdit::AmPmSection);
    QTest::newRow("data1") << QString("dd/yyyy hh:mm:ss zzz ap")
                        << (uint)(QDateTimeEdit::DaySection
                                  | QDateTimeEdit::YearSection | QDateTimeEdit::HourSection
                                  | QDateTimeEdit::MinuteSection | QDateTimeEdit::SecondSection
                                  | QDateTimeEdit::MSecSection | QDateTimeEdit::AmPmSection);
    QTest::newRow("data2") << QString("dd/MM/yyyy mm zzz ap")
                        << (uint)(QDateTimeEdit::DaySection | QDateTimeEdit::MonthSection
                                  | QDateTimeEdit::YearSection
                                  | QDateTimeEdit::MinuteSection
                                  | QDateTimeEdit::MSecSection | QDateTimeEdit::AmPmSection);
    QTest::newRow("data3") << QString("dd/MM/yyyy")
                        << (uint)(QDateTimeEdit::DaySection | QDateTimeEdit::MonthSection
                                  | QDateTimeEdit::YearSection);
    QTest::newRow("data4") << QString("hh:mm:ss zzz ap")
                        << (uint)(QDateTimeEdit::HourSection
                                  | QDateTimeEdit::MinuteSection | QDateTimeEdit::SecondSection
                                  | QDateTimeEdit::MSecSection | QDateTimeEdit::AmPmSection);
    QTest::newRow("data5") << QString("dd ap")
                        << (uint)(QDateTimeEdit::DaySection | QDateTimeEdit::AmPmSection);
    QTest::newRow("data6") << QString("zzz")
                        << (uint)QDateTimeEdit::MSecSection;
}

void tst_QDateTimeEdit::displayedSections()
{
    QFETCH(QString, format);
    QFETCH(uint, section);

    testWidget->setDisplayFormat(format);
    QCOMPARE(QDateTimeEdit::Sections(section), testWidget->displayedSections());
}

void tst_QDateTimeEdit::currentSection_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<uint>("section");
    QTest::addColumn<uint>("currentSection");

    // First is deliberate, this way we can make sure that it is not reset by specifying no section.
    QTest::newRow("data0") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::NoSection << (uint)QDateTimeEdit::YearSection;
    QTest::newRow("data1") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::AmPmSection << (uint)QDateTimeEdit::AmPmSection;
    QTest::newRow("data2") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::MSecSection << (uint)QDateTimeEdit::MSecSection;
    QTest::newRow("data3") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::SecondSection << (uint)QDateTimeEdit::SecondSection;
    QTest::newRow("data4") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::MinuteSection << (uint)QDateTimeEdit::MinuteSection;
    QTest::newRow("data5") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::HourSection << (uint)QDateTimeEdit::HourSection;
    QTest::newRow("data6") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::DaySection << (uint)QDateTimeEdit::DaySection;
    QTest::newRow("data7") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::MonthSection << (uint)QDateTimeEdit::MonthSection;
    QTest::newRow("data8") << QString("dd/MM/yyyy hh:mm:ss zzz ap")
                        << (uint)QDateTimeEdit::YearSection << (uint)QDateTimeEdit::YearSection;
    QTest::newRow("data9") << QString("dd/MM/yyyy hh:mm:ss zzz AP")
                        << (uint)QDateTimeEdit::AmPmSection << (uint)QDateTimeEdit::AmPmSection;
    QTest::newRow("data10") << QString("dd/MM/yyyy hh:mm:ss ap")
                         << (uint)QDateTimeEdit::MSecSection << (uint)QDateTimeEdit::DaySection;
}

void tst_QDateTimeEdit::currentSection()
{
    QFETCH(QString, format);
    QFETCH(uint, section);
    QFETCH(uint, currentSection);

    testWidget->setDisplayFormat(format);
    if ((QDateTimeEdit::Section)section == QDateTimeEdit::NoSection)
        testWidget->setCurrentSection(QDateTimeEdit::YearSection); // Ensure it's not reset (see above)
    testWidget->setCurrentSection((QDateTimeEdit::Section)section);
    QCOMPARE((QDateTimeEdit::Section)currentSection, testWidget->currentSection());
}

void tst_QDateTimeEdit::readOnly()
{
    testWidget->hide();
    QDateTimeEdit dt(QDate(2000, 2, 1));
    dt.setDisplayFormat("yyyy.MM.dd");
    dt.show();
    dt.setCurrentSection(QDateTimeEdit::DaySection);
    QTest::keyClick(&dt, Qt::Key_Up);
    QCOMPARE(dt.date(), QDate(2000, 2, 2));
    dt.setReadOnly(true);
    QTest::keyClick(&dt, Qt::Key_Up);
    QCOMPARE(dt.date(), QDate(2000, 2, 2));
    dt.stepBy(1); // stepBy should still work
    QCOMPARE(dt.date(), QDate(2000, 2, 3));
    dt.setReadOnly(false);
    QTest::keyClick(&dt, Qt::Key_Up);
    QCOMPARE(dt.date(), QDate(2000, 2, 4));
}

void tst_QDateTimeEdit::weirdCase()
{
    testWidget->lineEdit()->setCursorPosition(0);
    testWidget->setDateRange(QDate(2005, 1, 1), QDate(2010, 12, 31));
    testWidget->setDisplayFormat("dd//MM//yyyy");
    testWidget->setDate(testWidget->minimumDate());
    QTest::keyClick(testWidget, Qt::Key_Left);
    QVERIFY(!testWidget->lineEdit()->hasSelectedText());
    QCOMPARE(testWidget->lineEdit()->cursorPosition(), 0);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QVERIFY(!testWidget->lineEdit()->hasSelectedText());
    QCOMPARE(testWidget->lineEdit()->cursorPosition(), 8);

    QTest::keyClick(testWidget, Qt::Key_Delete);
    QCOMPARE(testWidget->text(), QString("01//01//005"));
    QTest::keyClick(testWidget, Qt::Key_4);
    QCOMPARE(testWidget->text(), QString("01//01//005"));
}

void tst_QDateTimeEdit::newCase()
{
    if (QDate::shortMonthName(6) != "Jun" || QDate::shortMonthName(7) != "Jul" ||
        QDate::longMonthName(6) != "June" || QDate::longMonthName(7) != "July")
        QSKIP("This test only works in English");

    testWidget->setDisplayFormat("MMMM'a'MbMMMcMM");
    testWidget->setDate(QDate(2005, 6, 1));
    QCOMPARE(testWidget->text(), QString("Junea6bJunc06"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->text(), QString("Julya7bJulc07"));
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("July"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString());
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Right);
    QTest::keyClick(testWidget, Qt::Key_Delete);
    QTest::keyClick(testWidget, Qt::Key_Left);

    QCOMPARE(testWidget->text(), QString("Jula7bJulc07"));
    QTest::keyClick(testWidget, Qt::Key_Delete);
    QCOMPARE(testWidget->text(), QString("Jua7bJulc07"));
    QTest::keyClick(testWidget, Qt::Key_N);
    QCOMPARE(testWidget->text(), QString("Juna7bJulc07"));
    QTest::keyClick(testWidget, Qt::Key_E);
    QCOMPARE(testWidget->text(), QString("Junea6bJunc06"));
}

void tst_QDateTimeEdit::newCase2()
{
    testWidget->setDisplayFormat("MMMM yyyy-MM-dd MMMM");
    testWidget->setDate(QDate(2005, 8, 8));
    QTest::keyClick(testWidget, Qt::Key_Return);
    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->text(), QString(" 2005-08-08 ") + QDate::longMonthName(8));
}

void tst_QDateTimeEdit::newCase3()
{
    if (!QDate::longMonthName(1).startsWith("Januar"))
        QSKIP("This test does not work in this locale");

    testWidget->setDisplayFormat("dd MMMM yyyy");
    testWidget->setDate(QDate(2000, 1, 1));
    testWidget->setGeometry(QRect(QPoint(0, 0), testWidget->sizeHint()));
    testWidget->setCurrentSection(QDateTimeEdit::MonthSection);
    QTest::keyClick(testWidget, Qt::Key_Return);
    QTest::keyClick(testWidget, Qt::Key_J);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("01 J 2000"));
    QCOMPARE(testWidget->lineEdit()->cursorPosition(), 4);
    QTest::keyClick(testWidget, Qt::Key_A);
    QTest::keyClick(testWidget, Qt::Key_N);
    QTest::keyClick(testWidget, Qt::Key_U);
    QTest::keyClick(testWidget, Qt::Key_A);
    QTest::keyClick(testWidget, Qt::Key_R);
}

void tst_QDateTimeEdit::cursorPos()
{
    if (QDate::longMonthName(1) != "January")
        QSKIP("This test only works in English");

    testWidget->setDisplayFormat("dd MMMM yyyy");
    //testWidget->setGeometry(0, 0, 200, 200);
    testWidget->setCurrentSection(QDateTimeEdit::MonthSection);
    QTest::keyClick(testWidget, Qt::Key_Return);
    QCOMPARE(testWidget->lineEdit()->cursorPosition(), 10);
    QTest::keyClick(testWidget, Qt::Key_J);
    QTest::keyClick(testWidget, Qt::Key_A);
    QTest::keyClick(testWidget, Qt::Key_N);
    QTest::keyClick(testWidget, Qt::Key_U);
    QTest::keyClick(testWidget, Qt::Key_A);
    QTest::keyClick(testWidget, Qt::Key_R);
    //QCursor::setPos(20, 20);
    //QEventLoop l;
    //l.exec();
    QTest::keyClick(testWidget, Qt::Key_Y);
    QCOMPARE(testWidget->lineEdit()->cursorPosition(), 11);
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
    QTest::keyClick(testWidget, Qt::Key_Return);
    QTest::keyClick(testWidget, Qt::Key_3);
    QTest::keyClick(testWidget, Qt::Key_1);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(testWidget->lineEdit()->cursorPosition(), 3);
}

void tst_QDateTimeEdit::newCase4()
{
    testWidget->setDisplayFormat("hh:mm");
    testWidget->setMinimumTime(QTime(3, 3, 0));
    QTest::keyClick(testWidget, Qt::Key_Return);
    QTest::keyClick(testWidget, Qt::Key_0);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("0:03"));
    QTest::keyClick(testWidget, Qt::Key_2);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("0:03"));
    QTest::keyClick(testWidget, Qt::Key_4);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("04:03"));
}

void tst_QDateTimeEdit::newCase5()
{
    testWidget->setDisplayFormat("yyyy-MM-dd hh:mm:ss zzz 'ms'");
    testWidget->setDateTime(QDateTime(QDate(2005, 10, 7), QTime(17, 44, 13, 100)));
    testWidget->show();
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("2005-10-07 17:44:13 100 ms"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QTest::keyClick(testWidget, Qt::Key_Backtab, Qt::ShiftModifier);

    QTest::keyClick(testWidget, Qt::Key_Return);
    QTest::keyClick(testWidget, Qt::Key_1);
    QTest::keyClick(testWidget, Qt::Key_2);
    QTest::keyClick(testWidget, Qt::Key_4);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("2005-10-07 17:44:13 124 ms"));

    QTest::keyClick(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("2005-10-07 17:44:13 12 ms"));
}

void tst_QDateTimeEdit::newCase6()
{
    testWidget->setDisplayFormat("d-yyyy-MM-dd");
    testWidget->setDate(QDate(2005, 10, 7));
    testWidget->show();
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("7-2005-10-07"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
    QTest::keyClick(testWidget, Qt::Key_Return);
    QTest::keyClick(testWidget, Qt::Key_1);
    QTest::keyClick(testWidget, Qt::Key_2);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("12-2005-10-12"));
}

void tst_QDateTimeEdit::task98554()
{
    testWidget->setDisplayFormat("mm.ss.zzz(ms)");
    testWidget->setTime(QTime(0, 0, 9));
    testWidget->setCurrentSection(QDateTimeEdit::SecondSection);
    testWidget->show();
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("00.09.000(09)"));
    QCOMPARE(testWidget->time(), QTime(0, 0, 9, 0));
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("00.10.000(010)"));
    QCOMPARE(testWidget->time(), QTime(0, 0, 10, 0));
}

static QList<int> makeList(int val1, int val2, int val3)
{
    QList<int> ret;
    ret << val1 << val2 << val3;
    return ret;
}

void tst_QDateTimeEdit::setCurrentSection_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<QList<int> >("setCurrentSections");
    QTest::addColumn<QList<int> >("expectedCursorPositions");

    QTest::newRow("Day") << QString("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z") << QDateTime(QDate(2001, 1, 1), QTime(1, 2, 3, 4))
                         << makeList(QDateTimeEdit::DaySection, QDateTimeEdit::DaySection, QDateTimeEdit::DaySection)
                         << makeList(24, 0, 24);
    QTest::newRow("Month") << QString("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z") << QDateTime(QDate(2001, 1, 1), QTime(1, 2, 3, 4))
                           << makeList(QDateTimeEdit::MonthSection, QDateTimeEdit::MonthSection, QDateTimeEdit::MonthSection)
                           << makeList(3, 26, 3);
    QTest::newRow("Year") << QString("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z") << QDateTime(QDate(2001, 1, 1), QTime(1, 2, 3, 4))
                          << makeList(QDateTimeEdit::YearSection, QDateTimeEdit::YearSection, QDateTimeEdit::YearSection)
                          << makeList(6, 28, 6);
    QTest::newRow("Hour") << QString("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z") << QDateTime(QDate(2001, 1, 1), QTime(1, 2, 3, 4))
                          << makeList(QDateTimeEdit::HourSection, QDateTimeEdit::HourSection, QDateTimeEdit::HourSection)
                          << makeList(11, 31, 11);
    QTest::newRow("Minute") << QString("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z") << QDateTime(QDate(2001, 1, 1), QTime(1, 2, 3, 4))
                            << makeList(QDateTimeEdit::MinuteSection, QDateTimeEdit::MinuteSection, QDateTimeEdit::MinuteSection)
                            << makeList(14, 33, 14);
    QTest::newRow("Second") << QString("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z") << QDateTime(QDate(2001, 1, 1), QTime(1, 2, 3, 4))
                            << makeList(QDateTimeEdit::SecondSection, QDateTimeEdit::SecondSection, QDateTimeEdit::SecondSection)
                            << makeList(17, 35, 17);
    QTest::newRow("MSec") << QString("dd/MM/yyyy hh:mm:ss.zzz d/M/yy h:m:s.z") << QDateTime(QDate(2001, 1, 1), QTime(1, 2, 3, 4))
                          << makeList(QDateTimeEdit::MSecSection, QDateTimeEdit::MSecSection, QDateTimeEdit::MSecSection)
                          << makeList(20, 37, 20);
}

void tst_QDateTimeEdit::setCurrentSection()
{
    QFETCH(QString, format);
    QFETCH(QDateTime, dateTime);
    QFETCH(QList<int>, setCurrentSections);
    QFETCH(QList<int>, expectedCursorPositions);

    QCOMPARE(setCurrentSections.size(), expectedCursorPositions.size());
    testWidget->setDisplayFormat(format);
    testWidget->setDateTime(dateTime);
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif

    testWidget->resize(400, 100);
    for (int i=0; i<setCurrentSections.size(); ++i) {
        testWidget->setCurrentSection((QDateTimeEdit::Section)setCurrentSections.at(i));
        QCOMPARE(testWidget->currentSection(), (QDateTimeEdit::Section)setCurrentSections.at(i));
        QCOMPARE(testWidget->lineEdit()->cursorPosition(), expectedCursorPositions.at(i));
    }
}

void tst_QDateTimeEdit::setSelectedSection()
{
    testWidget->setDisplayFormat("mm.ss.zzz('ms') m");
    testWidget->setTime(QTime(0, 0, 9));
    testWidget->show();
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_Home);
#endif
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QVERIFY(!testWidget->lineEdit()->hasSelectedText());
    testWidget->setSelectedSection(QDateTimeEdit::MinuteSection);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("00"));
    testWidget->setCurrentSection(QDateTimeEdit::MinuteSection);
    testWidget->setSelectedSection(QDateTimeEdit::MinuteSection);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("0"));
    testWidget->setSelectedSection(QDateTimeEdit::SecondSection);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("09"));
    testWidget->setSelectedSection(QDateTimeEdit::NoSection);
    QVERIFY(!testWidget->lineEdit()->hasSelectedText());
}

void tst_QDateTimeEdit::calendarPopup()
{
    {
        QDateEdit edit;
        QVERIFY(!edit.calendarWidget());
        edit.setCalendarPopup(true);
        QVERIFY(edit.calendarWidget());
    }

    {
        QTimeEdit edit;
        QVERIFY(!edit.calendarWidget());
        edit.setCalendarPopup(true);
        QVERIFY(!edit.calendarWidget());
    }

    {
        QDateEdit edit;
        QVERIFY(!edit.calendarWidget());
        QCalendarWidget *cw = new QCalendarWidget;
        edit.setCalendarWidget(cw);
        QVERIFY(!edit.calendarWidget());
        edit.setCalendarPopup(true);
        edit.setCalendarWidget(cw);
        QCOMPARE(edit.calendarWidget(), cw);
        edit.setDateRange(QDate(1980, 1, 5), QDate(1980, 2, 11));
        QCOMPARE(cw->minimumDate(), edit.minimumDate());
        QCOMPARE(cw->maximumDate(), edit.maximumDate());
        edit.setDate(QDate(1980, 1, 31));
        QCOMPARE(edit.date(), cw->selectedDate());
        cw->setSelectedDate(QDate(1980, 1, 30));
        QCOMPARE(edit.date(), cw->selectedDate());
    }

    testWidget->setDisplayFormat("dd/MM/yyyy");
    testWidget->setDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0)));
    testWidget->show();
    testWidget->setCalendarPopup(true);
    QCOMPARE(testWidget->calendarPopup(), true);
    QStyle *style = testWidget->style();
    QStyleOptionComboBox opt;
    opt.initFrom(testWidget);
    opt.editable = true;
    opt.subControls = QStyle::SC_ComboBoxArrow;
    QRect rect = style->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, testWidget);
    QTest::mouseClick(testWidget, Qt::LeftButton, 0, QPoint(rect.left()+rect.width()/2, rect.top()+rect.height()/2));
    QWidget *wid = testWidget->findChild<QWidget *>("qt_datetimedit_calendar");
    QVERIFY(wid != 0);
    testWidget->hide();

    QTimeEdit timeEdit;
    timeEdit.setCalendarPopup(true);
    timeEdit.show();

    opt.initFrom(&timeEdit);
    opt.subControls = QStyle::SC_ComboBoxArrow;
    rect = style->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, &timeEdit);
    QTest::mouseClick(&timeEdit, Qt::LeftButton, 0, QPoint(rect.left()+rect.width()/2, rect.top()+rect.height()/2));
    QWidget *wid2 = timeEdit.findChild<QWidget *>("qt_datetimedit_calendar");
    QVERIFY(!wid2);
    timeEdit.hide();


    QDateEdit dateEdit;
    dateEdit.setCalendarPopup(true);
    dateEdit.setReadOnly(true);
    dateEdit.show();

    opt.initFrom(&dateEdit);
    opt.subControls = QStyle::SC_ComboBoxArrow;
    rect = style->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, &dateEdit);
    QTest::mouseClick(&dateEdit, Qt::LeftButton, 0, QPoint(rect.left()+rect.width()/2, rect.top()+rect.height()/2));
    QWidget *wid3 = dateEdit.findChild<QWidget *>("qt_datetimedit_calendar");
    QVERIFY(!wid3);
    dateEdit.hide();
}

class RestoreLayoutDirectioner
{
public:
    RestoreLayoutDirectioner(Qt::LayoutDirection was)
        : old(was)
    {}

    ~RestoreLayoutDirectioner()
    {
        QApplication::setLayoutDirection(old);
    }
private:
    const Qt::LayoutDirection old;
};

void tst_QDateTimeEdit::reverseTest()
{
    const RestoreLayoutDirectioner restorer(QApplication::layoutDirection());
    QApplication::setLayoutDirection(Qt::RightToLeft);
    testWidget->setDisplayFormat("dd/MM/yyyy");
    testWidget->setDate(QDate(2001, 3, 30));
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("2001/03/30"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(testWidget->currentSection(), QDateTimeEdit::DaySection);
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2001, 3, 31));
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("2001/03/31"));
}

void tst_QDateTimeEdit::hour12Test()
{
    testWidget->setDisplayFormat("hh a");
    testWidget->setTime(QTime(0, 0, 0));
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("12 am"));
    for (int i=0; i<11; ++i) {
        QTest::keyClick(testWidget, Qt::Key_Up);
    }
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("11 am"));
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("12 pm"));
    for (int i=0; i<11; ++i) {
        QTest::keyClick(testWidget, Qt::Key_Up);
    }
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("11 pm"));
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("11 pm"));
    for (int i=0; i<12; ++i) {
        QTest::keyClick(testWidget, Qt::Key_Down);
    }
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("11 am"));
    QTest::keyClick(testWidget, Qt::Key_1);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("1 am"));
    QTest::keyClick(testWidget, Qt::Key_3);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("1 am"));
}

void tst_QDateTimeEdit::yyTest()
{
    testWidget->setDisplayFormat("dd-MMM-yy");
    testWidget->setTime(QTime(0, 0, 0));
    testWidget->setDateRange(QDate(2005, 1, 1), QDate(2010, 12, 31));
    testWidget->setDate(testWidget->minimumDate());
    testWidget->setCurrentSection(QDateTimeEdit::YearSection);

    QString jan = QDate::shortMonthName(1);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-05");
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-06");
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-07");
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-08");
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-09");
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-10");
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-10");
    testWidget->setWrapping(true);
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->lineEdit()->displayText(), "01-" + jan + "-05");

    testWidget->setDateRange(QDate(100, 1, 1), QDate(8000, 12, 31));
    testWidget->setDate(QDate(2000, 1, 1));
    testWidget->setCurrentSection(QDateTimeEdit::YearSection);
    testWidget->setWrapping(false);
    for (int i=0; i<10; ++i) {
        for (int j=0; j<50; ++j) {
            testWidget->stepBy(-1);
        }
        testWidget->stepBy(-50);
        QCOMPARE(testWidget->sectionText(QDateTimeEdit::YearSection), QString("00"));
        QCOMPARE(testWidget->date(), QDate(2000 - ((i + 1) * 100), 1, 1));
    }
}

void tst_QDateTimeEdit::task108572()
{
    testWidget->setDisplayFormat("hh:mm:ss.zzz");
    testWidget->setTime(QTime(0, 1, 2, 0));
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("00:01:02.000"));

    testWidget->setCurrentSection(QDateTimeEdit::MSecSection);
    QTest::keyClick(testWidget, Qt::Key_Return);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("000"));
    QTest::keyClick(testWidget, Qt::Key_2);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("00:01:02.2"));
    QTest::keyClick(testWidget, Qt::Key_Return);
    QCOMPARE(testWidget->lineEdit()->displayText(), QString("00:01:02.200"));
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("200"));
    QCOMPARE(testWidget->time(), QTime(0, 1, 2, 200));
}

void tst_QDateTimeEdit::task149097()
{
    QSignalSpy dtSpy(testWidget, SIGNAL(dateTimeChanged(QDateTime)));
    QSignalSpy dSpy(testWidget, SIGNAL(dateChanged(QDate)));
    QSignalSpy tSpy(testWidget, SIGNAL(timeChanged(QTime)));

    testWidget->setDisplayFormat("yyyy/MM/dd hh:mm:ss");
    testWidget->setDateTime(QDateTime(QDate(2001, 02, 03), QTime(5, 1, 2)));
//    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(dtSpy.count(), 1);
    QCOMPARE(dSpy.count(), 1);
    QCOMPARE(tSpy.count(), 1);
    testWidget->setCurrentSection(QDateTimeEdit::YearSection);
    testWidget->stepBy(1);

    QCOMPARE(dtSpy.count(), 2);
    QCOMPARE(dSpy.count(), 2);
    QCOMPARE(tSpy.count(), 1);

    testWidget->setCurrentSection(QDateTimeEdit::MinuteSection);
    testWidget->stepBy(1);

    QCOMPARE(dtSpy.count(), 3);
    QCOMPARE(dSpy.count(), 2);
    QCOMPARE(tSpy.count(), 2);
}

void tst_QDateTimeEdit::task148725()
{
    testWidget->setDisplayFormat("dd/MM");
    testWidget->setDate(QDate(2001, 2, 27));
    testWidget->stepBy(1);
    QCOMPARE(testWidget->date(), QDate(2001, 2, 28));
    testWidget->stepBy(1);
    QCOMPARE(testWidget->date(), QDate(2001, 2, 28));
    testWidget->setWrapping(true);
    testWidget->stepBy(1);
    QCOMPARE(testWidget->date(), QDate(2001, 2, 1));
}

void tst_QDateTimeEdit::task148522()
{
    QTimeEdit edit;
    const QDateTime dt(QDate(2000, 12, 12), QTime(12, 13, 14, 15));
    edit.setDateTime(dt);
    QCOMPARE(edit.dateTime(), dt);
}

void tst_QDateTimeEdit::ddMMMMyyyy()
{
    testWidget->setDisplayFormat("dd.MMMM.yyyy");
    testWidget->setDate(QDate(2000, 1, 1));
    testWidget->setCurrentSection(QDateTimeEdit::YearSection);
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QCOMPARE(testWidget->lineEdit()->selectedText(), QString("2000"));
#ifdef Q_OS_MAC
    QTest::keyClick(testWidget, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(testWidget, Qt::Key_End);
#endif
    QTest::keyClick(testWidget, Qt::Key_Backspace);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(testWidget->lineEdit()->text(), "01." + QDate::longMonthName(1) + ".200");
}

#if QT_CONFIG(wheelevent)
void tst_QDateTimeEdit::wheelEvent()
{
    testWidget->setDisplayFormat("dddd/MM");
    testWidget->setDate(QDate(2000, 2, 21));
    testWidget->setCurrentSection(QDateTimeEdit::DaySection);
    QWheelEvent w(testWidget->lineEdit()->geometry().center(), 120, 0, 0);
    qApp->sendEvent(testWidget, &w);
    QCOMPARE(testWidget->date(), QDate(2000, 2, 22));
    testWidget->setCurrentSection(QDateTimeEdit::MonthSection);
    qApp->sendEvent(testWidget, &w);
    QCOMPARE(testWidget->date(), QDate(2000, 3, 22));
}
#endif // QT_CONFIG(wheelevent)

void tst_QDateTimeEdit::specialValueCornerCase()
{
    // if you set minimum to value it won't update since value won't
    // be bounded to anything. If you have a specialValueText it needs
    // to call updateEdit to make sure the text is changed

    QDateTimeEdit edit;
    edit.setSpecialValueText("foobar");
    edit.setMinimumDate(edit.date());
    QCOMPARE(edit.minimumDate(), edit.date());
    QCOMPARE(edit.text(), QString("foobar"));
}

void tst_QDateTimeEdit::cursorPositionOnInit()
{
    {
        EditorDateEdit edit;
        edit.setDisplayFormat("dd/MM");
        edit.show();
        QCOMPARE(edit.lineEdit()->cursorPosition(), 0);
    }

    {
        EditorDateEdit edit;
        edit.setDisplayFormat("dd/MM");
        edit.setSpecialValueText("special");
        edit.setMinimumDate(edit.date());
        edit.show();
        QCOMPARE(edit.lineEdit()->cursorPosition(), 7);
        // ### legacy behavior. Keep it like this rather than changing
        // ### but add a test none-the-less
    }
}

void tst_QDateTimeEdit::task118867()
{
    EditorDateEdit edit;
    edit.setDisplayFormat("hh:mm");
    edit.setMinimumTime(QTime(5, 30));
    edit.setMaximumTime(QTime(6, 30));
    QCOMPARE(edit.text(), QString("05:30"));
    edit.lineEdit()->setCursorPosition(5);
    QTest::keyClick(&edit, Qt::Key_Backspace);
    QCOMPARE(edit.text(), QString("05:3"));
    QTest::keyClick(&edit, Qt::Key_Backspace);
    QCOMPARE(edit.text(), QString("05:"));
    QTest::keyClick(&edit, Qt::Key_1);
    QCOMPARE(edit.text(), QString("05:"));
    QTest::keyClick(&edit, Qt::Key_2);
    QCOMPARE(edit.text(), QString("05:"));
    QTest::keyClick(&edit, Qt::Key_3);
    QCOMPARE(edit.text(), QString("05:3"));
    QTest::keyClick(&edit, Qt::Key_3);
    QCOMPARE(edit.text(), QString("05:33"));
}

void tst_QDateTimeEdit::nextPrevSection_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QString>("selectedText");

    QTest::newRow("tab") << Qt::Key_Tab << (Qt::KeyboardModifiers)Qt::NoModifier << QString("56");
    QTest::newRow("backtab") << Qt::Key_Backtab << (Qt::KeyboardModifiers)Qt::NoModifier << QString("12");
    QTest::newRow("shift-tab") << Qt::Key_Tab << (Qt::KeyboardModifiers)Qt::ShiftModifier << QString("12");
    QTest::newRow("/") << Qt::Key_Slash << (Qt::KeyboardModifiers)Qt::NoModifier << QString("56");
    QTest::newRow("b") << Qt::Key_B << (Qt::KeyboardModifiers)Qt::NoModifier << QString("56");
    QTest::newRow("c") << Qt::Key_C << (Qt::KeyboardModifiers)Qt::NoModifier << QString("56");

    // 1. mac doesn't do these,
    // 2. some WinCE devices do not have modifiers
#if !defined(Q_OS_DARWIN)
    QTest::newRow("ctrl-right") << Qt::Key_Right << (Qt::KeyboardModifiers)Qt::ControlModifier << QString("56");
    QTest::newRow("ctrl-left") << Qt::Key_Left << (Qt::KeyboardModifiers)Qt::ControlModifier << QString("12");
#endif
}

void tst_QDateTimeEdit::nextPrevSection()
{
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QString, selectedText);

    EditorDateEdit edit;
    edit.setDisplayFormat("hh/mm/bc9ss");
    edit.setTime(QTime(12, 34, 56));
    edit.show();
    edit.setSelectedSection(QDateTimeEdit::MinuteSection);
    QCOMPARE(edit.lineEdit()->selectedText(), QString("34")); // selftest
    QTest::keyClick(&edit, key, modifiers);
    QCOMPARE(edit.lineEdit()->selectedText(), selectedText);
}

void tst_QDateTimeEdit::dateEditTimeEditFormats()
{
    QTimeEdit t;
    t.setDisplayFormat("hh yyyy");
    QCOMPARE(t.displayedSections(), QDateTimeEdit::HourSection);

    QDateEdit d;
    d.setDisplayFormat("hh yyyy");
    QCOMPARE(d.displayedSections(), QDateTimeEdit::YearSection);
}

void tst_QDateTimeEdit::timeSpec_data()
{
    QTest::addColumn<bool>("useSetProperty");
    QTest::newRow("setProperty") << true;
    QTest::newRow("setTimeSpec") << false;
}

void tst_QDateTimeEdit::timeSpec()
{
    QFETCH(bool, useSetProperty);

    QDateTimeEdit edit;
    QCOMPARE(edit.dateTime().timeSpec(), edit.timeSpec());
    QCOMPARE(edit.minimumDateTime().timeSpec(), edit.timeSpec());
    QCOMPARE(edit.maximumDateTime().timeSpec(), edit.timeSpec());
    if (useSetProperty) {
        edit.setProperty("timeSpec", Qt::UTC);
    } else {
        edit.setTimeSpec(Qt::UTC);
    }
    QCOMPARE(edit.minimumDateTime().timeSpec(), edit.timeSpec());
    QCOMPARE(edit.maximumDateTime().timeSpec(), edit.timeSpec());
    QCOMPARE(edit.dateTime().timeSpec(), edit.timeSpec());
    if (useSetProperty) {
        edit.setProperty("timeSpec", Qt::LocalTime);
    } else {
        edit.setTimeSpec(Qt::LocalTime);
    }
    const QDateTime dt = edit.dateTime();
    QCOMPARE(edit.timeSpec(), Qt::LocalTime);
    const QDateTime utc = dt.toUTC();
    if (dt.time() != utc.time()) {
        const QDateTime min(QDate(1999, 1, 1), QTime(1, 0, 0), Qt::LocalTime);
        edit.setMinimumDateTime(min);
        QCOMPARE(edit.minimumTime(), min.time());
        if (useSetProperty) {
            edit.setProperty("timeSpec", Qt::UTC);
        } else {
            edit.setTimeSpec(Qt::UTC);
        }
        QVERIFY(edit.minimumTime() != min.time());
        QVERIFY(edit.minimumDateTime().timeSpec() != min.timeSpec());
        QCOMPARE(edit.minimumDateTime().toTime_t(), min.toTime_t());
    } else {
        QSKIP("Not tested in the GMT timezone");
    }
}

void tst_QDateTimeEdit::timeSpecBug()
{
    testWidget->setTimeSpec(Qt::UTC);
    testWidget->setDisplayFormat("hh:mm");
    testWidget->setTime(QTime(2, 2));
    const QString oldText = testWidget->text();
    const QDateTime oldDateTime = testWidget->dateTime();
    QTest::keyClick(testWidget, Qt::Key_Tab);
    QCOMPARE(oldDateTime, testWidget->dateTime());
    QCOMPARE(oldText, testWidget->text());
}

void tst_QDateTimeEdit::timeSpecInit()
{
    QDateTime utc(QDate(2000, 1, 1), QTime(12, 0, 0), Qt::UTC);
    QDateTimeEdit widget(utc);
    QCOMPARE(widget.dateTime(), utc);
}

void tst_QDateTimeEdit::cachedDayTest()
{
    testWidget->setDisplayFormat("MM/dd");
    testWidget->setDate(QDate(2007, 1, 30));
    testWidget->setCurrentSection(QDateTimeEdit::DaySection);
    //QTest::keyClick(testWidget->lineEdit(), Qt::Key_Up); // this doesn't work
    //on Mac. Qt Test bug? ###
    QTest::keyClick(testWidget, Qt::Key_Up);
    testWidget->setCurrentSection(QDateTimeEdit::MonthSection);
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2007, 2, 28));
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2007, 3, 31));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2007, 2, 28));
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2007, 1, 31));

    testWidget->setCurrentSection(QDateTimeEdit::DaySection);
    QTest::keyClick(testWidget, Qt::Key_Down);
    QCOMPARE(testWidget->date(), QDate(2007, 1, 30));
    testWidget->setCurrentSection(QDateTimeEdit::MonthSection);
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2007, 2, 28));
    testWidget->setCurrentSection(QDateTimeEdit::MonthSection);
    QTest::keyClick(testWidget, Qt::Key_Up);
    QCOMPARE(testWidget->date(), QDate(2007, 3, 30));
}

void tst_QDateTimeEdit::monthEdgeCase()
{
    EditorDateEdit edit;
    edit.setLocale(QLocale("fr_FR"));
    edit.setDisplayFormat("MMM d");
    edit.setDate(QDate(2000, 1, 1));
    QCOMPARE(edit.text(), QString("janv. 1"));
    edit.lineEdit()->setCursorPosition(5);
    QTest::keyClick(&edit, Qt::Key_Backspace);
    QCOMPARE(edit.text(), QString("janv 1"));
}

class RestoreLocaler
{
public:
    RestoreLocaler()
        : old(QLocale())
    {}

    ~RestoreLocaler()
    {
        QLocale::setDefault(old);
    }
private:
    const QLocale old;
};

void tst_QDateTimeEdit::setLocale()
{
    RestoreLocaler r;
    QLocale::setDefault(QLocale("en_US"));
    {
        EditorDateEdit edit;
        edit.setDisplayFormat("MMMM d");
        edit.setDate(QDate(2000, 1, 1));
        QCOMPARE(edit.text(), QString("January 1"));
    }
    QLocale::setDefault(QLocale("no_NO"));
    {
        EditorDateEdit edit;
        edit.setDisplayFormat("MMMM d");
        edit.setDate(QDate(2000, 1, 1));
        QCOMPARE(edit.text().toLower(), QString("januar 1"));
        // I get Januar on Mac and januar on linux
    }
    QLocale::setDefault(QLocale("en_US"));
    {
        EditorDateEdit edit;
        edit.setDisplayFormat("MMMM d");
        edit.setDate(QDate(2000, 1, 1));
        QCOMPARE(edit.text(), QString("January 1"));
        edit.setLocale(QLocale("no_NO"));
        QCOMPARE(edit.text().toLower(), QString("januar 1"));
        edit.setLocale(QLocale("no_NO"));
    }
}

void tst_QDateTimeEdit::potentialYYValueBug()
{
    EditorDateEdit edit;
    edit.setDisplayFormat("dd/MM/yy");
    QCOMPARE(edit.minimumDate(), QDate(1752, 9, 14));
    edit.setDate(edit.minimumDate());
    edit.lineEdit()->setFocus();

#ifdef Q_OS_MAC
    QTest::keyClick(&edit, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(&edit, Qt::Key_End);
#endif
    QTest::keyClick(&edit, Qt::Key_Backspace);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(edit.text(), QString("14/09/5"));
}

void tst_QDateTimeEdit::textSectionAtEnd()
{
    EditorDateEdit edit;
    edit.setDisplayFormat("MMMM");
    edit.setDate(QDate(2000, 1, 1));
    edit.lineEdit()->setFocus();
#ifdef Q_OS_MAC
    QTest::keyClick(&edit, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(&edit, Qt::Key_End);
#endif
    QTest::keyClick(&edit, Qt::Key_Backspace);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23674", Abort);
#endif
    QCOMPARE(edit.text(), QString("Januar"));
}

void tst_QDateTimeEdit::keypadAutoAdvance_data()
{
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::newRow("None") << (Qt::KeyboardModifiers)Qt::NoModifier;
    QTest::newRow("Keypad") << (Qt::KeyboardModifiers)Qt::KeypadModifier;
    // QTBUG-7842: Using KeyPad with shift (numlock off)
    QTest::newRow("Keypad+Shift") << (Qt::KeyboardModifiers)(Qt::KeypadModifier|Qt::ShiftModifier);
}

void tst_QDateTimeEdit::keypadAutoAdvance()
{
    QFETCH(Qt::KeyboardModifiers, modifiers);

    EditorDateEdit edit;
    edit.setDate(QDate(2000, 2, 1));
    edit.setDisplayFormat("dd/MM");
#ifdef Q_OS_MAC
    QTest::keyClick(&edit, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(&edit, Qt::Key_Home);
#endif
    QTest::keyClick(&edit, Qt::Key_Return);
    QCOMPARE(edit.lineEdit()->selectedText(), QString("01"));
    QTest::keyClick(&edit, Qt::Key_1, modifiers);
    QTest::keyClick(&edit, Qt::Key_2, modifiers);
    QCOMPARE(edit.lineEdit()->selectedText(), QString("02"));
}

void tst_QDateTimeEdit::task196924()
{
    EditorDateEdit edit;
    edit.setDisplayFormat("dd/M/yyyy");
    edit.setDate(QDate(2345, 6, 17));
    QCOMPARE(edit.text(), QString("17/6/2345"));
    QTest::keyClick(&edit, Qt::Key_Tab);
    QCOMPARE(edit.lineEdit()->selectedText(), QString("6"));
    QTest::keyClick(&edit, Qt::Key_3);
    QCOMPARE(edit.lineEdit()->selectedText(), QString("2345"));
    QCOMPARE(edit.text(), QString("17/3/2345"));

    edit.setDisplayFormat("dd/MM/yyyy");
    edit.setDate(QDate(2345, 6, 17));
    edit.lineEdit()->setCursorPosition(0);
    QCOMPARE(edit.text(), QString("17/06/2345"));
    QTest::keyClick(&edit, Qt::Key_Tab);
    QCOMPARE(edit.lineEdit()->selectedText(), QString("06"));
    QTest::keyClick(&edit, Qt::Key_3);
    QCOMPARE(edit.lineEdit()->selectedText(), QString("2345"));
    QCOMPARE(edit.text(), QString("17/03/2345"));
}

void tst_QDateTimeEdit::focusNextPrevChild()
{
    EditorDateEdit edit;
    edit.setDisplayFormat("dd/MM/yyyy");

    edit.show();
    edit.setFocus();
    edit.setCurrentSection(QDateTimeEdit::DaySection);

    QCOMPARE(edit.currentSection(), QDateTimeEdit::DaySection);
    edit.focusNextPrevChild(true);
    QCOMPARE(edit.currentSection(), QDateTimeEdit::MonthSection);
}

void tst_QDateTimeEdit::taskQTBUG_12384_timeSpecShowTimeOnly()
{
    QDateTime time = QDateTime::fromString("20100723 04:02:40", "yyyyMMdd hh:mm:ss");
    time.setTimeSpec(Qt::UTC);

    EditorDateEdit edit;
    edit.setDisplayFormat("hh:mm:ss");
    edit.setTimeSpec(Qt::UTC);
    edit.setDateTime(time);

    QCOMPARE(edit.minimumTime(), QTime(0, 0, 0, 0));
    QCOMPARE(edit.maximumTime(), QTime(23, 59, 59, 999));
    QCOMPARE(edit.time(), time.time());
}

void tst_QDateTimeEdit::deleteCalendarWidget()
{
    {
        // setup
        QCalendarWidget *cw = 0;
        QDateEdit edit;
        QVERIFY(!edit.calendarWidget());
        edit.setCalendarPopup(true);
        QVERIFY(edit.calendarWidget());
        edit.calendarWidget()->setObjectName("cw1");;

        // delete
        cw = edit.calendarWidget();
        delete cw;

        // it should create a new widget
        QVERIFY(edit.calendarWidget());
        QVERIFY(edit.calendarWidget()->objectName() != QLatin1String("cw1"));
    }
}

void tst_QDateTimeEdit::setLocaleOnCalendarWidget()
{
    QDateEdit dateEdit;
    QList<QLocale> allLocales = QLocale::matchingLocales(
                QLocale::AnyLanguage,
                QLocale::AnyScript,
                QLocale::AnyCountry);
    QLocale c = QLocale::c();
    dateEdit.setCalendarPopup(true);
    dateEdit.setLocale(c);
    for (const QLocale& l : allLocales) {
        dateEdit.setLocale(l);
        const QLocale locCal = dateEdit.calendarWidget()->locale();
        const QLocale locEdit = dateEdit.locale();
        QCOMPARE(locCal.name(), locEdit.name());
        QVERIFY(locCal == locEdit);
    }
}

#ifdef QT_BUILD_INTERNAL

typedef QPair<Qt::Key, Qt::KeyboardModifier> KeyPair;
typedef QList<KeyPair> KeyPairList;

Q_DECLARE_METATYPE(KeyPair)

static inline KeyPair key(Qt::Key key, Qt::KeyboardModifier modifier = Qt::NoModifier) {
    return KeyPair(key, modifier);
}

/*
When a QDateEdit has its display format set to 'yyyy/MM/dd', its day
set to 31 and its month set to 2, it will display 291 as the day until
the cursor is moved or the focus changed. This is because
QDateTimeParser::parse calls sectionSize() for the day section, which
returns 1 when it should return 2.

This test verifies that QDateTimeEditPrivate has the correct text,
which is the text that is displayed until the cursor is moved or the
focus changed.
*/

void tst_QDateTimeEdit::dateEditCorrectSectionSize_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QDate>("defaultDate");
    QTest::addColumn<QString>("displayFormat");
    QTest::addColumn<KeyPairList>("keyPresses");
    QTest::addColumn<QString>("expectedDisplayString");

    const QDate defaultDate(2000, 1, 1);
    const QLocale defaultLocale(QLocale::English, QLocale::Australia);

    KeyPairList thirtyUpKeypresses;
    thirtyUpKeypresses.reserve(30);
    for (int i = 0; i < 30; ++i) {
        thirtyUpKeypresses << key(Qt::Key_Up);
    }

    // Make day the current section, set day to 31st (invalid for february),
    // move to month field, set month to february (2).
    KeyPairList threeDigitDayIssueKeypresses;
    threeDigitDayIssueKeypresses << key(Qt::Key_Tab) << key(Qt::Key_Tab)
        << key(Qt::Key_3) << key(Qt::Key_1) << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_2);

    // Same as above, except day-year-month format.
    KeyPairList threeDigitDayIssueKeypresses_DayYearMonth;
    threeDigitDayIssueKeypresses_DayYearMonth << key(Qt::Key_3) << key(Qt::Key_1) << key(Qt::Key_Tab)
        << key(Qt::Key_2);

    // Same as threeDigitDayIssueKeypresses, except doesn't require the day to be corrected.
    KeyPairList threeDigitDayIssueKeypresses_Nofixday;
    threeDigitDayIssueKeypresses_Nofixday << key(Qt::Key_Tab) << key(Qt::Key_Tab)
        << key(Qt::Key_2) << key(Qt::Key_8) << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_2);

    // Set day to 31st (invalid for february), set month to february (2).
    KeyPairList reverseThreeDigitDayIssueKeypresses;
    reverseThreeDigitDayIssueKeypresses
        << key(Qt::Key_3) << key(Qt::Key_1) << key(Qt::Key_2);

    // Make day the current section, set day to 31st, move to month field, set month to november (11).
    KeyPairList threeDigitDayIssueKeypresses_TwoDigitMonth;
    threeDigitDayIssueKeypresses_TwoDigitMonth << key(Qt::Key_Tab) << key(Qt::Key_Tab) << key(Qt::Key_3)
        << key(Qt::Key_1) << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_1) << key(Qt::Key_1);

    // Make day the current section, set day to 3rd, move to month field, set month to february (2).
    KeyPairList threeDigitDayIssueKeypresses_OneDigitDay;
    threeDigitDayIssueKeypresses_OneDigitDay << key(Qt::Key_Tab) << key(Qt::Key_Tab)
        << key(Qt::Key_3) << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_2);

    // Make day the current section, set day to 31st (invalid for february), move to month field,
    // set month to february (2).
    KeyPairList threeDigitDayIssueKeypresses_ShortMonthName;
    threeDigitDayIssueKeypresses_ShortMonthName << key(Qt::Key_Tab) << key(Qt::Key_Tab)
        << key(Qt::Key_3) << key(Qt::Key_1) << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_Up);

    // Make day the current section, set day to 31st (Monday), move to month field, set month to february (2).
    // Will probably never see this display format in a QDateTimeEdit, but it's good to test it anyway.
    KeyPairList threeDigitDayIssueKeypresses_DayName;
    threeDigitDayIssueKeypresses_DayName << key(Qt::Key_Tab) << key(Qt::Key_Tab) << thirtyUpKeypresses
        << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_2);

    KeyPairList threeDigitDayIssueKeypresses_DayName_DayYearMonth;
    threeDigitDayIssueKeypresses_DayName_DayYearMonth << thirtyUpKeypresses << key(Qt::Key_Tab)
        << key(Qt::Key_Tab) << key(Qt::Key_2);

    KeyPairList threeDigitDayIssueKeypresses_DayName_YearDayMonth;
    threeDigitDayIssueKeypresses_DayName_YearDayMonth << key(Qt::Key_Tab) << thirtyUpKeypresses
        << key(Qt::Key_Tab) << key(Qt::Key_2);

    KeyPairList threeDigitDayIssueKeypresses_DayName_DayMonthYear;
    threeDigitDayIssueKeypresses_DayName_DayMonthYear << thirtyUpKeypresses << key(Qt::Key_Tab)
        << key(Qt::Key_2);

    KeyPairList threeDigitDayIssueKeypresses_DayName_MonthDayYear;
    threeDigitDayIssueKeypresses_DayName_MonthDayYear << key(Qt::Key_Tab) << thirtyUpKeypresses
        << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_2);

    // Make day the current section, set day to 31st (invalid for february), move to month field,
    // set month to february (2).
    KeyPairList threeDigitDayIssueKeypresses_YearDayMonth;
    threeDigitDayIssueKeypresses_YearDayMonth << key(Qt::Key_Tab) << key(Qt::Key_3) << key(Qt::Key_1)
        << key(Qt::Key_Tab) << key(Qt::Key_2);

    // Make day the current section, set day to 31st, move to month field, set month to february (2).
    KeyPairList threeDigitDayIssueKeypresses_MonthDayYear;
    threeDigitDayIssueKeypresses_MonthDayYear << key(Qt::Key_Tab) << key(Qt::Key_3) << key(Qt::Key_1)
        << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_2);

    // Same as above, except month-year-day format.
    KeyPairList threeDigitDayIssueKeypresses_MonthYearDay;
    threeDigitDayIssueKeypresses_MonthYearDay << key(Qt::Key_Tab) << key(Qt::Key_Tab) << key(Qt::Key_3)
        << key(Qt::Key_1) << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_Tab, Qt::ShiftModifier)
        << key(Qt::Key_2);

    // Same as above, except short month name.
    KeyPairList threeDigitDayIssueKeypresses_ShortMonthName_MonthYearDay;
    threeDigitDayIssueKeypresses_ShortMonthName_MonthYearDay << key(Qt::Key_Tab) << key(Qt::Key_Tab)
        << key(Qt::Key_3) << key(Qt::Key_1) << key(Qt::Key_Tab, Qt::ShiftModifier)
        << key(Qt::Key_Tab, Qt::ShiftModifier) << key(Qt::Key_Up);

    KeyPairList shortAndLongNameIssueKeypresses;
    shortAndLongNameIssueKeypresses << key(Qt::Key_Tab) << key(Qt::Key_3) << key(Qt::Key_1) << key(Qt::Key_Up);

    QTest::newRow("no fixday, leap, yy/M/dddd") << defaultLocale << defaultDate << QString::fromLatin1("yy/M/dddd")
        << threeDigitDayIssueKeypresses_DayName << QString::fromLatin1("00/2/Tuesday");

    QTest::newRow("no fixday, leap, yy/M/ddd") << defaultLocale << defaultDate << QString::fromLatin1("yy/M/ddd")
        << threeDigitDayIssueKeypresses_DayName << QString::fromLatin1("00/2/Tue.");

    QTest::newRow("no fixday, leap, yy/MM/dddd") << defaultLocale << defaultDate << QString::fromLatin1("yy/MM/dddd")
        << threeDigitDayIssueKeypresses_DayName << QString::fromLatin1("00/02/Tuesday");

    QTest::newRow("fixday, leap, yy/MM/dd") << defaultLocale << defaultDate << QString::fromLatin1("yy/MM/dd")
        << threeDigitDayIssueKeypresses << QString::fromLatin1("00/02/29");

    QTest::newRow("fixday, leap, yy/MM/d") << defaultLocale << defaultDate << QString::fromLatin1("yy/MM/d")
        << threeDigitDayIssueKeypresses << QString::fromLatin1("00/02/29");

    QTest::newRow("fixday, leap, yyyy/M/d") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/M/d")
        << threeDigitDayIssueKeypresses << QString::fromLatin1("2000/2/29");

    QTest::newRow("no fixday, yyyy/M/d") << defaultLocale << defaultDate.addYears(1) << QString::fromLatin1("yyyy/M/d")
        << threeDigitDayIssueKeypresses_Nofixday << QString::fromLatin1("2001/2/28");

    QTest::newRow("fixday, leap, 2-digit month, yyyy/M/dd") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/M/dd")
        << threeDigitDayIssueKeypresses_TwoDigitMonth << QString::fromLatin1("2000/11/30");

    QTest::newRow("no fixday, leap, 1-digit day, yyyy/M/dd") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/M/dd")
        << threeDigitDayIssueKeypresses_OneDigitDay << QString::fromLatin1("2000/2/03");

    QTest::newRow("fixday, leap, yyyy/MM/dd") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/MM/dd")
        << threeDigitDayIssueKeypresses << QString::fromLatin1("2000/02/29");

    QTest::newRow("no fixday, yyyy/MM/dd") << defaultLocale << defaultDate.addYears(1) << QString::fromLatin1("yyyy/MM/dd")
        << threeDigitDayIssueKeypresses_Nofixday << QString::fromLatin1("2001/02/28");

    QTest::newRow("fixday, leap, 2-digit month, yyyy/MM/dd") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/MM/dd")
        << threeDigitDayIssueKeypresses_TwoDigitMonth << QString::fromLatin1("2000/11/30");

    QTest::newRow("no fixday, leap, yyyy/M/dddd") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/M/dddd")
        << threeDigitDayIssueKeypresses_DayName << QString::fromLatin1("2000/2/Tuesday");

    QTest::newRow("no fixday, leap, yyyy/MM/dddd") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/MM/dddd")
        << threeDigitDayIssueKeypresses_DayName << QString::fromLatin1("2000/02/Tuesday");

    QTest::newRow("fixday, leap, yyyy/dd/MM") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/dd/MM")
        << threeDigitDayIssueKeypresses_YearDayMonth << QString::fromLatin1("2000/29/02");

    QTest::newRow("fixday, leap, yyyy/dd/M") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/dd/M")
        << threeDigitDayIssueKeypresses_YearDayMonth << QString::fromLatin1("2000/29/2");

    QTest::newRow("fixday, leap, yyyy/d/M") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/d/M")
        << threeDigitDayIssueKeypresses_YearDayMonth << QString::fromLatin1("2000/29/2");

    QTest::newRow("fixday, leap, yyyy/MMM/dd") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/MMM/dd")
        << threeDigitDayIssueKeypresses_ShortMonthName << QString::fromLatin1("2000/Feb./29");

    QTest::newRow("fixday, leap, yyyy/MMM/d") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/MMM/d")
        << threeDigitDayIssueKeypresses_ShortMonthName << QString::fromLatin1("2000/Feb./29");

    QTest::newRow("fixday, leap, yy/MMM/dd") << defaultLocale << defaultDate << QString::fromLatin1("yy/MMM/dd")
        << threeDigitDayIssueKeypresses_ShortMonthName << QString::fromLatin1("00/Feb./29");

    QTest::newRow("fixday, leap, yyyy/dddd/M") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/dddd/M")
        << threeDigitDayIssueKeypresses_DayName_YearDayMonth << QString::fromLatin1("2000/Tuesday/2");

    QTest::newRow("fixday, leap, yyyy/dddd/MM") << defaultLocale << defaultDate << QString::fromLatin1("yyyy/dddd/MM")
        << threeDigitDayIssueKeypresses_DayName_YearDayMonth << QString::fromLatin1("2000/Tuesday/02");

    QTest::newRow("fixday, leap, d/M/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("d/M/yyyy")
        << reverseThreeDigitDayIssueKeypresses << QString::fromLatin1("29/2/2000");

    QTest::newRow("fixday, leap, dd/MM/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("dd/MM/yyyy")
        << reverseThreeDigitDayIssueKeypresses << QString::fromLatin1("29/02/2000");

    QTest::newRow("fixday, dd/MM/yyyy") << defaultLocale << defaultDate.addYears(1) << QString::fromLatin1("dd/MM/yyyy")
        << reverseThreeDigitDayIssueKeypresses << QString::fromLatin1("28/02/2001");

    QTest::newRow("fixday, leap, dddd/MM/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("dddd/MM/yyyy")
        << threeDigitDayIssueKeypresses_DayName_DayMonthYear << QString::fromLatin1("Tuesday/02/2000");

    QTest::newRow("fixday, leap, d/yy/M") << defaultLocale << defaultDate << QString::fromLatin1("d/yy/M")
        << threeDigitDayIssueKeypresses_DayYearMonth << QString::fromLatin1("29/00/2");

    QTest::newRow("fixday, leap, d/yyyy/M") << defaultLocale << defaultDate << QString::fromLatin1("d/yyyy/M")
        << threeDigitDayIssueKeypresses_DayYearMonth << QString::fromLatin1("29/2000/2");

    QTest::newRow("fixday, leap, d/yyyy/MM") << defaultLocale << defaultDate << QString::fromLatin1("d/yyyy/MM")
        << threeDigitDayIssueKeypresses_DayYearMonth << QString::fromLatin1("29/2000/02");

    QTest::newRow("fixday, leap, dd/yy/MM") << defaultLocale << defaultDate << QString::fromLatin1("dd/yy/MM")
        << threeDigitDayIssueKeypresses_DayYearMonth << QString::fromLatin1("29/00/02");

    QTest::newRow("fixday, leap, dd/yyyy/M") << defaultLocale << defaultDate << QString::fromLatin1("dd/yyyy/M")
        << threeDigitDayIssueKeypresses_DayYearMonth << QString::fromLatin1("29/2000/2");

    QTest::newRow("fixday, leap, dd/yyyy/MM") << defaultLocale << defaultDate << QString::fromLatin1("dd/yyyy/MM")
        << threeDigitDayIssueKeypresses_DayYearMonth << QString::fromLatin1("29/2000/02");

    QTest::newRow("fixday, leap, dddd/yy/M") << defaultLocale << defaultDate << QString::fromLatin1("dddd/yy/M")
        << threeDigitDayIssueKeypresses_DayName_DayYearMonth << QString::fromLatin1("Tuesday/00/2");

    QTest::newRow("fixday, leap, dddd/yy/MM") << defaultLocale << defaultDate << QString::fromLatin1("dddd/yy/MM")
        << threeDigitDayIssueKeypresses_DayName_DayYearMonth << QString::fromLatin1("Tuesday/00/02");

    QTest::newRow("fixday, leap, M/d/yy") << defaultLocale << defaultDate << QString::fromLatin1("M/d/yy")
        << threeDigitDayIssueKeypresses_MonthDayYear << QString::fromLatin1("2/29/00");

    QTest::newRow("fixday, leap, M/d/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("M/d/yyyy")
        << threeDigitDayIssueKeypresses_MonthDayYear << QString::fromLatin1("2/29/2000");

    QTest::newRow("fixday, leap, M/dd/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("M/dd/yyyy")
        << threeDigitDayIssueKeypresses_MonthDayYear << QString::fromLatin1("2/29/2000");

    QTest::newRow("fixday, leap, M/dddd/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("M/dddd/yyyy")
        << threeDigitDayIssueKeypresses_DayName_MonthDayYear << QString::fromLatin1("2/Tuesday/2000");

    QTest::newRow("fixday, leap, MM/dd/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("MM/dd/yyyy")
        << threeDigitDayIssueKeypresses_MonthDayYear << QString::fromLatin1("02/29/2000");

    QTest::newRow("fixday, leap, MM/dddd/yyyy") << defaultLocale << defaultDate << QString::fromLatin1("MM/dddd/yyyy")
        << threeDigitDayIssueKeypresses_DayName_MonthDayYear << QString::fromLatin1("02/Tuesday/2000");

    QTest::newRow("fixday, leap, M/yyyy/dd") << defaultLocale << defaultDate << QString::fromLatin1("M/yyyy/dd")
        << threeDigitDayIssueKeypresses_MonthYearDay << QString::fromLatin1("2/2000/29");

    QTest::newRow("fixday, leap, M/yy/dd") << defaultLocale << defaultDate << QString::fromLatin1("M/yy/dd")
        << threeDigitDayIssueKeypresses_MonthYearDay << QString::fromLatin1("2/00/29");

    QTest::newRow("fixday, leap, M/yy/d") << defaultLocale << defaultDate << QString::fromLatin1("M/yy/d")
        << threeDigitDayIssueKeypresses_MonthYearDay << QString::fromLatin1("2/00/29");

    QTest::newRow("fixday, leap, MM/yyyy/dd") << defaultLocale << defaultDate << QString::fromLatin1("MM/yyyy/dd")
        << threeDigitDayIssueKeypresses_MonthYearDay << QString::fromLatin1("02/2000/29");

    QTest::newRow("fixday, leap, MMM/yy/d") << defaultLocale << defaultDate << QString::fromLatin1("MMM/yy/d")
        << threeDigitDayIssueKeypresses_ShortMonthName_MonthYearDay << QString::fromLatin1("Feb./00/29");

    QTest::newRow("fixday, leap, MMM/yyyy/d") << defaultLocale << defaultDate << QString::fromLatin1("MMM/yyyy/d")
        << threeDigitDayIssueKeypresses_ShortMonthName_MonthYearDay << QString::fromLatin1("Feb./2000/29");

    QTest::newRow("fixday, MMM/yyyy/d") << defaultLocale << defaultDate.addYears(1) << QString::fromLatin1("MMM/yyyy/d")
        << threeDigitDayIssueKeypresses_ShortMonthName_MonthYearDay << QString::fromLatin1("Feb./2001/28");

    QTest::newRow("fixday, leap, MMM/yyyy/dd") << defaultLocale << defaultDate << QString::fromLatin1("MMM/yyyy/dd")
        << threeDigitDayIssueKeypresses_ShortMonthName_MonthYearDay << QString::fromLatin1("Feb./2000/29");

    QTest::newRow("fixday, leap, dddd, dd. MMMM yyyy") << defaultLocale
        << defaultDate << QString::fromLatin1("dddd, dd. MMMM yyyy")
        << shortAndLongNameIssueKeypresses << QString::fromLatin1("Tuesday, 29. February 2000");

    QTest::newRow("fixday, leap, german, dddd, dd. MMMM yyyy") << QLocale(QLocale::German, QLocale::Germany)
        << defaultDate << QString::fromLatin1("dddd, dd. MMMM yyyy")
        << shortAndLongNameIssueKeypresses << QString::fromLatin1("Dienstag, 29. Februar 2000");
}

void tst_QDateTimeEdit::dateEditCorrectSectionSize()
{
    QFETCH(QLocale, locale);
    QFETCH(QDate, defaultDate);
    QFETCH(QString, displayFormat);
    QFETCH(KeyPairList, keyPresses);
    QFETCH(QString, expectedDisplayString);

    QDateEdit edit;
    edit.setLocale(locale);
    edit.setDate(defaultDate);
    edit.setDisplayFormat(displayFormat);
    edit.show();
    edit.setFocus();
    // For some reason, we need to set the selected section for the dd/MM/yyyy tests,
    // otherwise the 3 is inserted at the front of 01/01/2000 (301/01/2000), instead of the
    // selected text being replaced. This is not an issue for the yyyy/MM/dd format though...
    edit.setSelectedSection(edit.sectionAt(0));

    foreach (const KeyPair &keyPair, keyPresses)
        QTest::keyClick(&edit, keyPair.first, keyPair.second);

    QDateTimeEditPrivate* edit_d_ptr(static_cast<QDateTimeEditPrivate*>(qt_widget_private(&edit)));
    QCOMPARE(edit_d_ptr->QDateTimeParser::displayText(), expectedDisplayString);
}
#endif

QTEST_MAIN(tst_QDateTimeEdit)
#include "tst_qdatetimeedit.moc"
