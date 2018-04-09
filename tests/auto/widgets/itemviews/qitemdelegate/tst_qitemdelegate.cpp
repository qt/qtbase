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
#include <QComboBox>
#include <QAbstractItemDelegate>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDialog>

#include <QtWidgets/private/qabstractitemdelegate_p.h>

Q_DECLARE_METATYPE(QAbstractItemDelegate::EndEditHint)

#if defined (Q_OS_WIN) && !defined(Q_OS_WINRT)
#include <windows.h>
#define Q_CHECK_PAINTEVENTS \
    if (::SwitchDesktop(::GetThreadDesktop(::GetCurrentThreadId())) == 0) \
        QSKIP("The widgets don't get the paint events");
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

private slots:
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
    void testEventFilter();
    void dateTimeEditor_data();
    void dateTimeEditor();
    void dateAndTimeEditorTest2();
    void uintEdit();
    void decoration_data();
    void decoration();
    void editorEvent_data();
    void editorEvent();
    void enterKey_data();
    void enterKey();
    void comboBox();
    void testLineEditValidation_data();
    void testLineEditValidation();

    void task257859_finalizeEdit();
    void QTBUG4435_keepSelectionOnCheck();

    void QTBUG16469_textForRole();
    void dateTextForRole_data();
    void dateTextForRole();

#ifdef QT_BUILD_INTERNAL
private:
    struct RoleDelegate : public QItemDelegate
    {
        QString textForRole(Qt::ItemDataRole role, const QVariant &value, const QLocale &locale)
        {
            QAbstractItemDelegatePrivate *d = reinterpret_cast<QAbstractItemDelegatePrivate *>(qGetPtrHelper(d_ptr));
            return d->textForRole(role, value, locale);
        }
    };
#endif
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

    QList<QLineEdit*> lineEditors = view.viewport()->findChildren<QLineEdit *>();
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

    QList<QDoubleSpinBox*> editors = view.viewport()->findChildren<QDoubleSpinBox *>();
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
    QVERIFY(QTest::qWaitForWindowExposed(&table));

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setText(itemText);
    item->setFont(itemFont);
    table.setItem(0, 0, item);

    QApplication::processEvents();

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
        << QRect(0, 0, 50 + 2*m, 1000)
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
        << QRect(0, 0, 50 + 2 * m, 1000)
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
        << QRect(0, 0, 50 + 2 * m, 1000)
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
        << QRect(0, 0, 50 + 2 * m, 1000)
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
void tst_QItemDelegate::testEventFilter()
{
    TestItemDelegate delegate;
    QWidget widget;
    QEvent *event;

    qRegisterMetaType<QAbstractItemDelegate::EndEditHint>("QAbstractItemDelegate::EndEditHint");

    QSignalSpy commitDataSpy(&delegate, SIGNAL(commitData(QWidget*)));
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

    QTimeEdit *timeEditor = widget.viewport()->findChild<QTimeEdit *>();
    QVERIFY(timeEditor);
    QCOMPARE(timeEditor->time(), time);
    // The data must actually be different in order for the model
    // to be updated.
    timeEditor->setTime(time.addSecs(60));

    widget.clearFocus();
    qApp->setActiveWindow(&widget);
    widget.setFocus();
    widget.editItem(item2);

    QTRY_VERIFY(widget.viewport()->findChild<QDateEdit *>());
    QDateEdit *dateEditor = widget.viewport()->findChild<QDateEdit *>();
    QCOMPARE(dateEditor->date(), date);
    dateEditor->setDate(date.addDays(60));

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
    dateTimeEditor->setTime(time.addSecs(600));
    widget.clearFocus();

    QCOMPARE(item1->data(Qt::EditRole).userType(), int(QMetaType::QTime));
    QCOMPARE(item2->data(Qt::EditRole).userType(), int(QMetaType::QDate));
    QCOMPARE(item3->data(Qt::EditRole).userType(), int(QMetaType::QDateTime));
}

