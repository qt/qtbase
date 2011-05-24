/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qabstractitemview.h>
#include <qstandarditemmodel.h>
#include <qapplication.h>
#include <qdatetimeedit.h>
#include <qspinbox.h>
#include <qlistview.h>
#include <qtableview.h>
#include <qtreeview.h>
#include <qheaderview.h>
#include <qitemeditorfactory.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qtablewidget.h>
#include <qtreewidget.h>

#include <QItemDelegate>
#include <QAbstractItemDelegate>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDialog>

#include "../../shared/util.h"

Q_DECLARE_METATYPE(QAbstractItemDelegate::EndEditHint)

//TESTED_CLASS=
//TESTED_FILES=

#if defined (Q_OS_WIN) && !defined(Q_OS_WINCE)
#include <windows.h>
#define Q_CHECK_PAINTEVENTS \
    if (::SwitchDesktop(::GetThreadDesktop(::GetCurrentThreadId())) == 0) \
        QSKIP("The widgets don't get the paint events", SkipSingle);
#else
#define Q_CHECK_PAINTEVENTS
#endif

//Begin of class definitions

class TestItemDelegate : public QItemDelegate
{
public:
    TestItemDelegate(QObject *parent = 0) : QItemDelegate(parent) {}
    ~TestItemDelegate() {}

    void drawDisplay(QPainter *painter,
                     const QStyleOptionViewItem &option,
                     const QRect &rect, const QString &text) const
    {
        displayText = text;
        displayFont = option.font;
        QItemDelegate::drawDisplay(painter, option, rect, text);
    }

    void drawDecoration(QPainter *painter,
                        const QStyleOptionViewItem &option,
                        const QRect &rect, const QPixmap &pixmap) const
    {
        decorationPixmap = pixmap;
        decorationRect = rect;
        QItemDelegate::drawDecoration(painter, option, rect, pixmap);
    }


    inline QRect textRectangle(QPainter * painter, const QRect &rect,
                               const QFont &font, const QString &text) const
    {
        return QItemDelegate::textRectangle(painter, rect, font, text);
    }

    inline void doLayout(const QStyleOptionViewItem &option,
                         QRect *checkRect, QRect *pixmapRect,
                         QRect *textRect, bool hint) const
    {
        QItemDelegate::doLayout(option, checkRect, pixmapRect, textRect, hint);
    }

    inline QRect rect(const QStyleOptionViewItem &option,
                      const QModelIndex &index, int role) const
    {
        return QItemDelegate::rect(option, index, role);
    }

    inline bool eventFilter(QObject *object, QEvent *event)
    {
        return QItemDelegate::eventFilter(object, event);
    }

    inline bool editorEvent(QEvent *event,
                            QAbstractItemModel *model,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index)
    {
        return QItemDelegate::editorEvent(event, model, option, index);
    }

    // stored values for testing
    mutable QString displayText;
    mutable QFont displayFont;
    mutable QPixmap decorationPixmap;
    mutable QRect decorationRect;
};

class TestItemModel : public QAbstractTableModel
{
public:

    enum Roles {
        PixmapTestRole,
        ImageTestRole,
        IconTestRole,
        ColorTestRole,
        DoubleTestRole
    };

    TestItemModel(const QSize &size) : size(size) {}

    ~TestItemModel() {}

    int rowCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return 1;
    }

    int columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant data(const QModelIndex& index, int role) const
    {
        Q_UNUSED(index);
        static QPixmap pixmap(size);
        static QImage image(size, QImage::Format_Mono);
        static QIcon icon(pixmap);
        static QColor color(Qt::green);

        switch (role) {
        case PixmapTestRole: return pixmap;
        case ImageTestRole:  return image;
        case IconTestRole:   return icon;
        case ColorTestRole:  return color;
        case DoubleTestRole:  return 10.00000001;
        default: break;
        }

        return QVariant();
    }

private:
    QSize size;
};