// A delegate where we can either enforce a certain widget or use the standard widget.
class ChooseEditorDelegate : public QItemDelegate
{
public:
    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &o, const QModelIndex &i) const
    {
        if (m_editor) {
            m_editor->setParent(parent);
            return m_editor;
        }
        m_editor = QItemDelegate::createEditor(parent, o, i);
        return m_editor;
    }

    virtual void destroyEditor(QWidget *editor, const QModelIndex &i) const
    {   // This is a reimplementation of QAbstractItemDelegate::destroyEditor just set the variable m_editor to 0
        // The only reason we do this is to avoid the not recommended direct delete of editor (destroyEditor uses deleteLater)
        QItemDelegate::destroyEditor(editor, i); // Allow destroy
        m_editor = 0;                            // but clear the variable
    }

    ChooseEditorDelegate(QObject *parent = 0) : QItemDelegate(parent) { }
    void setNextOpenEditor(QWidget *w) { m_editor = w; }
    QWidget* currentEditor() const { return m_editor; }
private:
    mutable QPointer<QWidget> m_editor;
};

// We could (nearly) use a normal QTableView but in order not to add many seconds to the autotest
// (and save a few lines) we do this
class FastEditItemView : public QTableView
{
public:
    QWidget* fastEdit(const QModelIndex &i) // Consider this as QAbstractItemView::edit( )
    {
        QWidget *v = itemDelegate()->createEditor(viewport(), viewOptions(), i);
        if (v)
            itemDelegate()->setEditorData(v, i);
        return v;
    }
    void doCloseEditor(QWidget *editor) // Consider this as QAbstractItemView::closeEditor( )
    {
        itemDelegate()->destroyEditor(editor, QModelIndex());
    }
};

void tst_QItemDelegate::dateAndTimeEditorTest2()
{
    // prepare createeditor
    FastEditItemView w;
    QStandardItemModel s;
    s.setRowCount(2);
    s.setColumnCount(1);
    w.setModel(&s);
    ChooseEditorDelegate *d = new ChooseEditorDelegate(&w);
    w.setItemDelegate(d);
    const QTime time1(3, 13, 37);
    const QDate date1(2013, 3, 7);

    QPointer<QTimeEdit> timeEdit;
    QPointer<QDateEdit> dateEdit;
    QPointer<QDateTimeEdit> dateTimeEdit;

    // Do some checks
    // a. Open time editor on empty cell + write QTime data
    const QModelIndex i1 = s.index(0, 0);
    timeEdit = new QTimeEdit();
    d->setNextOpenEditor(timeEdit);
    QCOMPARE(w.fastEdit(i1), timeEdit.data());
    timeEdit->setTime(time1);
    d->setModelData(timeEdit, &s, i1);
    QCOMPARE(s.data(i1).type(), QVariant::Time); // ensure that we wrote a time variant.
    QCOMPARE(s.data(i1).toTime(), time1);        // ensure that it is the correct time.
    w.doCloseEditor(timeEdit);
    QVERIFY(d->currentEditor() == 0); // should happen at doCloseEditor. We only test this once.

    // b. Test that automatic edit of a QTime value is QTimeEdit (and not QDateTimeEdit)
    QWidget *editor = w.fastEdit(i1);
    timeEdit = qobject_cast<QTimeEdit*>(editor);
    QVERIFY(timeEdit);
    QCOMPARE(timeEdit->time(), time1);
    w.doCloseEditor(timeEdit);

    const QTime time2(4, 14, 37);
    const QDate date2(2014, 4, 7);
    const QDateTime datetime1(date1, time1);
    const QDateTime datetime2(date2, time2);

    // c. Test that the automatic open of an QDateTime is QDateTimeEdit + value check + set check
    s.setData(i1, datetime2);
    editor = w.fastEdit(i1);
    timeEdit = qobject_cast<QTimeEdit*>(editor);
    QVERIFY(!timeEdit);
    dateEdit = qobject_cast<QDateEdit*>(editor);
    QVERIFY(!dateEdit);
    dateTimeEdit =  qobject_cast<QDateTimeEdit*>(editor);
    QVERIFY(dateTimeEdit);
    QCOMPARE(dateTimeEdit->dateTime(), datetime2);
    dateTimeEdit->setDateTime(datetime1);
    d->setModelData(dateTimeEdit, &s, i1);
    QCOMPARE(s.data(i1).type(), QVariant::DateTime); // ensure that we wrote a datetime variant.
    QCOMPARE(s.data(i1).toDateTime(), datetime1);
    w.doCloseEditor(dateTimeEdit);

    // d. Open date editor on empty cell + write QDate data (similar to a)
    const QModelIndex i2 = s.index(1, 0);
    dateEdit = new QDateEdit();
    d->setNextOpenEditor(dateEdit);
    QCOMPARE(w.fastEdit(i2), dateEdit.data());
    dateEdit->setDate(date1);
    d->setModelData(dateEdit, &s, i2);
    QCOMPARE(s.data(i2).type(), QVariant::Date); // ensure that we wrote a time variant.
    QCOMPARE(s.data(i2).toDate(), date1);        // ensure that it is the correct date.
    w.doCloseEditor(dateEdit);

    // e. Test that the default editor editor (QDateEdit) on a QDate (index i2)  (similar to b)
    editor = w.fastEdit(i2);
    dateEdit = qobject_cast<QDateEdit*>(editor);
    QVERIFY(dateEdit);
    QCOMPARE(dateEdit->date(), date1);
    w.doCloseEditor(dateEdit);
}