class tst_QItemDelegate : public QObject
{
    Q_OBJECT

public:
    tst_QItemDelegate();
    virtual ~tst_QItemDelegate();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void getSetCheck();
    void textRectangle_data();
    void textRectangle();
    void sizeHint_data();
    void sizeHint();
    void editorKeyPress_data();
    void editorKeyPress();
    void doubleEditorNegativeInput();
    void font_data();
    void font();
    void doLayout_data();
    void doLayout();
    void rect_data();
    void rect();
    void eventFilter();
    void dateTimeEditor_data();
    void dateTimeEditor();
    void decoration_data();
    void decoration();
    void editorEvent_data();
    void editorEvent();
    void enterKey_data();
    void enterKey();

    void task257859_finalizeEdit();
    void QTBUG4435_keepSelectionOnCheck();
};


//End of class definitions

// Testing get/set functions
void tst_QItemDelegate::getSetCheck()
{
    QItemDelegate obj1;

    // QItemEditorFactory * QItemDelegate::itemEditorFactory()
    // void QItemDelegate::setItemEditorFactory(QItemEditorFactory *)
    QItemEditorFactory *var1 = new QItemEditorFactory;
    obj1.setItemEditorFactory(var1);
    QCOMPARE(var1, obj1.itemEditorFactory());
    obj1.setItemEditorFactory((QItemEditorFactory *)0);
    QCOMPARE((QItemEditorFactory *)0, obj1.itemEditorFactory());
    delete var1;

    QCOMPARE(obj1.hasClipping(), true);
    obj1.setClipping(false);
    QCOMPARE(obj1.hasClipping(), false);
    obj1.setClipping(true);
    QCOMPARE(obj1.hasClipping(), true);
}

tst_QItemDelegate::tst_QItemDelegate()
{
}

tst_QItemDelegate::~tst_QItemDelegate()
{
}

void tst_QItemDelegate::initTestCase()
{
}

void tst_QItemDelegate::cleanupTestCase()
{
}

void tst_QItemDelegate::init()
{
}

void tst_QItemDelegate::cleanup()
{
}

void tst_QItemDelegate::textRectangle_data()
{
    QFont font;
    QFontMetrics fontMetrics(font);
    int pm = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
    int margins = 2 * (pm + 1); // margin on each side of the text
    int height = fontMetrics.height();

    QTest::addColumn<QString>("text");
    QTest::addColumn<QRect>("rect");
    QTest::addColumn<QRect>("expected");

    QTest::newRow("empty") << QString()
                           << QRect()
                           << QRect(0, 0, margins, height);
}

void tst_QItemDelegate::textRectangle()
{
    QFETCH(QString, text);
    QFETCH(QRect, rect);
    QFETCH(QRect, expected);

    QFont font;
    TestItemDelegate delegate;
    QRect result = delegate.textRectangle(0, rect, font, text);

    QCOMPARE(result, expected);
}

void tst_QItemDelegate::sizeHint_data()
{
    QTest::addColumn<QSize>("expected");

    QFont font;
    QFontMetrics fontMetrics(font);
    //int m = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    QTest::newRow("empty")
        << QSize(0, fontMetrics.height());

}

void tst_QItemDelegate::sizeHint()
{
    QFETCH(QSize, expected);

    QModelIndex index;
    QStyleOptionViewItem option;

    TestItemDelegate delegate;
    QSize result = delegate.sizeHint(option, index);
    QCOMPARE(result, expected);
}

void tst_QItemDelegate::editorKeyPress_data()
{
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("expected");

    QTest::newRow("foo bar")
        << QString("foo")
        << QString("bar");
}

void tst_QItemDelegate::editorKeyPress()
{
    QFETCH(QString, initial);
    QFETCH(QString, expected);

    QStandardItemModel model;
    model.appendRow(new QStandardItem(initial));

    QListView view;
    view.setModel(&model);
    view.show();

    QModelIndex index = model.index(0, 0);
    view.setCurrentIndex(index); // the editor will only selectAll on the current index
    view.edit(index);

    QList<QLineEdit*> lineEditors = qFindChildren<QLineEdit *>(view.viewport());
    QCOMPARE(lineEditors.count(), 1);

    QLineEdit *editor = lineEditors.at(0);
    QCOMPARE(editor->selectedText(), initial);

    QTest::keyClicks(editor, expected);
    QTest::keyClick(editor, Qt::Key_Enter);
    QApplication::processEvents();

    QCOMPARE(index.data().toString(), expected);
}

void tst_QItemDelegate::doubleEditorNegativeInput()
{
    QStandardItemModel model;

    QStandardItem *item = new QStandardItem;
    item->setData(10.0, Qt::DisplayRole);
    model.appendRow(item);

    QListView view;
    view.setModel(&model);
    view.show();

    QModelIndex index = model.index(0, 0);
    view.setCurrentIndex(index); // the editor will only selectAll on the current index
    view.edit(index);

    QList<QDoubleSpinBox*> editors = qFindChildren<QDoubleSpinBox *>(view.viewport());
    QCOMPARE(editors.count(), 1);

    QDoubleSpinBox *editor = editors.at(0);
    QCOMPARE(editor->value(), double(10));

    QTest::keyClick(editor, Qt::Key_Minus);
    QTest::keyClick(editor, Qt::Key_1);
    QTest::keyClick(editor, Qt::Key_0);
    QTest::keyClick(editor, Qt::Key_Comma); //support both , and . locales
    QTest::keyClick(editor, Qt::Key_Period);
    QTest::keyClick(editor, Qt::Key_0);
    QTest::keyClick(editor, Qt::Key_Enter);
    QApplication::processEvents();

    QCOMPARE(index.data().toString(), QString("-10"));
}

void tst_QItemDelegate::font_data()
{
    QTest::addColumn<QString>("itemText");
    QTest::addColumn<QString>("properties");
    QTest::addColumn<QFont>("itemFont");
    QTest::addColumn<QFont>("viewFont");

    QFont itemFont;
    itemFont.setItalic(true);
    QFont viewFont;

    QTest::newRow("foo italic")
        << QString("foo")
        << QString("italic")
        << itemFont
        << viewFont;

    itemFont.setItalic(true);

    QTest::newRow("foo bold")
        << QString("foo")
        << QString("bold")
        << itemFont
        << viewFont;

    itemFont.setFamily(itemFont.defaultFamily());

    QTest::newRow("foo family")
        << QString("foo")
        << QString("family")
        << itemFont
        << viewFont;
 }

void tst_QItemDelegate::font()
{
    Q_CHECK_PAINTEVENTS

    QFETCH(QString, itemText);
    QFETCH(QString, properties);
    QFETCH(QFont, itemFont);
    QFETCH(QFont, viewFont);

    QTableWidget table(1, 1);
    table.setFont(viewFont);

    TestItemDelegate *delegate = new TestItemDelegate(&table);
    table.setItemDelegate(delegate);
    table.show();
#ifdef Q_WS_X11
    qt_x11_wait_for_window_manager(&table);
#endif

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setText(itemText);
    item->setFont(itemFont);
    table.setItem(0, 0, item);

    QApplication::processEvents();
#ifdef Q_WS_QWS
    QApplication::sendPostedEvents(); //glib workaround
#endif

    QTRY_COMPARE(delegate->displayText, item->text());
    if (properties.contains("italic")) {
        QCOMPARE(delegate->displayFont.italic(), item->font().italic());
    }
    if (properties.contains("bold")){
        QCOMPARE(delegate->displayFont.bold(), item->font().bold());
    }
    if (properties.contains("family")){
        QCOMPARE(delegate->displayFont.family(), item->font().family());
    }
}

//Testing the different QRect created by the doLayout function.
//Tests are made with different values for the QStyleOptionViewItem properties:
//decorationPosition and position.