void tst_QItemDelegate::uintEdit()
{
    QListView view;
    QStandardItemModel model;

    {
        QStandardItem *data=new QStandardItem;
        data->setEditable(true);
        data->setData(QVariant((uint)1), Qt::DisplayRole);
        model.setItem(0, 0, data);
    }
    {
        QStandardItem *data=new QStandardItem;
        data->setEditable(true);
        data->setData(QVariant((uint)1), Qt::DisplayRole);
        model.setItem(1, 0, data);
    }

    view.setModel(&model);
    view.setEditTriggers(QAbstractItemView::AllEditTriggers);

    const QModelIndex firstCell = model.index(0, 0);

    QCOMPARE(firstCell.data(Qt::DisplayRole).userType(), static_cast<int>(QMetaType::UInt));

    view.selectionModel()->setCurrentIndex(model.index(0, 0), QItemSelectionModel::Select);
    view.edit(firstCell);

    QSpinBox *sb = view.findChild<QSpinBox*>();
    QVERIFY(sb);

    sb->stepUp();

    // Select another index to trigger the end of editing.
    const QModelIndex secondCell = model.index(1, 0);
    view.selectionModel()->setCurrentIndex(secondCell, QItemSelectionModel::Select);

    QCOMPARE(firstCell.data(Qt::DisplayRole).userType(), static_cast<int>(QMetaType::UInt));
    QCOMPARE(firstCell.data(Qt::DisplayRole).toUInt(), static_cast<uint>(2));


    view.edit(secondCell);

    // The first spinbox is deleted with deleteLater, so it is still there.
    QList<QSpinBox*> sbList = view.findChildren<QSpinBox*>();
    QCOMPARE(sbList.size(), 2);

    sb = sbList.at(1);

    sb->stepDown(); // 1 -> 0
    sb->stepDown(); // 0 (no effect)
    sb->stepDown(); // 0 (no effect)

    // Select another index to trigger the end of editing.
    view.selectionModel()->setCurrentIndex(firstCell, QItemSelectionModel::Select);

    QCOMPARE(secondCell.data(Qt::DisplayRole).userType(), static_cast<int>(QMetaType::UInt));
    QCOMPARE(secondCell.data(Qt::DisplayRole).toUInt(), static_cast<uint>(0));
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

    // This demands too much memory and potentially hangs. Feel free to uncomment
    // for your own testing.
//    QTest::newRow("pixmap 30x30 big")
//        << (int)QVariant::Pixmap
//        << QSize(1024, 1024)        // Over 1M
//        << QSize(1024, 1024);
}