void tst_QItemDelegate::doLayout_data()
{
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("direction");
    QTest::addColumn<bool>("hint");
    QTest::addColumn<QRect>("itemRect");
    QTest::addColumn<QRect>("checkRect");
    QTest::addColumn<QRect>("pixmapRect");
    QTest::addColumn<QRect>("textRect");
    QTest::addColumn<QRect>("expectedCheckRect");
    QTest::addColumn<QRect>("expectedPixmapRect");
    QTest::addColumn<QRect>("expectedTextRect");

    int m = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    //int item = 400;
    //int check = 50;
    //int pixmap = 1000;
    //int text = 400;

    QTest::newRow("top, left to right, hint")
        << (int)QStyleOptionViewItem::Top
        << (int)Qt::LeftToRight
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(m, 0, 50 + 2*m, 1000)
        << QRect(50 + 2*m, 0, 1000 + 2*m, 1000 + m)
        << QRect(50 + 2*m, 1000 + m, 1000 + 2*m, 400);
    /*
    QTest::newRow("top, left to right, limited")
        << (int)QStyleOptionViewItem::Top
        << (int)Qt::LeftToRight
        << false
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(m, (400/2) - (50/2), 50, 50)
        << QRect(50 + 2*m, 0, 1000, 1000)
        << QRect(50 + 2*m, 1000 + m, 400 - (50 + 2*m), 400 - 1000 - m);
    */
    QTest::newRow("top, right to left, hint")
        << (int)QStyleOptionViewItem::Top
        << (int)Qt::RightToLeft
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(1000 + 2 * m, 0, 50 + 2 * m, 1000)
        << QRect(0, 0, 1000 + 2 * m, 1000 + m)
        << QRect(0, 1000 + m, 1000 + 2 * m, 400);

    QTest::newRow("bottom, left to right, hint")
        << (int)QStyleOptionViewItem::Bottom
        << (int)Qt::LeftToRight
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(m, 0, 50 + 2 * m, 1000)
        << QRect(50 + 2 * m, 400 + m, 1000 + 2 * m, 1000)
        << QRect(50 + 2 * m, 0, 1000 + 2 * m, 400 + m);

    QTest::newRow("bottom, right to left, hint")
        << (int)QStyleOptionViewItem::Bottom
        << (int)Qt::RightToLeft
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(1000 + 2 * m, 0, 50 + 2 * m, 1000)
        << QRect(0, 400 + m, 1000 + 2 * m, 1000)
        << QRect(0, 0, 1000 + 2 * m, 400 + m);

    QTest::newRow("left, left to right, hint")
        << (int)QStyleOptionViewItem::Left
        << (int)Qt::LeftToRight
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(m, 0, 50 + 2 * m, 1000)
        << QRect(50 + 2 * m, 0, 1000 + 2 * m, 1000)
        << QRect(1050 + 4 * m, 0, 400 + 2 * m, 1000);

    QTest::newRow("left, right to left, hint")
        << (int)QStyleOptionViewItem::Left
        << (int)Qt::RightToLeft
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(1400 + 4 * m, 0, 50 + 2 * m, 1000)
        << QRect(400 + 2 * m, 0, 1000 + 2 * m, 1000)
        << QRect(0, 0, 400 + 2 * m, 1000);

    QTest::newRow("right, left to right, hint")
        << (int)QStyleOptionViewItem::Right
        << (int)Qt::LeftToRight
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(m, 0, 50 + 2 * m, 1000)
        << QRect(450 + 4 * m, 0, 1000 + 2 * m, 1000)
        << QRect(50 + 2 * m, 0, 400 + 2 * m, 1000);

    QTest::newRow("right, right to left, hint")
        << (int)QStyleOptionViewItem::Right
        << (int)Qt::RightToLeft
        << true
        << QRect(0, 0, 400, 400)
        << QRect(0, 0, 50, 50)
        << QRect(0, 0, 1000, 1000)
        << QRect(0, 0, 400, 400)
        << QRect(1400 + 4 * m, 0, 50 + 2 * m, 1000)
        << QRect(0, 0, 1000 + 2 * m, 1000)
        << QRect(1000 + 2 * m, 0, 400 + 2 * m, 1000);
}

void tst_QItemDelegate::doLayout()
{
    QFETCH(int, position);
    QFETCH(int, direction);
    QFETCH(bool, hint);
    QFETCH(QRect, itemRect);
    QFETCH(QRect, checkRect);
    QFETCH(QRect, pixmapRect);
    QFETCH(QRect, textRect);
    QFETCH(QRect, expectedCheckRect);
    QFETCH(QRect, expectedPixmapRect);
    QFETCH(QRect, expectedTextRect);

    TestItemDelegate delegate;
    QStyleOptionViewItem option;

    option.rect = itemRect;
    option.decorationPosition = (QStyleOptionViewItem::Position)position;
    option.direction = (Qt::LayoutDirection)direction;

    delegate.doLayout(option, &checkRect, &pixmapRect, &textRect, hint);

    QCOMPARE(checkRect, expectedCheckRect);
    QCOMPARE(pixmapRect, expectedPixmapRect);
    QCOMPARE(textRect, expectedTextRect);
}