void tst_QItemDelegate::decoration()
{
    Q_CHECK_PAINTEVENTS

    QFETCH(int, type);
    QFETCH(QSize, size);
    QFETCH(QSize, expected);

    QTableWidget table(1, 1);
    TestItemDelegate delegate;
    table.setItemDelegate(&delegate);
    table.show();
    QApplication::setActiveWindow(&table);
    QVERIFY(QTest::qWaitForWindowActive(&table));

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
        memset(img.bits(), 0, img.sizeInBytes());
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
    QTest::addColumn<int>("checkState");
    QTest::addColumn<int>("flags");
    QTest::addColumn<bool>("inCheck");
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("button");
    QTest::addColumn<bool>("edited");
    QTest::addColumn<int>("expectedCheckState");

    const int defaultFlags = (int)(Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled);

    QTest::newRow("unchecked, checkable, release")
        << (int)(Qt::Unchecked)
        << defaultFlags
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Checked);

    QTest::newRow("checked, checkable, release")
        << (int)(Qt::Checked)
        << defaultFlags
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, checkable, release")
        << (int)(Qt::Unchecked)
        << defaultFlags
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Checked);

    QTest::newRow("unchecked, checkable, release, right button")
        << (int)(Qt::Unchecked)
        << defaultFlags
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::RightButton)
        << false
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, checkable, release outside")
        << (int)(Qt::Unchecked)
        << defaultFlags
        << false
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << false
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, checkable, dblclick")
        << (int)(Qt::Unchecked)
        << defaultFlags
        << true
        << (int)(QEvent::MouseButtonDblClick)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, tristate, release")
        << (int)(Qt::Unchecked)
        << (int)(defaultFlags | Qt::ItemIsAutoTristate)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Checked);

    QTest::newRow("partially checked, tristate, release")
        << (int)(Qt::PartiallyChecked)
        << (int)(defaultFlags | Qt::ItemIsAutoTristate)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Checked);

    QTest::newRow("checked, tristate, release")
        << (int)(Qt::Checked)
        << (int)(defaultFlags | Qt::ItemIsAutoTristate)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Unchecked);

    QTest::newRow("unchecked, user-tristate, release")
        << (int)(Qt::Unchecked)
        << (int)(defaultFlags | Qt::ItemIsUserTristate)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::PartiallyChecked);

    QTest::newRow("partially checked, user-tristate, release")
        << (int)(Qt::PartiallyChecked)
        << (int)(defaultFlags | Qt::ItemIsUserTristate)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Checked);

    QTest::newRow("checked, user-tristate, release")
        << (int)(Qt::Checked)
        << (int)(defaultFlags | Qt::ItemIsUserTristate)
        << true
        << (int)(QEvent::MouseButtonRelease)
        << (int)(Qt::LeftButton)
        << true
        << (int)(Qt::Unchecked);
}

void tst_QItemDelegate::editorEvent()
{
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
    item->setText("foo");
    item->setCheckState((Qt::CheckState)checkState);
    item->setFlags((Qt::ItemFlags)flags);

    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 20, 20);
    option.state |= QStyle::State_Enabled;
    // mimic QStyledItemDelegate::initStyleOption logic
    option.features |= QStyleOptionViewItem::HasCheckIndicator | QStyleOptionViewItem::HasDisplay;
    option.checkState = Qt::CheckState(checkState);

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
    QTest::newRow("lineedit return") << LineEdit << int(Qt::Key_Return) << false;
    QTest::newRow("lineedit tab") << LineEdit << int(Qt::Key_Tab) << false;
    QTest::newRow("lineedit backtab") << LineEdit << int(Qt::Key_Backtab) << false;

    QTest::newRow("textedit enter") << TextEdit << int(Qt::Key_Enter) << true;
    QTest::newRow("textedit return") << TextEdit << int(Qt::Key_Return) << true;
    QTest::newRow("textedit tab") << TextEdit << int(Qt::Key_Tab) << true;
    QTest::newRow("textedit backtab") << TextEdit << int(Qt::Key_Backtab) << false;

    QTest::newRow("plaintextedit enter") << PlainTextEdit << int(Qt::Key_Enter) << true;
    QTest::newRow("plaintextedit return") << PlainTextEdit << int(Qt::Key_Return) << true;
    QTest::newRow("plaintextedit tab") << PlainTextEdit << int(Qt::Key_Tab) << true;
    QTest::newRow("plaintextedit backtab") << PlainTextEdit << int(Qt::Key_Backtab) << false;
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
    QVERIFY(QTest::qWaitForWindowActive(&view));

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

    QList<QWidget*> lineEditors = view.viewport()->findChildren<QWidget *>(QString::fromLatin1("TheEditor"));
    QCOMPARE(lineEditors.count(), 1);

    QPointer<QWidget> editor = lineEditors.at(0);
    QCOMPARE(editor->hasFocus(), true);

    QTest::keyClick(editor, Qt::Key(key));

    if (expectedFocus) {
        QVERIFY(!editor.isNull());
        QVERIFY(editor->hasFocus());
    } else {
        QTRY_VERIFY(editor.isNull()); // editor deletion happens via deleteLater
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
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QModelIndex index = model.index(0, 0);
    view.edit(index);

    QList<QLineEdit *> lineEditors = view.viewport()->findChildren<QLineEdit *>();
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
    view.setItemDelegate(new TestItemDelegate(&view));
    view.show();
    view.selectAll();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QStyleOptionViewItem option;
    option.rect = view.visualRect(model.index(0, 0));
    // mimic QStyledItemDelegate::initStyleOption logic
    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::HasCheckIndicator;
    option.checkState = Qt::CheckState(model.index(0, 0).data(Qt::CheckStateRole).toInt());
    const int checkMargin = qApp->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, 0) + 1;
    QPoint pos = qApp->style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &option, 0).center()
                 + QPoint(checkMargin, 0);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, pos);
    QTRY_VERIFY(view.selectionModel()->isColumnSelected(0, QModelIndex()));
    QCOMPARE(model.item(0)->checkState(), Qt::Checked);
}