void tst_QItemDelegate::rect_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QRect>("expected");

    QTest::newRow("pixmap")
        << (int)TestItemModel::PixmapTestRole
        << QSize(200, 300)
        << QRect(0, 0, 200, 300);

    QTest::newRow("image")
        << (int)TestItemModel::ImageTestRole
        << QSize(200, 300)
        << QRect(0, 0, 200, 300);

    QTest::newRow("icon")
        << (int)TestItemModel::IconTestRole
        << QSize(200, 300)
        << QRect(0, 0, 200, 300);

    QTest::newRow("color")
        << (int)TestItemModel::ColorTestRole
        << QSize(200, 300)
        << QRect(0, 0, 200, 300);

    QTest::newRow("double")
            << (int)TestItemModel::DoubleTestRole
            << QSize()
            << QRect();
}

void tst_QItemDelegate::rect()
{
    QFETCH(int, role);
    QFETCH(QSize, size);
    QFETCH(QRect, expected);

    TestItemModel model(size);
    QStyleOptionViewItem option;
    TestItemDelegate delegate;
    option.decorationSize = size;

    if (role == TestItemModel::DoubleTestRole)
        expected = delegate.textRectangle(0, QRect(), QFont(), QLatin1String("10.00000001"));

    QModelIndex index = model.index(0, 0);
    QVERIFY(index.isValid());
    QRect result = delegate.rect(option, index, role);
    QCOMPARE(result, expected);
}

//TODO : Add a test for the keyPress event
//with Qt::Key_Enter and Qt::Key_Return
void tst_QItemDelegate::eventFilter()
{
    TestItemDelegate delegate;
    QWidget widget;
    QEvent *event;

    qRegisterMetaType<QAbstractItemDelegate::EndEditHint>("QAbstractItemDelegate::EndEditHint");

    QSignalSpy commitDataSpy(&delegate, SIGNAL(commitData(QWidget *)));
    QSignalSpy closeEditorSpy(&delegate,
                              SIGNAL(closeEditor(QWidget *,
                                                 QAbstractItemDelegate::EndEditHint)));

    //Subtest KeyPress
    //For each test we send a key event and check if signals were emitted.
    event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    QVERIFY(delegate.eventFilter(&widget, event));
    QCOMPARE(closeEditorSpy.count(), 1);
    QCOMPARE(commitDataSpy.count(), 1);
    delete event;

    event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier);
    QVERIFY(delegate.eventFilter(&widget, event));
    QCOMPARE(closeEditorSpy.count(), 2);
    QCOMPARE(commitDataSpy.count(), 2);
    delete event;

    event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QVERIFY(delegate.eventFilter(&widget, event));
    QCOMPARE(closeEditorSpy.count(), 3);
    QCOMPARE(commitDataSpy.count(), 2);
    delete event;

    event = new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QVERIFY(!delegate.eventFilter(&widget, event));
    QCOMPARE(closeEditorSpy.count(), 3);
    QCOMPARE(commitDataSpy.count(), 2);
    delete event;

    //Subtest focusEvent
    event = new QFocusEvent(QEvent::FocusOut);
    QVERIFY(!delegate.eventFilter(&widget, event));
    QCOMPARE(closeEditorSpy.count(), 4);
    QCOMPARE(commitDataSpy.count(), 3);
    delete event;
}

void tst_QItemDelegate::dateTimeEditor_data()
{
    QTest::addColumn<QTime>("time");
    QTest::addColumn<QDate>("date");

    QTest::newRow("data")
        << QTime(7, 16, 34)
        << QDate(2006, 10, 31);
}

void tst_QItemDelegate::dateTimeEditor()
{
    QFETCH(QTime, time);
    QFETCH(QDate, date);

    QTableWidgetItem *item1 = new QTableWidgetItem;
    item1->setData(Qt::DisplayRole, time);

    QTableWidgetItem *item2 = new QTableWidgetItem;
    item2->setData(Qt::DisplayRole, date);

    QTableWidgetItem *item3 = new QTableWidgetItem;
    item3->setData(Qt::DisplayRole, QDateTime(date, time));

    QTableWidget widget(1, 3);
    widget.setItem(0, 0, item1);
    widget.setItem(0, 1, item2);
    widget.setItem(0, 2, item3);
    widget.show();

    widget.editItem(item1);

    QTestEventLoop::instance().enterLoop(1);

    QTimeEdit *timeEditor = qFindChild<QTimeEdit *>(widget.viewport());
    QVERIFY(timeEditor);
    QCOMPARE(timeEditor->time(), time);

    widget.clearFocus();
    qApp->setActiveWindow(&widget);
    widget.setFocus();
    widget.editItem(item2);

    QTestEventLoop::instance().enterLoop(1);

    QDateEdit *dateEditor = qFindChild<QDateEdit *>(widget.viewport());
    QVERIFY(dateEditor);
    QCOMPARE(dateEditor->date(), date);

    widget.clearFocus();
    widget.setFocus();
    widget.editItem(item3);

    QTestEventLoop::instance().enterLoop(1);

    QList<QDateTimeEdit *> dateTimeEditors = widget.findChildren<QDateTimeEdit *>();
    QDateTimeEdit *dateTimeEditor = 0;
    foreach(dateTimeEditor, dateTimeEditors)
        if (dateTimeEditor->metaObject()->className() == QLatin1String("QDateTimeEdit"))
            break;
    QVERIFY(dateTimeEditor);
    QCOMPARE(dateTimeEditor->date(), date);
    QCOMPARE(dateTimeEditor->time(), time);
}

void tst_QItemDelegate::decoration_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QSize>("expected");

    int pm = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

    QTest::newRow("pixmap 30x30")
        << (int)QVariant::Pixmap
        << QSize(30, 30)
        << QSize(30, 30);

    QTest::newRow("image 30x30")
        << (int)QVariant::Image
        << QSize(30, 30)
        << QSize(30, 30);

//The default engine scales pixmaps down if required, but never up. For WinCE we need bigger IconSize than 30
    QTest::newRow("icon 30x30")
        << (int)QVariant::Icon
        << QSize(60, 60)
        << QSize(pm, pm);

    QTest::newRow("color 30x30")
        << (int)QVariant::Color
        << QSize(30, 30)
        << QSize(pm, pm);

    QTest::newRow("pixmap 30x30 big")
        << (int)QVariant::Pixmap
        << QSize(1024, 1024)        // Over 1M
        << QSize(1024, 1024);
}

void tst_QItemDelegate::decoration()
{
    if (QByteArray(QTest::currentDataTag()) == QByteArray("pixmap 30x30 big"))
        QSKIP("Skipping this as it demands too much memory and potential hangs", SkipSingle);
    Q_CHECK_PAINTEVENTS

    QFETCH(int, type);
    QFETCH(QSize, size);
    QFETCH(QSize, expected);

    QTableWidget table(1, 1);
    TestItemDelegate delegate;
    table.setItemDelegate(&delegate);
    table.show();
#ifdef Q_WS_X11
    qt_x11_wait_for_window_manager(&table);
#endif
    QApplication::setActiveWindow(&table);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&table));

    QVariant value;
    switch ((QVariant::Type)type) {
    case QVariant::Pixmap: {
        QPixmap pm(size);
        pm.fill(Qt::black);
        value = pm;
        break;
    }
    case QVariant::Image: {
        QImage img(size, QImage::Format_Mono);
        qMemSet(img.bits(), 0, img.byteCount());
        value = img;
        break;
    }
    case QVariant::Icon: {
        QPixmap pm(size);
        pm.fill(Qt::black);
        value = QIcon(pm);
        break;
    }
    case QVariant::Color:
        value = QColor(Qt::green);
        break;
    default:
        break;
    }

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setData(Qt::DecorationRole, value);
    table.setItem(0, 0, item);
    item->setSelected(true);

    QApplication::processEvents();

    QTRY_COMPARE(delegate.decorationRect.size(), expected);
}