void tst_QItemDelegate::comboBox()
{
    QTableWidgetItem *item1 = new QTableWidgetItem;
    item1->setData(Qt::DisplayRole, true);

    QTableWidget widget(1, 1);
    widget.setItem(0, 0, item1);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.editItem(item1);

    QComboBox *boolEditor = nullptr;
    QTRY_VERIFY( (boolEditor = widget.viewport()->findChild<QComboBox*>()) );
    QCOMPARE(boolEditor->currentIndex(), 1); // True is selected initially.
    // The data must actually be different in order for the model
    // to be updated.
    boolEditor->setCurrentIndex(0);
    QCOMPARE(boolEditor->currentIndex(), 0); // Changed to false.

    widget.clearFocus();
    widget.setFocus();

    QVariant data = item1->data(Qt::EditRole);
    QCOMPARE(data.userType(), (int)QMetaType::Bool);
    QCOMPARE(data.toBool(), false);
}

void tst_QItemDelegate::testLineEditValidation_data()
{
    QTest::addColumn<int>("key");

    QTest::newRow("enter") << int(Qt::Key_Enter);
    QTest::newRow("return") << int(Qt::Key_Return);
    QTest::newRow("tab") << int(Qt::Key_Tab);
    QTest::newRow("backtab") << int(Qt::Key_Backtab);
    QTest::newRow("escape") << int(Qt::Key_Escape);
}

void tst_QItemDelegate::testLineEditValidation()
{
    QFETCH(int, key);

    struct TestDelegate : public QItemDelegate
    {
        virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            Q_UNUSED(option);
            Q_UNUSED(index);

            QLineEdit *editor = new QLineEdit(parent);
            QRegularExpression re("\\w+,\\w+"); // two words separated by a comma
            editor->setValidator(new QRegularExpressionValidator(re, editor));
            editor->setObjectName(QStringLiteral("TheEditor"));
            return editor;
        }
    } delegate;

    QStandardItemModel model;
    // need a couple of dummy items to test tab and back tab
    model.appendRow(new QStandardItem(QStringLiteral("dummy")));
    QStandardItem *item = new QStandardItem(QStringLiteral("abc,def"));
    model.appendRow(item);
    model.appendRow(new QStandardItem(QStringLiteral("dummy")));

    QListView view;
    view.setModel(&model);
    view.setItemDelegate(&delegate);
    view.show();
    view.setFocus();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QPointer<QLineEdit> editor;
    QPersistentModelIndex index = model.indexFromItem(item);

    view.setCurrentIndex(index);
    view.edit(index);

    const auto findEditors = [&]() {
        return view.findChildren<QLineEdit *>(QStringLiteral("TheEditor"));
    };
    QCOMPARE(findEditors().count(), 1);
    editor = findEditors().at(0);
    editor->clear();

    // first try to set a valid text
    QTest::keyClicks(editor, QStringLiteral("foo,bar"));

    // close the editor
    QTest::keyClick(editor, Qt::Key(key));

    QTRY_VERIFY(editor.isNull());
    if (key != Qt::Key_Escape)
        QCOMPARE(item->data(Qt::DisplayRole).toString(), QStringLiteral("foo,bar"));
    else
        QCOMPARE(item->data(Qt::DisplayRole).toString(), QStringLiteral("abc,def"));

    // now an invalid (but partially matching) text
    view.setCurrentIndex(index);
    view.edit(index);

    QTRY_COMPARE(findEditors().count(), 1);
    editor = findEditors().at(0);
    editor->clear();

    // edit
    QTest::keyClicks(editor, QStringLiteral("foobar"));

    // try to close the editor
    QTest::keyClick(editor, Qt::Key(key));

    if (key != Qt::Key_Escape) {
        QVERIFY(!editor.isNull());
        QCOMPARE(qApp->focusWidget(), editor.data());
        QCOMPARE(editor->text(), QStringLiteral("foobar"));
        QCOMPARE(item->data(Qt::DisplayRole).toString(), QStringLiteral("foo,bar"));
    } else {
        QTRY_VERIFY(editor.isNull());
        QCOMPARE(item->data(Qt::DisplayRole).toString(), QStringLiteral("abc,def"));
    }

    // reset the view to forcibly close the editor
    view.reset();
    QTRY_COMPARE(findEditors().count(), 0);

    // set a valid text again
    view.setCurrentIndex(index);
    view.edit(index);

    QTRY_COMPARE(findEditors().count(), 1);
    editor = findEditors().at(0);
    editor->clear();

    // set a valid text
    QTest::keyClicks(editor, QStringLiteral("gender,bender"));

    // close the editor
    QTest::keyClick(editor, Qt::Key(key));

    QTRY_VERIFY(editor.isNull());
    if (key != Qt::Key_Escape)
        QCOMPARE(item->data(Qt::DisplayRole).toString(), QStringLiteral("gender,bender"));
    else
        QCOMPARE(item->data(Qt::DisplayRole).toString(), QStringLiteral("abc,def"));
}