void tst_QItemDelegate::editorEvent_data()
{
    QTest::addColumn<QRect>("rect");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("checkState");
    QTest::addColumn<int>("flags");
    QTest::addColumn<bool>("inCheck");
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("button");
    QTest::addColumn<bool>("edited");
    QTest::addColumn<int>("expectedCheckState");

    QTest::newRow("unchecked, checkable, release")
        << QRect(0, 0, 20, 20)
        << QString("foo")
        << (int)(Qt::Unchecked)
        << (int)(Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Checked);

    QTest::newRow("checked, checkable, release")
        << QRect(0, 0, 20, 20)
        << QString("foo")
        << (int)(Qt::Checked)
        << (int)(Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, checkable, release")
        << QRect(0, 0, 20, 20)
        << QString("foo")
        << (int)(Qt::Unchecked)
        << (int)(Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Checked);

    QTest::newRow("unchecked, checkable, release, right button")
        << QRect(0, 0, 20, 20)
        << QString("foo")
        << (int)(Qt::Unchecked)
        << (int)(Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::RightButton)
        << false
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, checkable, release outside")
        << QRect(0, 0, 20, 20)
        << QString("foo")
        << (int)(Qt::Unchecked)
        << (int)(Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled)
        << false
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << false
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, checkable, dblclick")
        << QRect(0, 0, 20, 20)
        << QString("foo")
        << (int)(Qt::Unchecked)
        << (int)(Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled)
        << true
        << (int)(QEvent::MouseButtonDblClick)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Unchecked);
}

void tst_QItemDelegate::editorEvent()
{
    QFETCH(QRect, rect);
    QFETCH(QString, text);
    QFETCH(int, checkState);
    QFETCH(int, flags);
    QFETCH(bool, inCheck);
    QFETCH(int, type);
    QFETCH(int, button);
    QFETCH(bool, edited);
    QFETCH(int, expectedCheckState);

    QStandardItemModel model(1, 1);
    QModelIndex index = model.index(0, 0);
    QVERIFY(index.isValid());

    QStandardItem *item = model.itemFromIndex(index);
    item->setText(text);
    item->setCheckState((Qt::CheckState)checkState);
    item->setFlags((Qt::ItemFlags)flags);

    QStyleOptionViewItem option;
    option.rect = rect;
    option.state |= QStyle::State_Enabled;

    const int checkMargin = qApp->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, 0) + 1;
    QPoint pos = inCheck ? qApp->style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &option, 0).center() + QPoint(checkMargin, 0) : QPoint(200,200);

    QEvent *event = new QMouseEvent((QEvent::Type)type,
                                    pos,
                                    (Qt::MouseButton)button,
                                    (Qt::MouseButton)button,
                                    Qt::NoModifier);
    TestItemDelegate delegate;
    bool wasEdited = delegate.editorEvent(event, &model, option, index);
    delete event;

    QApplication::processEvents();

    QCOMPARE(wasEdited, edited);
    QCOMPARE(index.data(Qt::CheckStateRole).toInt(), expectedCheckState);
}

enum WidgetType
{
    LineEdit,
    TextEdit,
    PlainTextEdit
};
Q_DECLARE_METATYPE(WidgetType);

void tst_QItemDelegate::enterKey_data()
{
    QTest::addColumn<WidgetType>("widget");
    QTest::addColumn<int>("key");
    QTest::addColumn<bool>("expectedFocus");

    QTest::newRow("lineedit enter") << LineEdit << int(Qt::Key_Enter) << false;
    QTest::newRow("textedit enter") << TextEdit << int(Qt::Key_Enter) << true;
    QTest::newRow("plaintextedit enter") << PlainTextEdit << int(Qt::Key_Enter) << true;
    QTest::newRow("plaintextedit return") << PlainTextEdit << int(Qt::Key_Return) << true;
    QTest::newRow("plaintextedit tab") << PlainTextEdit << int(Qt::Key_Tab) << false;
    QTest::newRow("lineedit tab") << LineEdit << int(Qt::Key_Tab) << false;
}

void tst_QItemDelegate::enterKey()
{
    QFETCH(WidgetType, widget);
    QFETCH(int, key);
    QFETCH(bool, expectedFocus);

    QStandardItemModel model;
    model.appendRow(new QStandardItem());

    QListView view;
    view.setModel(&model);
    view.show();
    QApplication::setActiveWindow(&view);
    view.setFocus();
    QTest::qWait(30);

    struct TestDelegate : public QItemDelegate
    {
        WidgetType widgetType;
        virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
        {
            QWidget *editor = 0;
            switch(widgetType) {
                case LineEdit:
                    editor = new QLineEdit(parent);
                    break;
                case TextEdit:
                    editor = new QTextEdit(parent);
                    break;
                case PlainTextEdit:
                    editor = new QPlainTextEdit(parent);
                    break;
            }
            editor->setObjectName(QString::fromLatin1("TheEditor"));
            return editor;
        }
    } delegate;

    delegate.widgetType = widget;

    view.setItemDelegate(&delegate);
    QModelIndex index = model.index(0, 0);
    view.setCurrentIndex(index); // the editor will only selectAll on the current index
    view.edit(index);
    QTest::qWait(30);

    QList<QWidget*> lineEditors = qFindChildren<QWidget *>(view.viewport(), QString::fromLatin1("TheEditor"));
    QCOMPARE(lineEditors.count(), 1);

    QPointer<QWidget> editor = lineEditors.at(0);
    QCOMPARE(editor->hasFocus(), true);

    QTest::keyClick(editor, Qt::Key(key));
    QApplication::processEvents();

    // The line edit has already been destroyed, so avoid that case.
    if (widget == TextEdit || widget == PlainTextEdit) {
        QVERIFY(!editor.isNull());
        QCOMPARE(editor && editor->hasFocus(), expectedFocus);
    }
}

void tst_QItemDelegate::task257859_finalizeEdit()
{
    QStandardItemModel model;
    model.appendRow(new QStandardItem());

    QListView view;
    view.setModel(&model);
    view.show();
    QApplication::setActiveWindow(&view);
    view.setFocus();
    QTest::qWait(30);

    QModelIndex index = model.index(0, 0);
    view.edit(index);
    QTest::qWait(30);

    QList<QLineEdit *> lineEditors = qFindChildren<QLineEdit *>(view.viewport());
    QCOMPARE(lineEditors.count(), 1);

    QPointer<QWidget> editor = lineEditors.at(0);
    QCOMPARE(editor->hasFocus(), true);

    QDialog dialog;
    QTimer::singleShot(500, &dialog, SLOT(close()));
    dialog.exec();
    QTRY_VERIFY(!editor);
}

void tst_QItemDelegate::QTBUG4435_keepSelectionOnCheck()
{
    QStandardItemModel model(3, 1);
    for (int i = 0; i < 3; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("Item ") + QString::number(i));
        item->setCheckable(true);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        model.setItem(i, item);
    }
    QTableView view;
    view.setModel(&model);
    view.setItemDelegate(new TestItemDelegate);
    view.show();
    view.selectAll();
    QTest::qWaitForWindowShown(&view);
    QStyleOptionViewItem option;
    option.rect = view.visualRect(model.index(0, 0));
    const int checkMargin = qApp->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, 0) + 1;
    QPoint pos = qApp->style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &option, 0).center()
                 + QPoint(checkMargin, 0);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, pos);
    QTRY_VERIFY(view.selectionModel()->isColumnSelected(0, QModelIndex()));
    QCOMPARE(model.item(0)->checkState(), Qt::Checked);
}


// ### _not_ covered:

// editing with a custom editor factory

// painting when editing
// painting elided text
// painting wrapped text
// painting focus
// painting icon
// painting color
// painting check
// painting selected

// rect for invalid
// rect for pixmap
// rect for image
// rect for icon
// rect for check

QTEST_MAIN(tst_QItemDelegate)
#include "tst_qitemdelegate.moc"