void tst_QItemDelegate::QTBUG16469_textForRole()
{
#ifndef QT_BUILD_INTERNAL
    QSKIP("This test requires a developer build");
#else
    RoleDelegate delegate;
    QLocale locale;

    const float f = 123.456f;
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, f, locale), locale.toString(f));
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, f, locale), locale.toString(f));
    const double d = 123.456;
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, d, locale), locale.toString(d, 'g', 6));
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, d, locale), locale.toString(d, 'g', 6));
    const int i = 1234567;
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, i, locale), locale.toString(i));
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, i, locale), locale.toString(i));
    const qlonglong ll = 1234567;
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, ll, locale), locale.toString(ll));
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, ll, locale), locale.toString(ll));
    const uint ui = 1234567;
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, ui, locale), locale.toString(ui));
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, ui, locale), locale.toString(ui));
    const qulonglong ull = 1234567;
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, ull, locale), locale.toString(ull));
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, ull, locale), locale.toString(ull));

    const QString text("text");
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, text, locale), text);
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, text, locale), text);
    const QString multipleLines("multiple\nlines");
    QString multipleLines2 = multipleLines;
    multipleLines2.replace(QLatin1Char('\n'), QChar::LineSeparator);
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, multipleLines, locale), multipleLines2);
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, multipleLines, locale), multipleLines);
#endif
}

void tst_QItemDelegate::dateTextForRole_data()
{
#ifdef QT_BUILD_INTERNAL
    QTest::addColumn<QDateTime>("when");

    QTest::newRow("now") << QDateTime::currentDateTime(); // It's a local time
    QDate date(2013, 12, 11);
    QTime time(10, 9, 8, 765);
    // Ensure we exercise every time-spec variant:
    QTest::newRow("local") << QDateTime(date, time, Qt::LocalTime);
    QTest::newRow("UTC") << QDateTime(date, time, Qt::UTC);
    QTest::newRow("zone") << QDateTime(date, time, QTimeZone("Europe/Dublin"));
    QTest::newRow("offset") << QDateTime(date, time, Qt::OffsetFromUTC, 36000);
#endif
}

void tst_QItemDelegate::dateTextForRole()
{
#ifndef QT_BUILD_INTERNAL
    QSKIP("This test requires a developer build");
#else
    QFETCH(QDateTime, when);
    RoleDelegate delegate;
    QLocale locale;
# define CHECK(value) \
    QCOMPARE(delegate.textForRole(Qt::DisplayRole, value, locale), locale.toString(value, QLocale::ShortFormat)); \
    QCOMPARE(delegate.textForRole(Qt::ToolTipRole, value, locale), locale.toString(value, QLocale::LongFormat))

    CHECK(when);
    CHECK(when.date());
    CHECK(when.time());
# undef CHECK
#endif
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
