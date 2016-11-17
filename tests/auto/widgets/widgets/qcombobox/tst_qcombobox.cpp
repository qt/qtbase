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

#include "qcombobox.h"
#include <private/qcombobox_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#include <qfontcombobox.h>
#include <qdesktopwidget.h>
#include <qapplication.h>
#include <qpushbutton.h>
#include <qdialog.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qheaderview.h>
#include <qlistwidget.h>
#include <qtreewidget.h>
#include <qtablewidget.h>
#include <qscrollbar.h>
#include <qboxlayout.h>

#include <qstandarditemmodel.h>
#include <qstringlistmodel.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qdialog.h>
#include <qstringlist.h>
#include <qvalidator.h>
#include <qcompleter.h>
#include <qstylefactory.h>
#include <qabstractitemview.h>
#include <qstyleditemdelegate.h>
#include <qstandarditemmodel.h>
#include <qproxystyle.h>
#include <qfont.h>

static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

class tst_QComboBox : public QObject
{
    Q_OBJECT

public:
    tst_QComboBox() {}

private slots:
    void getSetCheck();
    void ensureReturnIsIgnored();
    void setEditable();
    void setPalette();
    void sizeAdjustPolicy();
    void clear();
    void insertPolicy_data();
    void insertPolicy();
    void virtualAutocompletion();
    void autoCompletionCaseSensitivity();
    void hide();
    void currentIndex_data();
    void currentIndex();
    void insertItems_data();
    void insertItems();
    void insertItem_data();
    void insertItem();
    void insertOnCurrentIndex();
    void textpixmapdata_data();
    void textpixmapdata();
    void currentTextChanged_data();
    void currentTextChanged();
    void editTextChanged();
    void setModel();
    void modelDeleted();
    void setMaxCount();
    void setCurrentIndex();
    void setCurrentText_data();
    void setCurrentText();
    void convenienceViews();
    void findText_data();
    void findText();
    void flaggedItems_data();
    void flaggedItems();
    void pixmapIcon();
#ifndef QT_NO_WHEELEVENT
    void mouseWheel_data();
    void mouseWheel();
    void popupWheelHandling();
#endif // !QT_NO_WHEELEVENT
    void layoutDirection();
    void itemListPosition();
    void separatorItem_data();
    void separatorItem();
#ifndef QT_NO_STYLE_FUSION
    void task190351_layout();
    void task191329_size();
#endif
    void task166349_setEditableOnReturn();
    void task190205_setModelAdjustToContents();
    void task248169_popupWithMinimalSize();
    void task247863_keyBoardSelection();
    void task220195_keyBoardSelection2();
    void setModelColumn();
    void noScrollbar_data();
    void noScrollbar();
    void setItemDelegate();
    void task253944_itemDelegateIsReset();
    void subControlRectsWithOffset_data();
    void subControlRectsWithOffset();
#ifndef QT_NO_STYLE_WINDOWS
    void task260974_menuItemRectangleForComboBoxPopup();
#endif
    void removeItem();
    void resetModel();
    void keyBoardNavigationWithMouse();
    void task_QTBUG_1071_changingFocusEmitsActivated();
    void maxVisibleItems_data();
    void maxVisibleItems();
    void task_QTBUG_10491_currentIndexAndModelColumn();
    void highlightedSignal();
    void itemData();
    void task_QTBUG_31146_popupCompletion();
    void task_QTBUG_41288_completerChangesCurrentIndex();
    void task_QTBUG_54191_slotOnEditTextChangedSetsComboBoxToReadOnly();
    void keyboardSelection();
    void setCustomModelAndView();
    void updateDelegateOnEditableChange();
    void respectChangedOwnershipOfItemView();
    void task_QTBUG_39088_inputMethodHints();
    void task_QTBUG_49831_scrollerNotActivated();
    void task_QTBUG_56693_itemFontFromModel();
};

class MyAbstractItemDelegate : public QAbstractItemDelegate
{
public:
    MyAbstractItemDelegate() : QAbstractItemDelegate() {};
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const { return QSize(); }
};

class MyAbstractItemModel: public QAbstractItemModel
{
public:
    MyAbstractItemModel() : QAbstractItemModel() {};
    QModelIndex index(int, int, const QModelIndex &) const { return QModelIndex(); }
    QModelIndex parent(const QModelIndex &) const  { return QModelIndex(); }
    int rowCount(const QModelIndex &) const { return 0; }
    int columnCount(const QModelIndex &) const { return 0; }
    bool hasChildren(const QModelIndex &) const { return false; }
    QVariant data(const QModelIndex &, int) const { return QVariant(); }
    bool setData(const QModelIndex &, const QVariant &, int) { return false; }
    bool insertRows(int, int, const QModelIndex &) { return false; }
    bool insertColumns(int, int, const QModelIndex &) { return false; }
    void setPersistent(const QModelIndex &, const QModelIndex &) {}
    bool removeRows (int, int, const QModelIndex &) { return false; }
    bool removeColumns(int, int, const QModelIndex &) { return false; }
    void reset() {}
};

class MyAbstractItemView : public QAbstractItemView
{
public:
    MyAbstractItemView() : QAbstractItemView() {}
    QRect visualRect(const QModelIndex &) const { return QRect(); }
    void scrollTo(const QModelIndex &, ScrollHint) {}
    QModelIndex indexAt(const QPoint &) const { return QModelIndex(); }
protected:
    QModelIndex moveCursor(CursorAction, Qt::KeyboardModifiers) { return QModelIndex(); }
    int horizontalOffset() const { return 0; }
    int verticalOffset() const { return 0; }
    bool isIndexHidden(const QModelIndex &) const { return false; }
    void setSelection(const QRect &, QItemSelectionModel::SelectionFlags) {}
    QRegion visualRegionForSelection(const QItemSelection &) const { return QRegion(); }
};

// Testing get/set functions
void tst_QComboBox::getSetCheck()
{
    QComboBox obj1;
    // int QComboBox::maxVisibleItems()
    // void QComboBox::setMaxVisibleItems(int)
    obj1.setMaxVisibleItems(100);
    QCOMPARE(100, obj1.maxVisibleItems());
    obj1.setMaxVisibleItems(0);
    QCOMPARE(obj1.maxVisibleItems(), 0);
    QTest::ignoreMessage(QtWarningMsg, "QComboBox::setMaxVisibleItems: "
                         "Invalid max visible items (-2147483648) must be >= 0");
    obj1.setMaxVisibleItems(INT_MIN);
    QCOMPARE(obj1.maxVisibleItems(), 0); // Cannot be set to something negative => old value
    obj1.setMaxVisibleItems(INT_MAX);
    QCOMPARE(INT_MAX, obj1.maxVisibleItems());

    // int QComboBox::maxCount()
    // void QComboBox::setMaxCount(int)
    obj1.setMaxCount(0);
    QCOMPARE(0, obj1.maxCount());
#ifndef QT_DEBUG
    QTest::ignoreMessage(QtWarningMsg, "QComboBox::setMaxCount: Invalid count (-2147483648) must be >= 0");
    obj1.setMaxCount(INT_MIN);
    QCOMPARE(0, obj1.maxCount()); // Setting a value below 0 makes no sense, and shouldn't be allowed
#endif
    obj1.setMaxCount(INT_MAX);
    QCOMPARE(INT_MAX, obj1.maxCount());

    // bool QComboBox::autoCompletion()
    // void QComboBox::setAutoCompletion(bool)
    obj1.setAutoCompletion(false);
    QCOMPARE(false, obj1.autoCompletion());
    obj1.setAutoCompletion(true);
    QCOMPARE(true, obj1.autoCompletion());

    // bool QComboBox::duplicatesEnabled()
    // void QComboBox::setDuplicatesEnabled(bool)
    obj1.setDuplicatesEnabled(false);
    QCOMPARE(false, obj1.duplicatesEnabled());
    obj1.setDuplicatesEnabled(true);
    QCOMPARE(true, obj1.duplicatesEnabled());

    // InsertPolicy QComboBox::insertPolicy()
    // void QComboBox::setInsertPolicy(InsertPolicy)
    obj1.setInsertPolicy(QComboBox::InsertPolicy(QComboBox::NoInsert));
    QCOMPARE(QComboBox::InsertPolicy(QComboBox::NoInsert), obj1.insertPolicy());
    obj1.setInsertPolicy(QComboBox::InsertPolicy(QComboBox::InsertAtTop));
    QCOMPARE(QComboBox::InsertPolicy(QComboBox::InsertAtTop), obj1.insertPolicy());
    obj1.setInsertPolicy(QComboBox::InsertPolicy(QComboBox::InsertAtCurrent));
    QCOMPARE(QComboBox::InsertPolicy(QComboBox::InsertAtCurrent), obj1.insertPolicy());
    obj1.setInsertPolicy(QComboBox::InsertPolicy(QComboBox::InsertAtBottom));
    QCOMPARE(QComboBox::InsertPolicy(QComboBox::InsertAtBottom), obj1.insertPolicy());
    obj1.setInsertPolicy(QComboBox::InsertPolicy(QComboBox::InsertAfterCurrent));
    QCOMPARE(QComboBox::InsertPolicy(QComboBox::InsertAfterCurrent), obj1.insertPolicy());
    obj1.setInsertPolicy(QComboBox::InsertPolicy(QComboBox::InsertBeforeCurrent));
    QCOMPARE(QComboBox::InsertPolicy(QComboBox::InsertBeforeCurrent), obj1.insertPolicy());
    obj1.setInsertPolicy(QComboBox::InsertPolicy(QComboBox::InsertAlphabetically));
    QCOMPARE(QComboBox::InsertPolicy(QComboBox::InsertAlphabetically), obj1.insertPolicy());

    // SizeAdjustPolicy QComboBox::sizeAdjustPolicy()
    // void QComboBox::setSizeAdjustPolicy(SizeAdjustPolicy)
    obj1.setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToContents));
    QCOMPARE(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToContents), obj1.sizeAdjustPolicy());
    obj1.setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow));
    QCOMPARE(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow), obj1.sizeAdjustPolicy());
    obj1.setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength));
    QCOMPARE(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength), obj1.sizeAdjustPolicy());
    obj1.setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon));
    QCOMPARE(QComboBox::SizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon), obj1.sizeAdjustPolicy());

    // int QComboBox::minimumContentsLength()
    // void QComboBox::setMinimumContentsLength(int)
    obj1.setMinimumContentsLength(0);
    QCOMPARE(0, obj1.minimumContentsLength());
    obj1.setMinimumContentsLength(100);
    QCOMPARE(100, obj1.minimumContentsLength());
    obj1.setMinimumContentsLength(INT_MIN);
    QCOMPARE(100, obj1.minimumContentsLength()); // Cannot be set to something negative => old value
    obj1.setMinimumContentsLength(INT_MAX);
    QCOMPARE(INT_MAX, obj1.minimumContentsLength());

    // QLineEdit * QComboBox::lineEdit()
    // void QComboBox::setLineEdit(QLineEdit *)
    QLineEdit *var8 = new QLineEdit(0);
    obj1.setLineEdit(var8);
    QCOMPARE(var8, obj1.lineEdit());
    QTest::ignoreMessage(QtWarningMsg, "QComboBox::setLineEdit: cannot set a 0 line edit");
    obj1.setLineEdit((QLineEdit *)0);
    QCOMPARE(var8, obj1.lineEdit());
    // delete var8; // No delete, since QComboBox takes ownership

    // const QValidator * QComboBox::validator()
    // void QComboBox::setValidator(const QValidator *)
    QIntValidator *var9 = new QIntValidator(0);
    obj1.setValidator(var9);
    QCOMPARE(obj1.validator(), (const QValidator *)var9);
    obj1.setValidator((QValidator *)0);
    QCOMPARE(obj1.validator(), (const QValidator *)0);
    delete var9;

    // QAbstractItemDelegate * QComboBox::itemDelegate()
    // void QComboBox::setItemDelegate(QAbstractItemDelegate *)
    MyAbstractItemDelegate *var10 = new MyAbstractItemDelegate;
    obj1.setItemDelegate(var10);
    QCOMPARE(obj1.itemDelegate(), (QAbstractItemDelegate *)var10);
    QTest::ignoreMessage(QtWarningMsg, "QComboBox::setItemDelegate: cannot set a 0 delegate");
    obj1.setItemDelegate((QAbstractItemDelegate *)0);
    QCOMPARE(obj1.itemDelegate(), (QAbstractItemDelegate *)var10);
    // delete var10; // No delete, since QComboBox takes ownership

    // QAbstractItemModel * QComboBox::model()
    // void QComboBox::setModel(QAbstractItemModel *)
    MyAbstractItemModel *var11 = new MyAbstractItemModel;
    obj1.setModel(var11);
    QCOMPARE(obj1.model(), (QAbstractItemModel *)var11);
    QTest::ignoreMessage(QtWarningMsg, "QComboBox::setModel: cannot set a 0 model");
    obj1.setModel((QAbstractItemModel *)0);
    QCOMPARE(obj1.model(), (QAbstractItemModel *)var11);
    delete var11;
    obj1.model();

    // int QComboBox::modelColumn()
    // void QComboBox::setModelColumn(int)
    obj1.setModelColumn(0);
    QCOMPARE(0, obj1.modelColumn());
    obj1.setModelColumn(INT_MIN);
//    QCOMPARE(0, obj1.modelColumn()); // Cannot be set to something negative => column 0
    obj1.setModelColumn(INT_MAX);
    QCOMPARE(INT_MAX, obj1.modelColumn());
    obj1.setModelColumn(0); // back to normal

    // QAbstractItemView * QComboBox::view()
    // void QComboBox::setView(QAbstractItemView *)
    MyAbstractItemView *var13 = new MyAbstractItemView;
    obj1.setView(var13);
    QCOMPARE(obj1.view(), (QAbstractItemView *)var13);
    QTest::ignoreMessage(QtWarningMsg, "QComboBox::setView: cannot set a 0 view");
    obj1.setView((QAbstractItemView *)0);
    QCOMPARE(obj1.view(), (QAbstractItemView *)var13);
    delete var13;

    // int QComboBox::currentIndex()
    // void QComboBox::setCurrentIndex(int)
    obj1.setEditable(false);
    obj1.setCurrentIndex(0);
    QCOMPARE(-1, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MIN);
    QCOMPARE(-1, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MAX);
    QCOMPARE(-1, obj1.currentIndex());
    obj1.addItems(QStringList() << "1" << "2" << "3" << "4" << "5");
    obj1.setCurrentIndex(0);
    QCOMPARE(0, obj1.currentIndex()); // Valid
    obj1.setCurrentIndex(INT_MIN);
    QCOMPARE(-1, obj1.currentIndex()); // Invalid => -1
    obj1.setCurrentIndex(4);
    QCOMPARE(4, obj1.currentIndex()); // Valid
    obj1.setCurrentIndex(INT_MAX);
    QCOMPARE(-1, obj1.currentIndex()); // Invalid => -1
}

typedef QList<QVariant> VariantList;
typedef QList<QIcon> IconList;

Q_DECLARE_METATYPE(QComboBox::InsertPolicy)

class TestWidget : public QWidget
{
public:
    TestWidget() : QWidget(0, Qt::Window), m_comboBox(new QComboBox(this))
    {
        setObjectName("parent");
        move(200, 200);
        resize(400, 400);
        m_comboBox->setGeometry(0, 0, 100, 100);
        m_comboBox->setObjectName("testObject");
        m_comboBox->setEditable(false);
    }

    QComboBox *comboBox() const { return m_comboBox; }

private:
    QComboBox *m_comboBox;


};

void tst_QComboBox::setEditable()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    // make sure we have no lineedit
    QVERIFY(!testWidget->lineEdit());
    // test setEditable(true)
    testWidget->setEditable(true);
    QVERIFY(testWidget->lineEdit());
    testWidget->addItem("foo");
    QCOMPARE(testWidget->lineEdit()->text(), QString("foo"));
    // test setEditable(false)

    QLineEdit *lineEdit = testWidget->lineEdit();
    // line edit is visible when combobox is editable
    QVERIFY(lineEdit->isVisible());
    testWidget->setEditable(false);
    QVERIFY(!testWidget->lineEdit());
    // line edit should have been explicitly hidden when editable was turned off
    QVERIFY(!lineEdit->isVisible());
}


void tst_QComboBox::setPalette()
{
#ifdef Q_OS_MAC
    if (QApplication::style()->inherits("QMacStyle")) {
        QSKIP("This test doesn't make sense for pixmap-based styles");
    }
#endif
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    QPalette pal = testWidget->palette();
    pal.setColor(QPalette::Base, Qt::red);
    testWidget->setPalette(pal);
    testWidget->setEditable(!testWidget->isEditable());

    pal.setColor(QPalette::Base, Qt::blue);
    testWidget->setPalette(pal);

    const QObjectList comboChildren = testWidget->children();
    for (int i = 0; i < comboChildren.size(); ++i) {
        QObject *o = comboChildren.at(i);
        if (o->isWidgetType()) {
            QCOMPARE(((QWidget*)o)->palette(), pal);
        }
    }

    testWidget->setEditable(true);
    pal.setColor(QPalette::Base, Qt::red);
    //Setting it on the lineedit should be separate form the combo
    testWidget->lineEdit()->setPalette(pal);
    QVERIFY(testWidget->palette() != pal);
    QCOMPARE(testWidget->lineEdit()->palette(), pal);
    pal.setColor(QPalette::Base, Qt::green);
    //Setting it on the combo directly should override lineedit
    testWidget->setPalette(pal);
    QCOMPARE(testWidget->palette(), pal);
    QCOMPARE(testWidget->lineEdit()->palette(), pal);
}

void tst_QComboBox::sizeAdjustPolicy()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    // test that adding new items will not change the sizehint for AdjustToContentsOnFirstShow
    QVERIFY(!testWidget->count());
    QCOMPARE(testWidget->sizeAdjustPolicy(), QComboBox::AdjustToContentsOnFirstShow);
    QVERIFY(testWidget->isVisible());
    QSize firstShow = testWidget->sizeHint();
    testWidget->addItem("normal item");
    QCOMPARE(testWidget->sizeHint(), firstShow);

    // check that with minimumContentsLength/AdjustToMinimumContentsLength sizehint changes
    testWidget->setMinimumContentsLength(30);
    QCOMPARE(testWidget->sizeHint(), firstShow);
    testWidget->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    QSize minimumContentsLength = testWidget->sizeHint();
    QVERIFY(minimumContentsLength.width() > firstShow.width());
    testWidget->setMinimumContentsLength(60);
    QVERIFY(minimumContentsLength.width() < testWidget->sizeHint().width());

    // check that with minimumContentsLength/AdjustToMinimumContentsLengthWithIcon sizehint changes
    testWidget->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    testWidget->setMinimumContentsLength(30);
    minimumContentsLength = testWidget->sizeHint();
    QVERIFY(minimumContentsLength.width() > firstShow.width());
    testWidget->setMinimumContentsLength(60);
    QVERIFY(minimumContentsLength.width() < testWidget->sizeHint().width());
    minimumContentsLength = testWidget->sizeHint();
    testWidget->setIconSize(QSize(128,128));
    QVERIFY(minimumContentsLength.width() < testWidget->sizeHint().width());

    // check AdjustToContents changes with content
    testWidget->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    QSize content = testWidget->sizeHint();
    testWidget->addItem("small");
    QCOMPARE(testWidget->sizeHint(), content);
    testWidget->addItem("looooooooooooooooooooooong item");
    // minimumContentsLength() > sizeof("looooooooooooooooooooooong item"), so the sizeHint()
    // stays the same
    QCOMPARE(testWidget->sizeHint(), content);
    // over 60 characters (cf. setMinimumContentsLength() call above)
    testWidget->addItem("loooooooooooooooooooooooooooooooooooooooooooooo"
                        "ooooooooooooooooooooooooooooooooooooooooooooooo"
                        "ooooooooooooooooooooooooooooong item");
    QVERIFY(testWidget->sizeHint().width() > content.width());

    // check AdjustToContents also shrinks when item changes
    content = testWidget->sizeHint();
    for (int i=0; i<testWidget->count(); ++i)
        testWidget->setItemText(i, "XXXXXXXXXX");
    QVERIFY(testWidget->sizeHint().width() < content.width());

    // check AdjustToContents shrinks when items are removed
    content = testWidget->sizeHint();
    while (testWidget->count())
        testWidget->removeItem(0);
    QCOMPARE(testWidget->sizeHint(), content);
    testWidget->setMinimumContentsLength(0);
    QVERIFY(testWidget->sizeHint().width() < content.width());

    // check AdjustToContents changes when model changes
    content = testWidget->sizeHint();
    QStandardItemModel *model = new QStandardItemModel(2, 1, testWidget);
    testWidget->setModel(model);
    QVERIFY(testWidget->sizeHint().width() < content.width());

    // check AdjustToContents changes when a row is inserted into the model
    content = testWidget->sizeHint();
    QStandardItem *item = new QStandardItem(QStringLiteral("This is an item"));
    model->appendRow(item);
    QVERIFY(testWidget->sizeHint().width() > content.width());

    // check AdjustToContents changes when model is reset
    content = testWidget->sizeHint();
    model->clear();
    QVERIFY(testWidget->sizeHint().width() < content.width());
}

void tst_QComboBox::clear()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    // first non editable combobox
    testWidget->addItem("foo");
    testWidget->addItem("bar");
    QVERIFY(testWidget->count() > 0);
    QCOMPARE(testWidget->currentIndex(), 0);

    testWidget->clear();
    QCOMPARE(testWidget->count(), 0);
    QCOMPARE(testWidget->currentIndex(), -1);
    QVERIFY(testWidget->currentText().isEmpty());

    // then editable combobox
    testWidget->clear();
    testWidget->setEditable(true);
    testWidget->addItem("foo");
    testWidget->addItem("bar");
    QVERIFY(testWidget->count() > 0);
    QCOMPARE(testWidget->currentIndex(), 0);
    QVERIFY(testWidget->lineEdit());
    QVERIFY(!testWidget->lineEdit()->text().isEmpty());
    testWidget->clear();
    QCOMPARE(testWidget->count(), 0);
    QCOMPARE(testWidget->currentIndex(), -1);
    QVERIFY(testWidget->currentText().isEmpty());
    QVERIFY(testWidget->lineEdit()->text().isEmpty());
}

void tst_QComboBox::insertPolicy_data()
{
    QTest::addColumn<QStringList>("initialEntries");
    QTest::addColumn<QComboBox::InsertPolicy>("insertPolicy");
    QTest::addColumn<int>("currentIndex");
    QTest::addColumn<QString>("userInput");
    QTest::addColumn<QStringList>("result");

    /* Each insertPolicy should test at least:
       no initial entries
       one initial entry
       five initial entries, current is first item
       five initial entries, current is third item
       five initial entries, current is last item
    */

    /* QComboBox::NoInsert - the string will not be inserted into the combobox.
       QComboBox::InsertAtTop - insert the string as the first item in the combobox.
       QComboBox::InsertAtCurrent - replace the previously selected item with the string the user has entered.
       QComboBox::InsertAtBottom - insert the string as the last item in the combobox.
       QComboBox::InsertAfterCurrent - insert the string after the previously selected item.
       QComboBox::InsertBeforeCurrent - insert the string before the previously selected item.
       QComboBox::InsertAlphabetically - insert the string at the alphabetic position.
    */
    QStringList initial;
    QStringList oneEntry("One");
    QStringList fiveEntries;
    fiveEntries << "One" << "Two" << "Three" << "Four" << "Five";
    QString input("insert");

    {
        QTest::newRow("NoInsert-NoInitial") << initial << QComboBox::NoInsert << 0 << input << initial;
        QTest::newRow("NoInsert-OneInitial") << oneEntry << QComboBox::NoInsert << 0 << input << oneEntry;
        QTest::newRow("NoInsert-FiveInitial-FirstCurrent") << fiveEntries << QComboBox::NoInsert << 0 << input << fiveEntries;
        QTest::newRow("NoInsert-FiveInitial-ThirdCurrent") << fiveEntries << QComboBox::NoInsert << 2 << input << fiveEntries;
        QTest::newRow("NoInsert-FiveInitial-LastCurrent") << fiveEntries << QComboBox::NoInsert << 4 << input << fiveEntries;
    }

    {
        QStringList initialAtTop("insert");
        QStringList oneAtTop;
        oneAtTop << "insert" << "One";
        QStringList fiveAtTop;
        fiveAtTop << "insert" << "One" << "Two" << "Three" << "Four" << "Five";

        QTest::newRow("AtTop-NoInitial") << initial << QComboBox::InsertAtTop << 0 << input << initialAtTop;
        QTest::newRow("AtTop-OneInitial") << oneEntry << QComboBox::InsertAtTop << 0 << input << oneAtTop;
        QTest::newRow("AtTop-FiveInitial-FirstCurrent") << fiveEntries << QComboBox::InsertAtTop << 0 << input << fiveAtTop;
        QTest::newRow("AtTop-FiveInitial-ThirdCurrent") << fiveEntries << QComboBox::InsertAtTop << 2 << input << fiveAtTop;
        QTest::newRow("AtTop-FiveInitial-LastCurrent") << fiveEntries << QComboBox::InsertAtTop << 4 << input << fiveAtTop;
    }

    {
        QStringList initialAtCurrent("insert");
        QStringList oneAtCurrent("insert");
        QStringList fiveAtCurrentFirst;
        fiveAtCurrentFirst << "insert" << "Two" << "Three" << "Four" << "Five";
        QStringList fiveAtCurrentThird;
        fiveAtCurrentThird << "One" << "Two" << "insert" << "Four" << "Five";
        QStringList fiveAtCurrentLast;
        fiveAtCurrentLast << "One" << "Two" << "Three" << "Four" << "insert";

        QTest::newRow("AtCurrent-NoInitial") << initial << QComboBox::InsertAtCurrent << 0 << input << initialAtCurrent;
        QTest::newRow("AtCurrent-OneInitial") << oneEntry << QComboBox::InsertAtCurrent << 0 << input << oneAtCurrent;
        QTest::newRow("AtCurrent-FiveInitial-FirstCurrent") << fiveEntries << QComboBox::InsertAtCurrent << 0 << input << fiveAtCurrentFirst;
        QTest::newRow("AtCurrent-FiveInitial-ThirdCurrent") << fiveEntries << QComboBox::InsertAtCurrent << 2 << input << fiveAtCurrentThird;
        QTest::newRow("AtCurrent-FiveInitial-LastCurrent") << fiveEntries << QComboBox::InsertAtCurrent << 4 << input << fiveAtCurrentLast;
    }

    {
        QStringList initialAtBottom("insert");
        QStringList oneAtBottom;
        oneAtBottom << "One" << "insert";
        QStringList fiveAtBottom;
        fiveAtBottom << "One" << "Two" << "Three" << "Four" << "Five" << "insert";

        QTest::newRow("AtBottom-NoInitial") << initial << QComboBox::InsertAtBottom << 0 << input << initialAtBottom;
        QTest::newRow("AtBottom-OneInitial") << oneEntry << QComboBox::InsertAtBottom << 0 << input << oneAtBottom;
        QTest::newRow("AtBottom-FiveInitial-FirstCurrent") << fiveEntries << QComboBox::InsertAtBottom << 0 << input << fiveAtBottom;
        QTest::newRow("AtBottom-FiveInitial-ThirdCurrent") << fiveEntries << QComboBox::InsertAtBottom << 2 << input << fiveAtBottom;
        QTest::newRow("AtBottom-FiveInitial-LastCurrent") << fiveEntries << QComboBox::InsertAtBottom << 4 << input << fiveAtBottom;
    }

    {
        QStringList initialAfterCurrent("insert");
        QStringList oneAfterCurrent;
        oneAfterCurrent << "One" << "insert";
        QStringList fiveAfterCurrentFirst;
        fiveAfterCurrentFirst << "One" << "insert" << "Two" << "Three" << "Four" << "Five";
        QStringList fiveAfterCurrentThird;
        fiveAfterCurrentThird << "One" << "Two" << "Three" << "insert" << "Four" << "Five";
        QStringList fiveAfterCurrentLast;
        fiveAfterCurrentLast << "One" << "Two" << "Three" << "Four" << "Five" << "insert";

        QTest::newRow("AfterCurrent-NoInitial") << initial << QComboBox::InsertAfterCurrent << 0 << input << initialAfterCurrent;
        QTest::newRow("AfterCurrent-OneInitial") << oneEntry << QComboBox::InsertAfterCurrent << 0 << input << oneAfterCurrent;
        QTest::newRow("AfterCurrent-FiveInitial-FirstCurrent") << fiveEntries << QComboBox::InsertAfterCurrent << 0 << input << fiveAfterCurrentFirst;
        QTest::newRow("AfterCurrent-FiveInitial-ThirdCurrent") << fiveEntries << QComboBox::InsertAfterCurrent << 2 << input << fiveAfterCurrentThird;
        QTest::newRow("AfterCurrent-FiveInitial-LastCurrent") << fiveEntries << QComboBox::InsertAfterCurrent << 4 << input << fiveAfterCurrentLast;
    }

    {
        QStringList initialBeforeCurrent("insert");
        QStringList oneBeforeCurrent;
        oneBeforeCurrent << "insert" << "One";
        QStringList fiveBeforeCurrentFirst;
        fiveBeforeCurrentFirst << "insert" << "One" << "Two" << "Three" << "Four" << "Five";
        QStringList fiveBeforeCurrentThird;
        fiveBeforeCurrentThird << "One" << "Two" << "insert" << "Three" << "Four" << "Five";
        QStringList fiveBeforeCurrentLast;
        fiveBeforeCurrentLast << "One" << "Two" << "Three" << "Four" << "insert" << "Five";

        QTest::newRow("BeforeCurrent-NoInitial") << initial << QComboBox::InsertBeforeCurrent << 0 << input << initialBeforeCurrent;
        QTest::newRow("BeforeCurrent-OneInitial") << oneEntry << QComboBox::InsertBeforeCurrent << 0 << input << oneBeforeCurrent;
        QTest::newRow("BeforeCurrent-FiveInitial-FirstCurrent") << fiveEntries << QComboBox::InsertBeforeCurrent << 0 << input << fiveBeforeCurrentFirst;
        QTest::newRow("BeforeCurrent-FiveInitial-ThirdCurrent") << fiveEntries << QComboBox::InsertBeforeCurrent << 2 << input << fiveBeforeCurrentThird;
        QTest::newRow("BeforeCurrent-FiveInitial-LastCurrent") << fiveEntries << QComboBox::InsertBeforeCurrent << 4 << input << fiveBeforeCurrentLast;
    }

    {
        oneEntry.clear();
        oneEntry << "foobar";
        fiveEntries.clear();
        fiveEntries << "bar" << "foo" << "initial" << "Item" << "stamp";

        QStringList initialAlphabetically("insert");
        QStringList oneAlphabetically;
        oneAlphabetically << "foobar" << "insert";
        QStringList fiveAlphabetically;
        fiveAlphabetically << "bar" << "foo" << "initial" << "insert" << "Item" << "stamp";

        QTest::newRow("Alphabetically-NoInitial") << initial << QComboBox::InsertAlphabetically << 0 << input << initialAlphabetically;
        QTest::newRow("Alphabetically-OneInitial") << oneEntry << QComboBox::InsertAlphabetically << 0 << input << oneAlphabetically;
        QTest::newRow("Alphabetically-FiveInitial-FirstCurrent") << fiveEntries << QComboBox::InsertAlphabetically << 0 << input << fiveAlphabetically;
        QTest::newRow("Alphabetically-FiveInitial-ThirdCurrent") << fiveEntries << QComboBox::InsertAlphabetically << 2 << input << fiveAlphabetically;
        QTest::newRow("Alphabetically-FiveInitial-LastCurrent") << fiveEntries << QComboBox::InsertAlphabetically << 4 << input << fiveAlphabetically;
    }
}

void tst_QComboBox::insertPolicy()
{
    QFETCH(QStringList, initialEntries);
    QFETCH(QComboBox::InsertPolicy, insertPolicy);
    QFETCH(int, currentIndex);
    QFETCH(QString, userInput);
    QFETCH(QStringList, result);

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    testWidget->clear();
    testWidget->setInsertPolicy(insertPolicy);
    testWidget->addItems(initialEntries);
    testWidget->setEditable(true);
    if (initialEntries.count() > 0)
        testWidget->setCurrentIndex(currentIndex);

    // clear
    QTest::mouseDClick(testWidget->lineEdit(), Qt::LeftButton);
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Delete);

    QTest::keyClicks(testWidget->lineEdit(), userInput);
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Return);

    // First check that there is the right number of entries, or
    // we may unwittingly pass
    QCOMPARE((int)result.count(), testWidget->count());

    // No need to compare if there are no strings to compare
    if (result.count() > 0) {
        for (int i=0; i<testWidget->count(); ++i) {
            QCOMPARE(testWidget->itemText(i), result.at(i));
        }
    }
}

// Apps running with valgrind are not fast enough.
void tst_QComboBox::virtualAutocompletion()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    testWidget->clear();
    testWidget->setAutoCompletion(true);
    testWidget->addItem("Foo");
    testWidget->addItem("Bar");
    testWidget->addItem("Boat");
    testWidget->addItem("Boost");
    testWidget->clearEditText();

    // We need to set the keyboard input interval to a higher value
    // as the processEvent() call takes too much time, so it restarts
    // the keyboard search then
#if defined(Q_PROCESSOR_ARM) || defined(Q_PROCESSOR_MIPS)
    int oldInterval = QApplication::keyboardInputInterval();
    QApplication::setKeyboardInputInterval(1500);
#endif

    // NOTE:
    // Cannot use keyClick for this test, as it simulates keyclicks too well
    // The virtual keyboards we're trying to catch here, do not perform that
    // well, and send a keypress & keyrelease right after each other.
    // This provokes the actual error, as there's no events in between to do
    // the text completion.
    QKeyEvent kp1(QEvent::KeyPress, Qt::Key_B, 0, "b");
    QKeyEvent kr1(QEvent::KeyRelease, Qt::Key_B, 0, "b");
    QApplication::sendEvent(testWidget, &kp1);
    QApplication::sendEvent(testWidget, &kr1);

    qApp->processEvents(); // Process events to trigger autocompletion
    QTRY_COMPARE(testWidget->currentIndex(), 1);

    QKeyEvent kp2(QEvent::KeyPress, Qt::Key_O, 0, "o");
    QKeyEvent kr2(QEvent::KeyRelease, Qt::Key_O, 0, "o");

    QApplication::sendEvent(testWidget, &kp2);
    QApplication::sendEvent(testWidget, &kr2);

    qApp->processEvents(); // Process events to trigger autocompletion
    QTRY_COMPARE(testWidget->currentIndex(), 2);

    QApplication::sendEvent(testWidget, &kp2);
    QApplication::sendEvent(testWidget, &kr2);
    qApp->processEvents(); // Process events to trigger autocompletion
    QTRY_COMPARE(testWidget->currentIndex(), 3);
#if defined(Q_PROCESSOR_ARM) || defined(Q_PROCESSOR_MIPS)
    QApplication::setKeyboardInputInterval(oldInterval);
#endif
}

void tst_QComboBox::autoCompletionCaseSensitivity()
{
    //we have put the focus because the completer
    //is only used when the widget actually has the focus
    TestWidget topLevel;
    topLevel.show();
    QComboBox *testWidget = topLevel.comboBox();
    qApp->setActiveWindow(&topLevel);
    testWidget->setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));
    QCOMPARE(qApp->focusWidget(), (QWidget *)testWidget);

    testWidget->clear();
    testWidget->setAutoCompletion(true);
    testWidget->addItem("Cow");
    testWidget->addItem("irrelevant1");
    testWidget->addItem("aww");
    testWidget->addItem("A*");
    testWidget->addItem("irrelevant2");
    testWidget->addItem("aBCDEF");
    testWidget->addItem("irrelevant3");
    testWidget->addItem("abcdef");
    testWidget->addItem("abCdef");
    testWidget->setEditable(true);

    // case insensitive
    testWidget->clearEditText();
    QSignalSpy spyReturn(testWidget, SIGNAL(activated(int)));
    testWidget->setAutoCompletionCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(testWidget->autoCompletionCaseSensitivity(), Qt::CaseInsensitive);

    QTest::keyClick(testWidget->lineEdit(), Qt::Key_A);
    qApp->processEvents();
    QCOMPARE(testWidget->currentText(), QString("aww"));
    QCOMPARE(spyReturn.count(), 0);

    QTest::keyClick(testWidget->lineEdit(), Qt::Key_B);
    qApp->processEvents();
    // autocompletions preserve userkey-case from 4.2
    QCOMPARE(testWidget->currentText(), QString("abCDEF"));
    QCOMPARE(spyReturn.count(), 0);

    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Enter);
    qApp->processEvents();
    QCOMPARE(testWidget->currentText(), QString("aBCDEF")); // case restored to item's case
    QCOMPARE(spyReturn.count(), 1);

    testWidget->clearEditText();
    QTest::keyClick(testWidget->lineEdit(), 'c');
    QCOMPARE(testWidget->currentText(), QString("cow"));
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Enter);
    QCOMPARE(testWidget->currentText(), QString("Cow")); // case restored to item's case

    testWidget->clearEditText();
    QTest::keyClick(testWidget->lineEdit(), 'a');
    QTest::keyClick(testWidget->lineEdit(), '*');
    QCOMPARE(testWidget->currentText(), QString("a*"));
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Enter);
    QCOMPARE(testWidget->currentText(), QString("A*"));

    // case sensitive
    testWidget->clearEditText();
    testWidget->setAutoCompletionCaseSensitivity(Qt::CaseSensitive);
    QCOMPARE(testWidget->autoCompletionCaseSensitivity(), Qt::CaseSensitive);
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_A);
    qApp->processEvents();
    QCOMPARE(testWidget->currentText(), QString("aww"));
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_B);
    qApp->processEvents();
    QCOMPARE(testWidget->currentText(), QString("abcdef"));

    testWidget->setCurrentIndex(0); // to reset the completion's "start"
    testWidget->clearEditText();
    QTest::keyClick(testWidget->lineEdit(), 'a');
    QTest::keyClick(testWidget->lineEdit(), 'b');
    QCOMPARE(testWidget->currentText(), QString("abcdef"));
    QTest::keyClick(testWidget->lineEdit(), 'C');
    qApp->processEvents();
    QCOMPARE(testWidget->currentText(), QString("abCdef"));
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Enter);
    qApp->processEvents();
    QCOMPARE(testWidget->currentText(), QString("abCdef")); // case restored to item's case

    testWidget->clearEditText();
    QTest::keyClick(testWidget->lineEdit(), 'c');
    QCOMPARE(testWidget->currentText(), QString("c"));
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Backspace);
    QTest::keyClick(testWidget->lineEdit(), 'C');
    QCOMPARE(testWidget->currentText(), QString("Cow"));
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Enter);
    QCOMPARE(testWidget->currentText(), QString("Cow"));

    testWidget->clearEditText();
    QTest::keyClick(testWidget->lineEdit(), 'a');
    QTest::keyClick(testWidget->lineEdit(), '*');
    QCOMPARE(testWidget->currentText(), QString("a*"));
    QTest::keyClick(testWidget->lineEdit(), Qt::Key_Enter);
    QCOMPARE(testWidget->currentText(), QString("a*")); // A* not matched
}

void tst_QComboBox::hide()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    testWidget->addItem("foo");
    testWidget->showPopup();
    //allow combobox effect to complete
    QTRY_VERIFY(testWidget->view());
    QTRY_VERIFY(testWidget->view()->isVisible());
    testWidget->hidePopup();
    //allow combobox effect to complete
    QTRY_VERIFY(!testWidget->view()->isVisible());
    testWidget->hide();
    QVERIFY(!testWidget->isVisible());
}



void tst_QComboBox::currentIndex_data()
{
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<int>("setCurrentIndex");
    QTest::addColumn<int>("removeIndex");
    QTest::addColumn<int>("insertIndex");
    QTest::addColumn<QString>("insertText");
    QTest::addColumn<int>("expectedCurrentIndex");
    QTest::addColumn<QString>("expectedCurrentText");
    QTest::addColumn<int>("expectedSignalCount");

    QStringList initialItems;
    int setCurrentIndex;
    int removeIndex;
    int insertIndex;
    QString insertText;
    int expectedCurrentIndex;
    QString expectedCurrentText;
    int expectedSignalCount;

    {
        initialItems.clear();
        initialItems << "foo" << "bar";
        setCurrentIndex = -2;
        removeIndex = -1;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = 0;
        expectedCurrentText = "foo";
        expectedSignalCount = 1;
        QTest::newRow("first added item is set to current if there is no current")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
    {
        initialItems.clear();
        initialItems << "foo" << "bar";
        setCurrentIndex = 1;
        removeIndex = -1;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = 1;
        expectedCurrentText = "bar";
        expectedSignalCount = 2;
        QTest::newRow("check that setting the index works")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;

    }
    {
        initialItems.clear();
        initialItems << "foo" << "bar";
        setCurrentIndex = -1; // will invalidate the currentIndex
        removeIndex = -1;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = -1;
        expectedCurrentText = "";
        expectedSignalCount = 2;
        QTest::newRow("check that isetting the index to -1 works")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;

    }
    {
        initialItems.clear();
        initialItems << "foo";
        setCurrentIndex = 0;
        removeIndex = 0;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = -1;
        expectedCurrentText = "";
        expectedSignalCount = 2;
        QTest::newRow("check that current index is invalid when removing the only item")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
    {
        initialItems.clear();
        initialItems << "foo" << "bar";
        setCurrentIndex = 1;
        removeIndex = 0;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = 0;
        expectedCurrentText = "bar";
        expectedSignalCount = 3;
        QTest::newRow("check that the current index follows the item when removing an item above")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;

    }
    {
        initialItems.clear();
        initialItems << "foo" << "bar" << "baz";
        setCurrentIndex = 1;
        removeIndex = 1;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = 1;
        expectedCurrentText = "baz";
        expectedSignalCount = 3;
        QTest::newRow("check that the current index uses the next item if current is removed")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
    {
        initialItems.clear();
        initialItems << "foo" << "bar" << "baz";
        setCurrentIndex = 2;
        removeIndex = 2;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = 1;
        expectedCurrentText = "bar";
        expectedSignalCount = 3;
        QTest::newRow("check that the current index is moved to the one before if current is removed")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
    {
        initialItems.clear();
        initialItems << "foo" << "bar" << "baz";
        setCurrentIndex = 1;
        removeIndex = 2;
        insertIndex = -1;
        insertText = "";
        expectedCurrentIndex = 1;
        expectedCurrentText = "bar";
        expectedSignalCount = 2;
        QTest::newRow("check that the current index is unchanged if you remove an item after")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
    {
        initialItems.clear();
        initialItems << "foo" << "bar";
        setCurrentIndex = 1;
        removeIndex = -1;
        insertIndex = 0;
        insertText = "baz";
        expectedCurrentIndex = 2;
        expectedCurrentText = "bar";
        expectedSignalCount = 3;
        QTest::newRow("check that the current index follows the item if you insert before current")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
    {
        initialItems.clear();
        initialItems << "foo";
        setCurrentIndex = 0;
        removeIndex = -1;
        insertIndex = 0;
        insertText = "bar";
        expectedCurrentIndex = 1;
        expectedCurrentText = "foo";
        expectedSignalCount = 2;
        QTest::newRow("check that the current index follows the item if you insert on the current")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
    {
        initialItems.clear();
        initialItems << "foo";
        setCurrentIndex = 0;
        removeIndex = -1;
        insertIndex = 1;
        insertText = "bar";
        expectedCurrentIndex = 0;
        expectedCurrentText = "foo";
        expectedSignalCount = 1;
        QTest::newRow("check that the current index stays the same if you insert after the current")
            << initialItems << setCurrentIndex << removeIndex
            << insertIndex << insertText << expectedCurrentIndex << expectedCurrentText
            << expectedSignalCount;
    }
}

void tst_QComboBox::currentIndex()
{
    QFETCH(QStringList, initialItems);
    QFETCH(int, setCurrentIndex);
    QFETCH(int, removeIndex);
    QFETCH(int, insertIndex);
    QFETCH(QString, insertText);
    QFETCH(int, expectedCurrentIndex);
    QFETCH(QString, expectedCurrentText);
    QFETCH(int, expectedSignalCount);

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    // test both editable/non-editable combobox
    for (int edit = 0; edit < 2; ++edit) {
        testWidget->clear();
        testWidget->setEditable(edit ? true : false);
        if (edit)
            QVERIFY(testWidget->lineEdit());

        // verify it is empty, has no current index and no current text
        QCOMPARE(testWidget->count(), 0);
        QCOMPARE(testWidget->currentIndex(), -1);
        QVERIFY(testWidget->currentText().isEmpty());

        // spy on currentIndexChanged
        QSignalSpy indexChangedInt(testWidget, SIGNAL(currentIndexChanged(int)));
        QSignalSpy indexChangedString(testWidget, SIGNAL(currentIndexChanged(QString)));

        // stuff items into it
        foreach(QString text, initialItems) {
            testWidget->addItem(text);
        }
        QCOMPARE(testWidget->count(), initialItems.count());

        // set current index, remove and/or insert
        if (setCurrentIndex >= -1) {
            testWidget->setCurrentIndex(setCurrentIndex);
            QCOMPARE(testWidget->currentIndex(), setCurrentIndex);
        }

        if (removeIndex >= 0)
            testWidget->removeItem(removeIndex);
        if (insertIndex >= 0)
            testWidget->insertItem(insertIndex, insertText);

        // compare with expected index and text
        QCOMPARE(testWidget->currentIndex(), expectedCurrentIndex);
        QCOMPARE(testWidget->currentText(), expectedCurrentText);

        // check that signal count is correct
        QCOMPARE(indexChangedInt.count(), expectedSignalCount);
        QCOMPARE(indexChangedString.count(), expectedSignalCount);

        // compare with last sent signal values
        if (indexChangedInt.count())
            QCOMPARE(indexChangedInt.at(indexChangedInt.count() - 1).at(0).toInt(),
                    testWidget->currentIndex());
        if (indexChangedString.count())
            QCOMPARE(indexChangedString.at(indexChangedString.count() - 1).at(0).toString(),
                     testWidget->currentText());

        if (edit) {
            testWidget->setCurrentIndex(-1);
            testWidget->setInsertPolicy(QComboBox::InsertAtBottom);
            QTest::keyPress(testWidget, 'a');
            QTest::keyPress(testWidget, 'b');
            QCOMPARE(testWidget->currentText(), QString("ab"));
            QCOMPARE(testWidget->currentIndex(), -1);
            int numItems = testWidget->count();
            QTest::keyPress(testWidget, Qt::Key_Return);
            QCOMPARE(testWidget->count(), numItems + 1);
            QCOMPARE(testWidget->currentIndex(), numItems);
            testWidget->setCurrentIndex(-1);
            QTest::keyPress(testWidget, 'a');
            QTest::keyPress(testWidget, 'b');
            QCOMPARE(testWidget->currentIndex(), -1);
        }
    }
}

void tst_QComboBox::insertItems_data()
{
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<QStringList>("insertedItems");
    QTest::addColumn<int>("insertIndex");
    QTest::addColumn<int>("expectedIndex");

    QStringList initialItems;
    QStringList insertedItems;

    initialItems << "foo" << "bar";
    insertedItems << "mongo";

    QTest::newRow("prepend") << initialItems << insertedItems << 0 << 0;
    QTest::newRow("prepend with negative value") << initialItems << insertedItems << -1 << 0;
    QTest::newRow("append") << initialItems << insertedItems << initialItems.count() << initialItems.count();
    QTest::newRow("append with too high value") << initialItems << insertedItems << 999 << initialItems.count();
    QTest::newRow("insert") << initialItems << insertedItems << 1 << 1;
}

void tst_QComboBox::insertItems()
{
    QFETCH(QStringList, initialItems);
    QFETCH(QStringList, insertedItems);
    QFETCH(int, insertIndex);
    QFETCH(int, expectedIndex);

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    testWidget->insertItems(0, initialItems);
    QCOMPARE(testWidget->count(), initialItems.count());

    testWidget->insertItems(insertIndex, insertedItems);

    QCOMPARE(testWidget->count(), initialItems.count() + insertedItems.count());
    for (int i=0; i<insertedItems.count(); ++i)
        QCOMPARE(testWidget->itemText(expectedIndex + i), insertedItems.at(i));
}

void tst_QComboBox::insertItem_data()
{
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<int>("insertIndex");
    QTest::addColumn<QString>("itemLabel");
    QTest::addColumn<int>("expectedIndex");
    QTest::addColumn<bool>("editable");

    QStringList initialItems;
    initialItems << "foo" << "bar";
    for(int e = 0 ; e<2 ; e++) {
        bool editable = (e==0);
        QTest::newRow("Insert less then 0") << initialItems << -1 << "inserted" << 0 << editable;
        QTest::newRow("Insert at 0") << initialItems << 0 << "inserted" << 0 << editable;
        QTest::newRow("Insert beyond count") << initialItems << 3 << "inserted" << 2 << editable;
        QTest::newRow("Insert at count") << initialItems << 2 << "inserted" << 2 << editable;
        QTest::newRow("Insert in the middle") << initialItems << 1 << "inserted" << 1 << editable;
    }
}

void tst_QComboBox::insertItem()
{
    QFETCH(QStringList, initialItems);
    QFETCH(int, insertIndex);
    QFETCH(QString, itemLabel);
    QFETCH(int, expectedIndex);
    QFETCH(bool, editable);

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    testWidget->insertItems(0, initialItems);
    QCOMPARE(testWidget->count(), initialItems.count());

    testWidget->setEditable(true);
    if (editable)
        testWidget->setEditText("FOO");
    testWidget->insertItem(insertIndex, itemLabel);

    QCOMPARE(testWidget->count(), initialItems.count() + 1);
    QCOMPARE(testWidget->itemText(expectedIndex), itemLabel);

    if (editable)
        QCOMPARE(testWidget->currentText(), QString("FOO"));
}

void tst_QComboBox::insertOnCurrentIndex()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    testWidget->setEditable(true);
    testWidget->addItem("first item");
    testWidget->setCurrentIndex(0);
    testWidget->insertItem(0, "second item");
    QCOMPARE(testWidget->lineEdit()->text(), QString::fromLatin1("first item"));
}

void tst_QComboBox::textpixmapdata_data()
{
    QTest::addColumn<QStringList>("text");
    QTest::addColumn<IconList>("icons");
    QTest::addColumn<VariantList>("variant");

    QStringList text;
    IconList icon;
    VariantList variant;
    QString qtlogoPath = QFINDTESTDATA("qtlogo.png");
    QString qtlogoinvertedPath = QFINDTESTDATA("qtlogoinverted.png");

    {
        text.clear(); icon.clear(); variant.clear();
        text << "foo" << "bar";
        icon << QIcon() << QIcon();
        variant << QVariant() << QVariant();
        QTest::newRow("just text") << text << icon << variant;
    }
    {
        text.clear(); icon.clear(); variant.clear();
        text << QString() << QString();
        icon << QIcon(QPixmap(qtlogoPath)) << QIcon(QPixmap(qtlogoinvertedPath));
        variant << QVariant() << QVariant();
        QTest::newRow("just icons") << text << icon << variant;
    }
    {
        text.clear(); icon.clear(); variant.clear();
        text << QString() << QString();
        icon << QIcon() << QIcon();
        variant << 12 << "bingo";
        QTest::newRow("just user data") << text << icon << variant;
    }
    {
        text.clear(); icon.clear(); variant.clear();
        text << "foo" << "bar";
        icon << QIcon(QPixmap(qtlogoPath)) << QIcon(QPixmap(qtlogoinvertedPath));
        variant << 12 << "bingo";
        QTest::newRow("text, icons and user data") << text << icon << variant;
    }
}

void tst_QComboBox::textpixmapdata()
{
    QFETCH(QStringList, text);
    QFETCH(IconList, icons);
    QFETCH(VariantList, variant);

    QVERIFY(text.count() == icons.count() && text.count() == variant.count());

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    for (int i = 0; i<text.count(); ++i) {
        testWidget->insertItem(i, text.at(i));
        testWidget->setItemIcon(i, icons.at(i));
        testWidget->setItemData(i, variant.at(i), Qt::UserRole);
    }

    QCOMPARE(testWidget->count(), text.count());

    for (int i = 0; i<text.count(); ++i) {
        QIcon icon = testWidget->itemIcon(i);
        QCOMPARE(icon.cacheKey(), icons.at(i).cacheKey());
        QPixmap original = icons.at(i).pixmap(1024);
        QPixmap pixmap = icon.pixmap(1024);
        QCOMPARE(pixmap.toImage(), original.toImage());
    }

    for (int i = 0; i<text.count(); ++i) {
        QCOMPARE(testWidget->itemText(i), text.at(i));
        // ### we should test icons/pixmap as well, but I need to fix the api mismatch first
        QCOMPARE(testWidget->itemData(i, Qt::UserRole), variant.at(i));
    }
}

void tst_QComboBox::setCurrentIndex()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    QCOMPARE(testWidget->count(), 0);
    testWidget->addItem("foo");
    testWidget->addItem("bar");
    QCOMPARE(testWidget->count(), 2);

    QCOMPARE(testWidget->currentIndex(), 0);
    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentText(), QString("foo"));

    testWidget->setCurrentIndex(1);
    QCOMPARE(testWidget->currentText(), QString("bar"));

    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentText(), QString("foo"));
}

void tst_QComboBox::setCurrentText_data()
{
    QTest::addColumn<bool>("editable");
    QTest::newRow("editable") << true;
    QTest::newRow("not editable") << false;
}

void tst_QComboBox::setCurrentText()
{
    QFETCH(bool, editable);

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    QCOMPARE(testWidget->count(), 0);
    testWidget->addItems(QStringList() << "foo" << "bar");
    QCOMPARE(testWidget->count(), 2);

    testWidget->setEditable(editable);
    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentIndex(), 0);

    // effect on currentText and currentIndex
    // currentIndex not changed if editable
    QCOMPARE(testWidget->currentText(), QString("foo"));
    testWidget->setCurrentText(QString("bar"));
    QCOMPARE(testWidget->currentText(), QString("bar"));
    if (editable)
        QCOMPARE(testWidget->currentIndex(), 0);
    else
        QCOMPARE(testWidget->currentIndex(), 1);

    testWidget->setCurrentText(QString("foo"));
    QCOMPARE(testWidget->currentIndex(), 0);
    QCOMPARE(testWidget->currentText(), QString("foo"));

    // effect of text not found in list
    testWidget->setCurrentText(QString("qt"));
    QCOMPARE(testWidget->currentIndex(), 0);
    if (editable)
        QCOMPARE(testWidget->currentText(), QString("qt"));
    else
        QCOMPARE(testWidget->currentText(), QString("foo"));

#ifndef QT_NO_PROPERTIES
    // verify WRITE for currentText property
    testWidget->setCurrentIndex(0);
    const QByteArray n("currentText");
    QCOMPARE(testWidget->property(n).toString(), QString("foo"));
    testWidget->setProperty(n, QString("bar"));
    QCOMPARE(testWidget->property(n).toString(), QString("bar"));
#endif
}

void tst_QComboBox::currentTextChanged_data()
{
    QTest::addColumn<bool>("editable");
    QTest::newRow("editable") << true;
    QTest::newRow("not editable") << false;
}

void tst_QComboBox::currentTextChanged()
{
    QFETCH(bool, editable);

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    QCOMPARE(testWidget->count(), 0);
    testWidget->addItems(QStringList() << "foo" << "bar");
    QCOMPARE(testWidget->count(), 2);

    QSignalSpy spy(testWidget, SIGNAL(currentTextChanged(QString)));

    testWidget->setEditable(editable);

    // set text in list
    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentIndex(), 0);
    spy.clear();
    testWidget->setCurrentText(QString("bar"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<QString>(spy.at(0).at(0)), QString("bar"));

    // set text not in list
    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentIndex(), 0);
    spy.clear();
    testWidget->setCurrentText(QString("qt"));
    if (editable) {
        QCOMPARE(spy.count(), 1);
        QCOMPARE(qvariant_cast<QString>(spy.at(0).at(0)), QString("qt"));
    } else {
        QCOMPARE(spy.count(), 0);
    }

    // item changed
    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentIndex(), 0);
    spy.clear();
    testWidget->setItemText(0, QString("ape"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<QString>(spy.at(0).at(0)), QString("ape"));
    // change it back
    spy.clear();
    testWidget->setItemText(0, QString("foo"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<QString>(spy.at(0).at(0)), QString("foo"));
}

void tst_QComboBox::editTextChanged()
{
    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    QCOMPARE(testWidget->count(), 0);
    testWidget->addItem("foo");
    testWidget->addItem("bar");
    QCOMPARE(testWidget->count(), 2);

    // first we test non editable
    testWidget->setEditable(false);
    QCOMPARE(testWidget->isEditable(), false);

    QSignalSpy spy(testWidget, SIGNAL(editTextChanged(QString)));

    // no signal should be sent when current is set to the same
    QCOMPARE(testWidget->currentIndex(), 0);
    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentIndex(), 0);
    QCOMPARE(spy.count(), 0);

    // no signal should be sent when changing to other index because we are not editable
    QCOMPARE(testWidget->currentIndex(), 0);
    testWidget->setCurrentIndex(1);
    QCOMPARE(testWidget->currentIndex(), 1);
    QCOMPARE(spy.count(), 0);

    // now set to editable and reset current index
    testWidget->setEditable(true);
    QCOMPARE(testWidget->isEditable(), true);
    testWidget->setCurrentIndex(0);

    // no signal should be sent when current is set to the same
    spy.clear();
    QCOMPARE(testWidget->currentIndex(), 0);
    testWidget->setCurrentIndex(0);
    QCOMPARE(testWidget->currentIndex(), 0);
    QCOMPARE(spy.count(), 0);

    // signal should be sent when changing to other index
    QCOMPARE(testWidget->currentIndex(), 0);
    testWidget->setCurrentIndex(1);
    QCOMPARE(testWidget->currentIndex(), 1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<QString>(spy.at(0).at(0)), QString("bar"));


    // insert some keys and notice they are all signaled
    spy.clear();
    QTest::keyClicks(testWidget, "bingo");
    QCOMPARE(spy.count(), 5);
    QCOMPARE(qvariant_cast<QString>(spy.at(4).at(0)), QString("barbingo"));
}

void tst_QComboBox::setModel()
{
    QComboBox box;
    QCOMPARE(box.currentIndex(), -1);
    box.addItems((QStringList() << "foo" << "bar"));
    QCOMPARE(box.currentIndex(), 0);
    box.setCurrentIndex(1);
    QCOMPARE(box.currentIndex(), 1);

    // check that currentIndex is set to invalid
    QAbstractItemModel *oldModel = box.model();
    box.setModel(new QStandardItemModel(&box));
    QCOMPARE(box.currentIndex(), -1);
    QVERIFY(box.model() != oldModel);

    // check that currentIndex is set to first item
    oldModel = box.model();
    box.setModel(new QStandardItemModel(2,1, &box));
    QCOMPARE(box.currentIndex(), 0);
    QVERIFY(box.model() != oldModel);

    // set a new root index
    QModelIndex rootModelIndex;
    rootModelIndex = box.model()->index(0, 0);
    QVERIFY(rootModelIndex.isValid());
    box.setRootModelIndex(rootModelIndex);
    QCOMPARE(box.rootModelIndex(), rootModelIndex);

    // change the model, ensure that the root index gets reset
    oldModel = box.model();
    box.setModel(new QStandardItemModel(2, 1, &box));
    QCOMPARE(box.currentIndex(), 0);
    QVERIFY(box.model() != oldModel);
    QVERIFY(box.rootModelIndex() != rootModelIndex);
    QCOMPARE(box.rootModelIndex(), QModelIndex());

    // check that setting the very same model doesn't move the current item
    box.setCurrentIndex(1);
    QCOMPARE(box.currentIndex(), 1);
    box.setModel(box.model());
    QCOMPARE(box.currentIndex(), 1);

    // check that setting the very same model doesn't move the root index
    rootModelIndex = box.model()->index(0, 0);
    QVERIFY(rootModelIndex.isValid());
    box.setRootModelIndex(rootModelIndex);
    QCOMPARE(box.rootModelIndex(), rootModelIndex);
    box.setModel(box.model());
    QCOMPARE(box.rootModelIndex(), rootModelIndex);
}

void tst_QComboBox::setCustomModelAndView()
{
    // QTBUG-27597, ensure the correct text is returned when using custom view and a tree model.
    QComboBox combo;
    combo.setWindowTitle("QTBUG-27597, setCustomModelAndView");
    combo.setEditable(true);
    combo.setMinimumWidth(400);
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    combo.move(availableGeometry.center() - QPoint(200, 20));

    QStandardItemModel *model = new QStandardItemModel(0, 1, &combo);

    QStandardItem *item = new QStandardItem(QStringLiteral("Item1"));
    item->appendRow(new QStandardItem(QStringLiteral("Item11")));
    model->appendRow(item);

    item = new QStandardItem(QStringLiteral("Item2"));
    model->appendRow(item);
    const QString subItem21Text = QStringLiteral("Item21");
    QStandardItem *subItem = new QStandardItem(subItem21Text);
    item->appendRow(subItem);

    QTreeView* view = new QTreeView(&combo);
    view->setHeaderHidden(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setModel(model);
    view->expandAll();
    combo.setModel(model);
    combo.setView(view);
    combo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&combo));
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    const QRect subItemRect = view->visualRect(model->indexFromItem(subItem));
    QWidget *window = view->window();
    QTest::mouseClick(window->windowHandle(), Qt::LeftButton, 0, view->mapTo(window, subItemRect.center()));
    QTRY_COMPARE(combo.currentText(), subItem21Text);
}

void tst_QComboBox::modelDeleted()
{
    QComboBox box;
    QStandardItemModel *model = new QStandardItemModel;
    box.setModel(model);
    QCOMPARE(box.model(), static_cast<QAbstractItemModel *>(model));
    delete model;
    QVERIFY(box.model());
    QCOMPARE(box.findText("bubu"), -1);

    delete box.model();
    QVERIFY(box.model());
    delete box.model();
    QVERIFY(box.model());
}

void tst_QComboBox::setMaxCount()
{
    QStringList items;
    items << "1" << "2" << "3" << "4" << "5";

    QComboBox box;
    box.addItems(items);
    QCOMPARE(box.count(), 5);

    box.setMaxCount(4);
    QCOMPARE(box.count(), 4);
    QCOMPARE(box.itemText(0), QString("1"));
    QCOMPARE(box.itemText(1), QString("2"));
    QCOMPARE(box.itemText(2), QString("3"));
    QCOMPARE(box.itemText(3), QString("4"));

    // appending should do nothing
    box.addItem("foo");
    QCOMPARE(box.count(), 4);
    QCOMPARE(box.findText("foo"), -1);

    // inserting one item at top should remove the last
    box.insertItem(0, "0");
    QCOMPARE(box.count(), 4);
    QCOMPARE(box.itemText(0), QString("0"));
    QCOMPARE(box.itemText(1), QString("1"));
    QCOMPARE(box.itemText(2), QString("2"));
    QCOMPARE(box.itemText(3), QString("3"));

    // insert 5 items in a box with maxCount 4
    box.insertItems(0, items);
    QCOMPARE(box.count(), 4);
    QCOMPARE(box.itemText(0), QString("1"));
    QCOMPARE(box.itemText(1), QString("2"));
    QCOMPARE(box.itemText(2), QString("3"));
    QCOMPARE(box.itemText(3), QString("4"));

    // insert 5 items at pos 2. Make sure only two get inserted
    QSignalSpy spy(box.model(), SIGNAL(rowsInserted(QModelIndex,int,int)));
    box.insertItems(2, items);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toInt(), 2);
    QCOMPARE(spy.at(0).at(2).toInt(), 3);

    QCOMPARE(box.count(), 4);
    QCOMPARE(box.itemText(0), QString("1"));
    QCOMPARE(box.itemText(1), QString("2"));
    QCOMPARE(box.itemText(2), QString("1"));
    QCOMPARE(box.itemText(3), QString("2"));

    box.insertItems(0, QStringList());
    QCOMPARE(box.count(), 4);

    box.setMaxCount(0);
    QCOMPARE(box.count(), 0);
    box.addItem("foo");
    QCOMPARE(box.count(), 0);
    box.addItems(items);
    QCOMPARE(box.count(), 0);
}

void tst_QComboBox::convenienceViews()
{
    // QListWidget
    QComboBox listCombo;
    QListWidget *list = new QListWidget();
    listCombo.setModel(list->model());
    listCombo.setView(list);
    // add items
    list->addItem("list0");
    listCombo.addItem("list1");
    QCOMPARE(listCombo.count(), 2);
    QCOMPARE(listCombo.itemText(0), QString("list0"));
    QCOMPARE(listCombo.itemText(1), QString("list1"));

    // QTreeWidget
    QComboBox treeCombo;
    QTreeWidget *tree = new QTreeWidget();
    tree->setColumnCount(1);
    tree->header()->hide();
    treeCombo.setModel(tree->model());
    treeCombo.setView(tree);
    // add items
    tree->addTopLevelItem(new QTreeWidgetItem(QStringList("tree0")));
    treeCombo.addItem("tree1");
    QCOMPARE(treeCombo.count(), 2);
    QCOMPARE(treeCombo.itemText(0), QString("tree0"));
    QCOMPARE(treeCombo.itemText(1), QString("tree1"));

    // QTableWidget
    QComboBox tableCombo;
    QTableWidget *table = new QTableWidget(0,1);
    table->verticalHeader()->hide();
    table->horizontalHeader()->hide();
    tableCombo.setModel(table->model());
    tableCombo.setView(table);
    // add items
    table->setRowCount(table->rowCount() + 1);
    table->setItem(0, table->rowCount() - 1, new QTableWidgetItem("table0"));
    tableCombo.addItem("table1");
    QCOMPARE(tableCombo.count(), 2);
    QCOMPARE(tableCombo.itemText(0), QString("table0"));
    QCOMPARE(tableCombo.itemText(1), QString("table1"));
}

class ReturnClass : public QWidget
{
    Q_OBJECT
public:
    ReturnClass(QWidget *parent = 0)
        : QWidget(parent), received(false)
    {
        QComboBox *box = new QComboBox(this);
        box->setEditable(true);
        edit = box->lineEdit();
        box->setGeometry(rect());
    }

    void keyPressEvent(QKeyEvent *e)
    {
        received = (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter);
    }

    QLineEdit *edit;

    bool received;

};



void tst_QComboBox::ensureReturnIsIgnored()
{
    ReturnClass r;
    r.move(200, 200);
    r.show();

    QTest::keyClick(r.edit, Qt::Key_Return);
    QVERIFY(r.received);
    r.received = false;
    QTest::keyClick(r.edit, Qt::Key_Enter);
    QVERIFY(r.received);
}


void tst_QComboBox::findText_data()
{
    QTest::addColumn<QStringList>("items");
    QTest::addColumn<int>("matchflags");
    QTest::addColumn<QString>("search");
    QTest::addColumn<int>("result");

    QStringList list;
    list << "One" << "Two" << "Three" << "Four" << "Five" << "Six" << "one";
    QTest::newRow("CaseSensitive_1") << list << (int)(Qt::MatchExactly|Qt::MatchCaseSensitive)
                                     << QString("Two") << 1;
    QTest::newRow("CaseSensitive_2") << list << (int)(Qt::MatchExactly|Qt::MatchCaseSensitive)
                                     << QString("two") << -1;
    QTest::newRow("CaseSensitive_3") << list << (int)(Qt::MatchExactly|Qt::MatchCaseSensitive)
                                     << QString("One") << 0;
    QTest::newRow("CaseSensitive_4") << list << (int)(Qt::MatchExactly|Qt::MatchCaseSensitive)
                                     << QString("one") << 6;
    QTest::newRow("CaseInsensitive_1") << list << (int)(Qt::MatchExactly) << QString("Two") << 1;
    QTest::newRow("CaseInsensitive_2") << list << (int)(Qt::MatchExactly) << QString("two") << -1;
    QTest::newRow("CaseInsensitive_3") << list << (int)(Qt::MatchExactly) << QString("One") << 0;
    QTest::newRow("CaseInsensitive_4") << list << (int)(Qt::MatchExactly) << QString("one") << 6;
}
void tst_QComboBox::findText()
{
    QFETCH(QStringList, items);
    QFETCH(int, matchflags);
    QFETCH(QString, search);
    QFETCH(int, result);

    TestWidget topLevel;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QComboBox *testWidget = topLevel.comboBox();
    testWidget->clear();
    testWidget->addItems(items);

    QCOMPARE(testWidget->findText(search, (Qt::MatchFlags)matchflags), result);
}

typedef QList<int> IntList;
typedef QList<Qt::Key> KeyList;
Q_DECLARE_METATYPE(KeyList)

void tst_QComboBox::flaggedItems_data()
{
    QTest::addColumn<QStringList>("itemList");
    QTest::addColumn<IntList>("deselectFlagList");
    QTest::addColumn<IntList>("disableFlagList");
    QTest::addColumn<KeyList>("keyMovementList");
    QTest::addColumn<bool>("editable");
    QTest::addColumn<int>("expectedIndex");

    for (int editable=0;editable<2;editable++) {
        QString testCase = editable ? "editable:" : "non-editable:";
        QStringList itemList;
        itemList << "One" << "Two" << "Three" << "Four" << "Five" << "Six" << "Seven" << "Eight";
        IntList deselectFlagList;
        IntList disableFlagList;
        KeyList keyMovementList;

        keyMovementList << Qt::Key_Down << Qt::Key_Down << Qt::Key_Down << Qt::Key_Down;
        QTest::newRow(testCase.toLatin1() + "normal") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 4;

        deselectFlagList.clear();
        disableFlagList.clear();
        deselectFlagList << 1 << 3;
        QTest::newRow(testCase.toLatin1() + "non-selectable") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 4;

        deselectFlagList.clear();
        disableFlagList.clear();
        disableFlagList << 2;
        QTest::newRow(testCase.toLatin1() + "disabled") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 5;

        deselectFlagList.clear();
        disableFlagList.clear();
        deselectFlagList << 1 << 3;
        disableFlagList << 2 << 3;
        QTest::newRow(testCase.toLatin1() + "mixed") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 6;
        deselectFlagList.clear();
        disableFlagList.clear();
        disableFlagList << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        QTest::newRow(testCase.toLatin1() + "nearly-empty") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 7;

        deselectFlagList.clear();
        disableFlagList.clear();
        disableFlagList << 0 << 1 << 2 << 3 << 5 << 6 << 7;
        keyMovementList.clear();
        QTest::newRow(testCase.toLatin1() + "only one enabled") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 4;

        if (!editable) {
            deselectFlagList.clear();
            disableFlagList.clear();
            keyMovementList.clear();
            disableFlagList << 0 << 2 << 3;
            keyMovementList << Qt::Key_Down << Qt::Key_Home;
            QTest::newRow(testCase.toLatin1() + "home-disabled") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 1;

            keyMovementList.clear();
            keyMovementList << Qt::Key_End;
            QTest::newRow(testCase.toLatin1() + "end-key") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 7;

            disableFlagList.clear();
            disableFlagList << 1 ;
            keyMovementList << Qt::Key_T;
            QTest::newRow(testCase.toLatin1() + "keyboard-search") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 2;

            itemList << "nine" << "ten";
            keyMovementList << Qt::Key_T;
            QTest::newRow(testCase.toLatin1() + "search same start letter") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 2;

            keyMovementList.clear();
            keyMovementList << Qt::Key_T << Qt::Key_H;
            QTest::newRow(testCase.toLatin1() + "keyboard search item") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 2;

            disableFlagList.clear();
            disableFlagList << 1 << 3 << 5 << 7 << 9;
            keyMovementList.clear();
            keyMovementList << Qt::Key_End << Qt::Key_Up << Qt::Key_Up << Qt::Key_PageDown << Qt::Key_PageUp << Qt::Key_PageUp << Qt::Key_Down;
            QTest::newRow(testCase.toLatin1() + "all key combinations") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 4;
        } else {
            deselectFlagList.clear();
            disableFlagList.clear();
            disableFlagList << 1;
            keyMovementList.clear();
            keyMovementList << Qt::Key_T << Qt::Key_Enter;
            QTest::newRow(testCase.toLatin1() + "disabled") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 2;
            QTest::newRow(testCase.toLatin1() + "broken autocompletion") << itemList << deselectFlagList << disableFlagList << keyMovementList << bool(editable) << 2;
        }
    }
}

void tst_QComboBox::flaggedItems()
{
    QFETCH(QStringList, itemList);
    QFETCH(IntList, deselectFlagList);
    QFETCH(IntList, disableFlagList);
    QFETCH(KeyList, keyMovementList);
    QFETCH(bool, editable);
    QFETCH(int, expectedIndex);

    QComboBox comboBox;
    setFrameless(&comboBox);
    QListWidget listWidget;
    listWidget.addItems(itemList);

    comboBox.setEditable(editable);
    foreach (int index, deselectFlagList)
        listWidget.item(index)->setFlags(listWidget.item(index)->flags() & ~Qt::ItemIsSelectable);

    foreach (int index, disableFlagList)
        listWidget.item(index)->setFlags(listWidget.item(index)->flags() & ~Qt::ItemIsEnabled);

    comboBox.setModel(listWidget.model());
    comboBox.setView(&listWidget);
    comboBox.move(200, 200);
    comboBox.show();
    QApplication::setActiveWindow(&comboBox);
    comboBox.activateWindow();
    comboBox.setFocus();
    QTRY_VERIFY(comboBox.isVisible());
    QTRY_VERIFY(comboBox.hasFocus());

    if (editable)
        comboBox.lineEdit()->selectAll();

    QSignalSpy indexChangedInt(&comboBox, SIGNAL(currentIndexChanged(int)));
    for (int i = 0; i < keyMovementList.count(); ++i) {
        Qt::Key key = keyMovementList[i];
        QTest::keyClick(&comboBox, key);
        if (indexChangedInt.count() != i + 1) {
            QTest::qWait(400);
        }
    }

    QCOMPARE(comboBox.currentIndex() , expectedIndex);
}

void tst_QComboBox::pixmapIcon()
{
    QComboBox box;
    QStandardItemModel *model = new QStandardItemModel(2, 1, &box);

    QPixmap pix(10, 10);
    pix.fill(Qt::red);
    model->setData(model->index(0, 0), "Element 1");
    model->setData(model->index(0, 0), pix, Qt::DecorationRole);

    QIcon icon(pix);
    model->setData(model->index(1, 0), "Element 2");
    model->setData(model->index(1, 0), icon, Qt::DecorationRole);

    box.setModel(model);

    QCOMPARE( box.itemIcon(0).isNull(), false );
    QCOMPARE( box.itemIcon(1).isNull(), false );
}

#ifndef QT_NO_WHEELEVENT
// defined to be 120 by the wheel mouse vendors according to the docs
#define WHEEL_DELTA 120

void tst_QComboBox::mouseWheel_data()
{
    QTest::addColumn<IntList>("disabledItems");
    QTest::addColumn<int>("startIndex");
    QTest::addColumn<int>("wheelDirection");
    QTest::addColumn<int>("expectedIndex");

    IntList disabled;
    disabled << 0 << 1 << 2 << 4;
    int start = 3;
    int wheel = 1;
    int expected = 3;
    QTest::newRow("upper locked") << disabled << start << wheel << expected;

    wheel = -1;
#ifdef Q_OS_DARWIN
    // on OS X & iOS mouse wheel shall have no effect on combo box
    expected = start;
#else
    // on other OSes we should jump to next enabled item (no. 5)
    expected = 5;
#endif
    QTest::newRow("jump over") << disabled << start << wheel << expected;

    disabled.clear();
    disabled << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9;
    start = 0;
    wheel = -1;
    expected = 0;
    QTest::newRow("single Item enabled") << disabled << start << wheel << expected;
}

void tst_QComboBox::mouseWheel()
{
    QFETCH(IntList, disabledItems);
    QFETCH(int, startIndex);
    QFETCH(int, wheelDirection);
    QFETCH(int, expectedIndex);

    QCoreApplication *applicationInstance = QCoreApplication::instance();
    QVERIFY(applicationInstance != 0);

    QComboBox box;
    QStringList list;
    list << "one" << "two" << "three" << "four" << "five" << "six" << "seven" << "eight" << "nine" << "ten";

    QListWidget listWidget;
    listWidget.addItems(list);

    foreach (int index, disabledItems)
        listWidget.item(index)->setFlags(listWidget.item(index)->flags() & ~Qt::ItemIsEnabled);

    box.setModel(listWidget.model());
    box.setView(&listWidget);
    for (int i=0; i < 2; ++i) {
        box.setEditable(i==0?false:true);
        box.setCurrentIndex(startIndex);

        QWheelEvent event = QWheelEvent(box.rect().bottomRight() , WHEEL_DELTA * wheelDirection, Qt::NoButton, Qt::NoModifier);
        QVERIFY(applicationInstance->sendEvent(&box,&event));

        QCOMPARE(box.currentIndex(), expectedIndex);
    }
}

void tst_QComboBox::popupWheelHandling()
{
    // QTBUG-40656, QTBUG-42731 combo and other popups should not be affected by wheel events.
    QScrollArea scrollArea;
    scrollArea.move(300, 300);
    QWidget *widget = new QWidget;
    scrollArea.setWidget(widget);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    layout->addSpacing(100);
    QComboBox *comboBox = new QComboBox;
    comboBox->addItems(QStringList() << QStringLiteral("Won") << QStringLiteral("Too")
                       << QStringLiteral("3") << QStringLiteral("fore"));
    layout->addWidget(comboBox);
    layout->addSpacing(100);
    const QPoint sizeP(scrollArea.width(), scrollArea.height());
    scrollArea.move(QGuiApplication::primaryScreen()->availableGeometry().center() - sizeP / 2);
    scrollArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scrollArea));
    comboBox->showPopup();
    QTRY_VERIFY(comboBox->view() && comboBox->view()->isVisible());
    const QPoint popupPos = comboBox->view()->pos();
    QWheelEvent event(QPointF(10, 10), WHEEL_DELTA, Qt::NoButton, Qt::NoModifier);
    QVERIFY(QCoreApplication::sendEvent(scrollArea.windowHandle(), &event));
    QCoreApplication::processEvents();
    QVERIFY(comboBox->view()->isVisible());
    QCOMPARE(comboBox->view()->pos(), popupPos);
}
#endif // !QT_NO_WHEELEVENT

void tst_QComboBox::layoutDirection()
{
    QComboBox box;
    Qt::LayoutDirection dir;
    QLineEdit *lineEdit;

    // RTL
    box.setLayoutDirection(Qt::RightToLeft);
    QStyleOptionComboBox opt;
    opt.direction = Qt::RightToLeft;
    dir = (Qt::LayoutDirection)box.style()->styleHint(QStyle::SH_ComboBox_LayoutDirection, &opt, &box);

    QCOMPARE(box.view()->layoutDirection(), dir);
    box.setEditable(true);
    QCOMPARE(box.lineEdit()->layoutDirection(), dir);
    lineEdit = new QLineEdit;
    QCOMPARE(lineEdit->layoutDirection(), qApp->layoutDirection());
    box.setLineEdit(lineEdit);
    QCOMPARE(lineEdit->layoutDirection(), dir);

    // LTR
    box.setLayoutDirection(Qt::LeftToRight);
    qApp->setLayoutDirection(Qt::RightToLeft);

    opt.direction = Qt::LeftToRight;
    dir = (Qt::LayoutDirection)box.style()->styleHint(QStyle::SH_ComboBox_LayoutDirection, &opt, &box);

    QCOMPARE(box.view()->layoutDirection(), dir);
    box.setEditable(true);
    QCOMPARE(box.lineEdit()->layoutDirection(), dir);
    lineEdit = new QLineEdit;
    QCOMPARE(lineEdit->layoutDirection(), qApp->layoutDirection());
    box.setLineEdit(lineEdit);
    QCOMPARE(lineEdit->layoutDirection(), dir);

}

void tst_QComboBox::itemListPosition()
{
    //tests that the list is not out of the screen boundaries

    //put the QApplication layout back
    QApplication::setLayoutDirection(Qt::LeftToRight);

    //we test QFontComboBox because it has the specific behaviour to set a fixed size
    //to the list view
    QWidget topLevel;
    QHBoxLayout *layout = new QHBoxLayout(&topLevel);

    QFontComboBox combo(&topLevel);

    layout->addWidget(&combo);
    //the code to get the available screen space is copied from QComboBox code
    const int scrNumber = QApplication::desktop()->screenNumber(&combo);

    bool useFullScreenForPopupMenu = false;
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        useFullScreenForPopupMenu = theme->themeHint(QPlatformTheme::UseFullScreenForPopupMenu).toBool();
    const QRect screen = useFullScreenForPopupMenu ?
                         QApplication::desktop()->screenGeometry(scrNumber) :
                         QApplication::desktop()->availableGeometry(scrNumber);

    topLevel.move(screen.width() - topLevel.sizeHint().width() - 10, 0); //puts the combo to the top-right corner

    topLevel.showNormal();

    //wait because the window manager can move the window if there is a right panel
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    combo.showPopup();
    QTRY_VERIFY(combo.view());
    QTRY_VERIFY(combo.view()->isVisible());
    QVERIFY( combo.view()->window()->x() + combo.view()->window()->width() <= screen.x() + screen.width() );
}

void tst_QComboBox::separatorItem_data()
{
    QTest::addColumn<QStringList>("items");
    QTest::addColumn<IntList>("separators");

    QTest::newRow("test") << (QStringList() << "one" << "two" << "three" << "other...")
                          << (IntList() << 4);
}

void tst_QComboBox::separatorItem()
{
    QFETCH(QStringList, items);
    QFETCH(IntList, separators);

    QComboBox box;
    box.addItems(items);
    foreach(int index, separators)
        box.insertSeparator(index);
    QCOMPARE(box.count(), (items.count() + separators.count()));
    for (int i = 0, s = 0; i < box.count(); ++i) {
        if (i == separators.at(s)) {
            QCOMPARE(box.itemText(i), QString());
            ++s;
        } else {
            QCOMPARE(box.itemText(i), items.at(i - s));
        }
    }
}

// This test requires the Fusionstyle
#ifndef QT_NO_STYLE_FUSION
void tst_QComboBox::task190351_layout()
{
    const QString oldStyle = QApplication::style()->objectName();
    QApplication::setStyle(QStyleFactory::create(QLatin1String("Fusion")));

    QComboBox listCombo;
    listCombo.move(200, 200);
    setFrameless(&listCombo);
    QListWidget *list = new QListWidget();
    listCombo.setModel(list->model());
    listCombo.setView(list);
    for(int i = 1; i < 150; i++)
        list->addItem(QLatin1String("list") + QString::number(i));

    listCombo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&listCombo));
    QTRY_VERIFY(listCombo.isVisible());
    listCombo.setCurrentIndex(70);
    listCombo.showPopup();
    QTRY_VERIFY(listCombo.view());
    QTest::qWaitForWindowExposed(listCombo.view());
    QTRY_VERIFY(listCombo.view()->isVisible());
    QApplication::processEvents();

#ifdef QT_BUILD_INTERNAL
    QFrame *container = listCombo.findChild<QComboBoxPrivateContainer *>();
    QVERIFY(container);
    QCOMPARE(static_cast<QAbstractItemView *>(list), container->findChild<QAbstractItemView *>());
    QWidget *top = container->findChild<QComboBoxPrivateScroller *>();
    QVERIFY(top);
    QVERIFY(top->isVisible());
    QCOMPARE(top->mapToGlobal(QPoint(0, top->height())).y(), list->mapToGlobal(QPoint()).y());
#endif

    QApplication::setStyle(oldStyle);
}
#endif

class task166349_ComboBox : public QComboBox
{
    Q_OBJECT
public:
    task166349_ComboBox(QWidget *parent = 0) : QComboBox(parent)
    {
        QStringList list;
        list << "one" << "two";
        connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
        addItems(list);
    }
public slots:
    void onCurrentIndexChanged(int index)
    {
        setEditable(index % 2 == 1);
    }
};

void tst_QComboBox::task166349_setEditableOnReturn()
{
    task166349_ComboBox comboBox;
    QTest::keyClick(&comboBox, Qt::Key_Down);
    QTest::keyClick(&comboBox, Qt::Key_1);
    QTest::keyClick(&comboBox, Qt::Key_Enter);
    QCOMPARE(QLatin1String("two1"), comboBox.itemText(comboBox.count() - 1));
}

// This test requires the Fusion style.
#ifndef QT_NO_STYLE_FUSION
void tst_QComboBox::task191329_size()
{
    const QString oldStyle = QApplication::style()->objectName();
    QApplication::setStyle(QStyleFactory::create(QLatin1String("Fusion")));


    QComboBox tableCombo;
    setFrameless(&tableCombo);
    tableCombo.move(200, 200);
    int rows;
    if (QApplication::desktop()->screenGeometry().height() < 480)
        rows = 8;
    else
        rows = 15;

    QStandardItemModel model(rows, 2);
    for (int row = 0; row < model.rowCount(); ++row) {
        const QString rowS = QLatin1String("row ") + QString::number(row);
        for (int column = 0; column < model.columnCount(); ++column) {
            const QString text = rowS + QLatin1String(", column ") + QString::number(column);
            model.setItem(row, column, new QStandardItem(text));
        }
    }
    QTableView *table = new QTableView();
    table->verticalHeader()->hide();
    table->horizontalHeader()->hide();
    tableCombo.setView(table);
    tableCombo.setModel(&model);

    tableCombo.show();
    QTRY_VERIFY(tableCombo.isVisible());
    tableCombo.showPopup();
    QTRY_VERIFY(tableCombo.view());
    QTRY_VERIFY(tableCombo.view()->isVisible());

#ifdef QT_BUILD_INTERNAL
    QFrame *container = tableCombo.findChild<QComboBoxPrivateContainer *>();
    QVERIFY(container);
    QCOMPARE(static_cast<QAbstractItemView *>(table), container->findChild<QAbstractItemView *>());
    foreach (QWidget *button, container->findChildren<QComboBoxPrivateScroller *>()) {
        //the popup should be large enough to contains everithing so the top and left button are hidden
        QVERIFY(!button->isVisible());
    }
#endif

    QApplication::setStyle(oldStyle);
}
#endif

void tst_QComboBox::task190205_setModelAdjustToContents()
{
    QStringList initialContent;
    QStringList finalContent;
    initialContent << "foo" << "bar";
    finalContent << "bar" << "foooooooobar";

    QComboBox box;
    setFrameless(&box);
    box.move(100, 100);
    box.setSizeAdjustPolicy(QComboBox::AdjustToContents);
    box.addItems(initialContent);
    box.showNormal();

    //wait needed in order to get the combo initial size
    QTRY_VERIFY(box.isVisible());

    box.setModel(new QStringListModel(finalContent));

    QComboBox correctBox;
    setFrameless(&correctBox);
    correctBox.move(400, 100);

    correctBox.addItems(finalContent);
    correctBox.showNormal();

    QVERIFY(QTest::qWaitForWindowExposed(&box));
    QVERIFY(QTest::qWaitForWindowExposed(&correctBox));

    // box should be resized to the same size as correctBox
    QTRY_COMPARE(box.size(), correctBox.size());
}

void tst_QComboBox::task248169_popupWithMinimalSize()
{
    QStringList initialContent;
    initialContent << "foo" << "bar" << "foobar";

    QComboBox comboBox;
    comboBox.addItems(initialContent);
    QDesktopWidget desktop;
    QRect desktopSize = desktop.availableGeometry();
    comboBox.view()->setMinimumWidth(desktopSize.width() / 2);

    comboBox.setGeometry(desktopSize.width() - (desktopSize.width() / 4), (desktopSize.width() / 4), (desktopSize.width() / 2), (desktopSize.width() / 4));

    comboBox.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&comboBox));
    QTRY_VERIFY(comboBox.isVisible());
    comboBox.showPopup();
    QTRY_VERIFY(comboBox.view());
    QTest::qWaitForWindowExposed(comboBox.view());
    QTRY_VERIFY(comboBox.view()->isVisible());

#if defined QT_BUILD_INTERNAL
    QFrame *container = comboBox.findChild<QComboBoxPrivateContainer *>();
    QVERIFY(container);
    QTRY_VERIFY(desktop.screenGeometry(container).contains(container->geometry()));
#endif
}

void tst_QComboBox::task247863_keyBoardSelection()
{
  QComboBox combo;
  setFrameless(&combo);
  combo.move(200, 200);
  combo.setEditable(false);
  combo.addItem( QLatin1String("111"));
  combo.addItem( QLatin1String("222"));
  combo.show();
  QApplication::setActiveWindow(&combo);
  QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&combo));

  QSignalSpy spy(&combo, SIGNAL(activated(QString)));
  qApp->setEffectEnabled(Qt::UI_AnimateCombo, false);
  QTest::keyClick(&combo, Qt::Key_Space);
  qApp->setEffectEnabled(Qt::UI_AnimateCombo, true);
  QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down);
  QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter);
  QCOMPARE(combo.currentText(), QLatin1String("222"));
  QCOMPARE(spy.count(), 1);
}

void tst_QComboBox::task220195_keyBoardSelection2()
{
    QComboBox combo;
    setFrameless(&combo);
    combo.move(200, 200);
    combo.setEditable(false);
    combo.addItem( QLatin1String("foo1"));
    combo.addItem( QLatin1String("foo2"));
    combo.addItem( QLatin1String("foo3"));
    combo.show();
    QApplication::setActiveWindow(&combo);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&combo));

    combo.setCurrentIndex(-1);
    QVERIFY(combo.currentText().isNull());

    QTest::keyClick(&combo, 'f');
    QCOMPARE(combo.currentText(), QLatin1String("foo1"));
    QTest::qWait(QApplication::keyboardInputInterval() + 30);
    QTest::keyClick(&combo, 'f');
    QCOMPARE(combo.currentText(), QLatin1String("foo2"));
    QTest::qWait(QApplication::keyboardInputInterval() + 30);
    QTest::keyClick(&combo, 'f');
    QCOMPARE(combo.currentText(), QLatin1String("foo3"));
    QTest::qWait(QApplication::keyboardInputInterval() + 30);
    QTest::keyClick(&combo, 'f');
    QCOMPARE(combo.currentText(), QLatin1String("foo1"));
    QTest::qWait(QApplication::keyboardInputInterval() + 30);

    combo.setCurrentIndex(1);
    QCOMPARE(combo.currentText(), QLatin1String("foo2"));
    QTest::keyClick(&combo, 'f');
    QCOMPARE(combo.currentText(), QLatin1String("foo3"));
}


void tst_QComboBox::setModelColumn()
{
    QStandardItemModel model(5,3);
    model.setItem(0,0, new QStandardItem("0"));
    model.setItem(1,0, new QStandardItem("1"));
    model.setItem(2,0, new QStandardItem("2"));
    model.setItem(3,0, new QStandardItem("3"));
    model.setItem(4,0, new QStandardItem("4"));
    model.setItem(0,1, new QStandardItem("zero"));
    model.setItem(1,1, new QStandardItem("un"));
    model.setItem(2,1, new QStandardItem("deux"));
    model.setItem(3,1, new QStandardItem("trois"));
    model.setItem(4,1, new QStandardItem("quatre"));
    model.setItem(0,2, new QStandardItem("a"));
    model.setItem(1,2, new QStandardItem("b"));
    model.setItem(2,2, new QStandardItem("c"));
    model.setItem(3,2, new QStandardItem("d"));
    model.setItem(4,2, new QStandardItem("e"));

    QComboBox box;
    box.setModel(&model);
    QCOMPARE(box.currentText(), QString("0"));
    box.setModelColumn(1);
    QCOMPARE(box.currentText(), QString("zero"));
}

void tst_QComboBox::noScrollbar_data()
{
    QTest::addColumn<QString>("stylesheet");

    QTest::newRow("normal") << QString();
    QTest::newRow("border") << QString::fromLatin1("QAbstractItemView { border: 12px solid blue;}");
    QTest::newRow("margin") << QString::fromLatin1("QAbstractItemView { margin: 12px 15px 13px 10px; }");
    QTest::newRow("padding") << QString::fromLatin1("QAbstractItemView { padding: 12px 15px 13px 10px;}");
    QTest::newRow("everything") << QString::fromLatin1("QAbstractItemView {  border: 12px  solid blue; "
                                                       " padding: 12px 15px 13px 10px; margin: 12px 15px 13px 10px;  }");
    QTest::newRow("everything and more") << QString::fromLatin1("QAbstractItemView {  border: 1px 3px 5px 1px solid blue; "
                                                       " padding: 2px 5px 3px 1px; margin: 2px 5px 3px 1px;  } "
                                                       " QAbstractItemView::item {  border: 2px solid green; "
                                                       "                      padding: 1px 1px 2px 2px; margin: 1px; } " );
}

void tst_QComboBox::noScrollbar()
{
    QStringList initialContent;
    initialContent << "foo" << "bar" << "foobar" << "moo";
    QFETCH(QString, stylesheet);
    QString oldCss = qApp->styleSheet();
    qApp->setStyleSheet(stylesheet);

    {
        QWidget topLevel;
        QComboBox comboBox(&topLevel);
        comboBox.addItems(initialContent);
        topLevel.move(200, 200);
        topLevel.show();
        comboBox.resize(200, comboBox.height());
        QTRY_VERIFY(comboBox.isVisible());
        comboBox.showPopup();
        QTRY_VERIFY(comboBox.view());
        QTRY_VERIFY(comboBox.view()->isVisible());

        QVERIFY(!comboBox.view()->horizontalScrollBar()->isVisible());
        QVERIFY(!comboBox.view()->verticalScrollBar()->isVisible());
    }

    {
        QTableWidget *table = new QTableWidget(2,2);
        QComboBox comboBox;
        comboBox.setModel(table->model());
        comboBox.setView(table);
        comboBox.move(200, 200);
        comboBox.show();
        QTRY_VERIFY(comboBox.isVisible());
        comboBox.resize(200, comboBox.height());
        comboBox.showPopup();
        QTRY_VERIFY(comboBox.view());
        QTRY_VERIFY(comboBox.view()->isVisible());

        QVERIFY(!comboBox.view()->horizontalScrollBar()->isVisible());
        QVERIFY(!comboBox.view()->verticalScrollBar()->isVisible());
    }

    qApp->setStyleSheet(oldCss);
}

void tst_QComboBox::setItemDelegate()
{
    QComboBox comboBox;
    QStyledItemDelegate *itemDelegate = new QStyledItemDelegate;
    comboBox.setItemDelegate(itemDelegate);
    // the cast is a workaround for the XLC and Metrowerks compilers
    QCOMPARE(static_cast<QStyledItemDelegate *>(comboBox.itemDelegate()), itemDelegate);
}

void tst_QComboBox::task253944_itemDelegateIsReset()
{
    QComboBox comboBox;
    QStyledItemDelegate *itemDelegate = new QStyledItemDelegate;
    comboBox.setItemDelegate(itemDelegate);

    // the casts are workarounds for the XLC and Metrowerks compilers

    comboBox.setEditable(true);
    QCOMPARE(static_cast<QStyledItemDelegate *>(comboBox.itemDelegate()), itemDelegate);

    comboBox.setStyleSheet("QComboBox { border: 1px solid gray; }");
    QCOMPARE(static_cast<QStyledItemDelegate *>(comboBox.itemDelegate()), itemDelegate);
}


void tst_QComboBox::subControlRectsWithOffset_data()
{
    QTest::addColumn<bool>("editable");

    QTest::newRow("editable = true") << true;
    QTest::newRow("editable = false") << false;
}

void tst_QComboBox::subControlRectsWithOffset()
{
    // The sub control rect relative position should not depends
    // on the position of the combobox

    class FriendlyCombo : public QComboBox {
    public:
        void styleOption(QStyleOptionComboBox *optCombo) {
            initStyleOption(optCombo);
        }
    } combo;
    QStyleOptionComboBox optCombo;
    combo.styleOption(&optCombo);


    const QRect rectAtOrigin(0, 0, 80, 30);
    const QPoint offset(25, 50);
    const QRect rectWithOffset = rectAtOrigin.translated(offset);

    QStyle *style = combo.style();

    QFETCH(bool, editable);
    optCombo.editable = editable;

    optCombo.rect = rectAtOrigin;
    QRect editFieldRect = style->subControlRect(QStyle::CC_ComboBox, &optCombo, QStyle::SC_ComboBoxEditField, 0);
    QRect arrowRect = style->subControlRect(QStyle::CC_ComboBox, &optCombo, QStyle::SC_ComboBoxArrow, 0);
    QRect listboxRect = style->subControlRect(QStyle::CC_ComboBox, &optCombo, QStyle::SC_ComboBoxListBoxPopup, 0);

    optCombo.rect = rectWithOffset;
    QRect editFieldRectWithOffset = style->subControlRect(QStyle::CC_ComboBox, &optCombo, QStyle::SC_ComboBoxEditField, 0);
    QRect arrowRectWithOffset = style->subControlRect(QStyle::CC_ComboBox, &optCombo, QStyle::SC_ComboBoxArrow, 0);
    QRect listboxRectWithOffset = style->subControlRect(QStyle::CC_ComboBox, &optCombo, QStyle::SC_ComboBoxListBoxPopup, 0);

    QCOMPARE(editFieldRect, editFieldRectWithOffset.translated(-offset));
    QCOMPARE(arrowRect, arrowRectWithOffset.translated(-offset));
    QCOMPARE(listboxRect, listboxRectWithOffset.translated(-offset));

}

// This test depends on Windows style.
#ifndef QT_NO_STYLE_WINDOWS
void tst_QComboBox::task260974_menuItemRectangleForComboBoxPopup()
{
    class TestStyle: public QProxyStyle
    {
    public:
        TestStyle() : QProxyStyle(QStyleFactory::create("windows")) { }

        int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *ret) const
        {
            if (hint == SH_ComboBox_Popup) return 1;
            else return QCommonStyle::styleHint(hint, option, widget, ret);
        }

        void drawControl(ControlElement element, const QStyleOption *option, QPainter *, const QWidget *) const
        {
            if (element == CE_MenuItem)
                discoveredRect = option->rect;
        }

        mutable QRect discoveredRect;
    } style;


    {
        QComboBox comboBox;
        comboBox.setStyle(&style);
        comboBox.addItem("Item 1");
        comboBox.move(200, 200);

        comboBox.show();
        QTRY_VERIFY(comboBox.isVisible());
        comboBox.showPopup();
        QTRY_VERIFY(comboBox.view());
        QTRY_VERIFY(comboBox.view()->isVisible());

        QTRY_VERIFY(style.discoveredRect.width() <= comboBox.width());
    }
}
#endif

void tst_QComboBox::removeItem()
{
    QComboBox cb;
    cb.removeItem(-1);
    cb.removeItem(1);
    cb.removeItem(0);
    QCOMPARE(cb.count(), 0);

    cb.addItem("foo");
    cb.removeItem(-1);
    QCOMPARE(cb.count(), 1);
    cb.removeItem(1);
    QCOMPARE(cb.count(), 1);
    cb.removeItem(0);
    QCOMPARE(cb.count(), 0);
}

void tst_QComboBox::resetModel()
{
    class StringListModel : public QStringListModel
    {
    public:
        StringListModel(const QStringList &list) : QStringListModel(list)
        {
        }

        void reset()
        {
            QStringListModel::beginResetModel();
            QStringListModel::endResetModel();
        }
    };
    QComboBox cb;
    StringListModel model( QStringList() << "1" << "2");
    QSignalSpy spy(&cb, SIGNAL(currentIndexChanged(int)));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(cb.currentIndex(), -1); //no selection

    cb.setModel(&model);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(cb.currentIndex(), 0); //first item selected

    model.reset();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(cb.currentIndex(), -1); //no selection

}

static inline void centerCursor(const QWidget *w)
{
#ifndef QT_NO_CURSOR
    // Force cursor movement to prevent QCursor::setPos() from returning prematurely on QPA:
    const QPoint target(w->mapToGlobal(w->rect().center()));
    QCursor::setPos(QPoint(target.x() + 1, target.y()));
    QCursor::setPos(target);
#else // !QT_NO_CURSOR
    Q_UNUSED(w)
#endif
}

void tst_QComboBox::keyBoardNavigationWithMouse()
{
    QComboBox combo;
    combo.setEditable(false);
    setFrameless(&combo);
    combo.move(200, 200);
    for (int i = 0; i < 80; i++)
        combo.addItem( QString::number(i));

    combo.showNormal();
    centerCursor(&combo); // QTBUG-33973, cursor needs to be within view from start on Mac.
    QApplication::setActiveWindow(&combo);
    QVERIFY(QTest::qWaitForWindowActive(&combo));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&combo));

    QCOMPARE(combo.currentText(), QLatin1String("0"));

    combo.setFocus();
    QTRY_VERIFY(combo.hasFocus());

    QTest::keyClick(combo.lineEdit(), Qt::Key_Space);
    QTest::qWait(30);
    QTRY_VERIFY(combo.view());
    QTRY_VERIFY(combo.view()->isVisible());
    QTest::qWait(130);

    QCOMPARE(combo.currentText(), QLatin1String("0"));

    // When calling cursor function, Windows CE responds with: This function is not supported on this system.
#if !defined Q_OS_QNX
    // Force cursor movement to prevent QCursor::setPos() from returning prematurely on QPA:
    centerCursor(combo.view());
    QTest::qWait(200);

#define GET_SELECTION(SEL) \
    QCOMPARE(combo.view()->selectionModel()->selection().count(), 1); \
    QCOMPARE(combo.view()->selectionModel()->selection().indexes().count(), 1); \
    SEL = combo.view()->selectionModel()->selection().indexes().first().row()

    int selection;
    GET_SELECTION(selection);

    //since we moved the mouse is in the middle it should even be around 5;
    QVERIFY2(selection > 3, (QByteArrayLiteral("selection=") + QByteArray::number(selection)).constData());

    static const int final = 40;
    for (int i = selection + 1;  i <= final; i++)
    {
        QTest::keyClick(combo.view(), Qt::Key_Down);
        QTest::qWait(20);
        GET_SELECTION(selection);
        QCOMPARE(selection, i);
    }

    QTest::keyClick(combo.view(), Qt::Key_Enter);
    QTRY_COMPARE(combo.currentText(), QString::number(final));
#endif
}

void tst_QComboBox::task_QTBUG_1071_changingFocusEmitsActivated()
{
    QWidget w;
    w.move(200, 200);
    QVBoxLayout layout(&w);
    QComboBox cb;
    cb.setEditable(true);
    QSignalSpy spy(&cb, SIGNAL(activated(int)));
    cb.addItem("0");
    cb.addItem("1");
    cb.addItem("2");
    QLineEdit edit;
    layout.addWidget(&cb);
    layout.addWidget(&edit);

    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));
    cb.clearEditText();
    cb.setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(cb.hasFocus());
    QTest::keyClick(static_cast<QWidget *>(0), '1');
    QCOMPARE(spy.count(), 0);
    edit.setFocus();
    QTRY_VERIFY(edit.hasFocus());
    QTRY_COMPARE(spy.count(), 1);
}

void tst_QComboBox::maxVisibleItems_data()
{
    QTest::addColumn<int>("spacing");
    QTest::newRow("Default")  << -1;
    QTest::newRow("No spacing") << 0;
    QTest::newRow("20")  << -1;
}

void tst_QComboBox::maxVisibleItems()
{
    QFETCH(int, spacing);

    QComboBox comboBox;
    QCOMPARE(comboBox.maxVisibleItems(), 10); //default value.

    QStringList content;
    for(int i = 1; i < 50; i++)
        content += QString::number(i);

    comboBox.addItems(content);
    comboBox.move(200, 200);
    comboBox.show();
    comboBox.resize(200, comboBox.height());
    QTRY_VERIFY(comboBox.isVisible());

    comboBox.setMaxVisibleItems(5);
    QCOMPARE(comboBox.maxVisibleItems(), 5);

    comboBox.showPopup();
    QTRY_VERIFY(comboBox.view());
    QTRY_VERIFY(comboBox.view()->isVisible());

    QListView *listView = qobject_cast<QListView*>(comboBox.view());
    QVERIFY(listView);
    if (spacing >= 0)
        listView->setSpacing(spacing);

    const int itemHeight = listView->visualRect(listView->model()->index(0,0)).height()
        + 2 * listView->spacing();

    QStyleOptionComboBox opt;
    opt.initFrom(&comboBox);
    if (!comboBox.style()->styleHint(QStyle::SH_ComboBox_Popup, &opt))
        QCOMPARE(listView->viewport()->height(), itemHeight * comboBox.maxVisibleItems());
}

void tst_QComboBox::task_QTBUG_10491_currentIndexAndModelColumn()
{
    QComboBox comboBox;

    QStandardItemModel model(4, 4, &comboBox);
    for (int i = 0; i < 4; i++){
        const QString iS = QString::number(i);
        model.setItem(i, 0, new QStandardItem(QLatin1String("Employee Nr ") + iS));
        model.setItem(i, 1, new QStandardItem(QLatin1String("Street Nr ") + iS));
        model.setItem(i, 2, new QStandardItem(QLatin1String("Town Nr ") + iS));
        model.setItem(i, 3, new QStandardItem(QLatin1String("Phone Nr ") + iS));
    }
    comboBox.setModel(&model);
    comboBox.setModelColumn(0);

    QComboBoxPrivate *d = static_cast<QComboBoxPrivate *>(QComboBoxPrivate::get(&comboBox));
    d->setCurrentIndex(model.index(2, 2));
    QCOMPARE(QModelIndex(d->currentIndex), model.index(2, comboBox.modelColumn()));
}

void tst_QComboBox::highlightedSignal()
{
    QComboBox comboBox;

    QSignalSpy spy(&comboBox, SIGNAL(highlighted(int)));
    QVERIFY(spy.isValid());

    // Calling view() before setting the model causes the creation
    // of a QComboBoxPrivateContainer containing an actual view, and connecting to
    // the selectionModel to generate the highlighted signal. When setModel is called
    // further down, that selectionModel is obsolete. We test that the highlighted
    // signal is emitted anyway as the bug fix. (QTBUG-4454)
    comboBox.view();
    QItemSelectionModel *initialItemSelectionModel = comboBox.view()->selectionModel();


    QStandardItemModel model;
    for (int i = 0; i < 5; i++)
        model.appendRow(new QStandardItem(QString::number(i)));
    comboBox.setModel(&model);

    comboBox.view()->selectionModel()->setCurrentIndex(model.index(0, 0), QItemSelectionModel::Current);

    QVERIFY(initialItemSelectionModel != comboBox.view()->selectionModel());

    QCOMPARE(spy.size(), 1);
}

void tst_QComboBox::itemData()
{
    QComboBox comboBox;
    const int itemCount = 10;

    // ensure that the currentText(), the DisplayRole and the EditRole
    // stay in sync when using QComboBox's default model
    for (int i = 0; i < itemCount; ++i) {
        QString itemText = QLatin1String("item text ") + QString::number(i);
        comboBox.addItem(itemText);
    }

    for (int i = 0; i < itemCount; ++i) {
        QString itemText = QLatin1String("item text ") + QString::number(i);
        QCOMPARE(comboBox.itemText(i), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::EditRole).toString(), itemText);

        comboBox.setCurrentIndex(i);
        QCOMPARE(comboBox.currentIndex(), i);
        QCOMPARE(comboBox.currentText(), itemText);
        QCOMPARE(comboBox.currentData(Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.currentData(Qt::EditRole).toString(), itemText);
    }

    for (int i = 0; i < itemCount; ++i) // now change by using setItemText
        comboBox.setItemText(i, QLatin1String("setItemText ") + QString::number(i));

    for (int i = 0; i < itemCount; ++i) {
        QString itemText = QLatin1String("setItemText ") + QString::number(i);
        QCOMPARE(comboBox.itemText(i), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::EditRole).toString(), itemText);

        comboBox.setCurrentIndex(i);
        QCOMPARE(comboBox.currentIndex(), i);
        QCOMPARE(comboBox.currentText(), itemText);
        QCOMPARE(comboBox.currentData(Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.currentData(Qt::EditRole).toString(), itemText);
    }

    for (int i = 0; i < itemCount; ++i) {
        // now change by changing the DisplayRole's data
        QString itemText = QLatin1String("setItemData(DisplayRole) ") + QString::number(i);
        comboBox.setItemData(i, QVariant(itemText), Qt::DisplayRole);
    }

    for (int i = 0; i < itemCount; ++i) {
        QString itemText = QLatin1String("setItemData(DisplayRole) ") + QString::number(i);
        QCOMPARE(comboBox.itemText(i), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::EditRole).toString(), itemText);

        comboBox.setCurrentIndex(i);
        QCOMPARE(comboBox.currentIndex(), i);
        QCOMPARE(comboBox.currentText(), itemText);
        QCOMPARE(comboBox.currentData(Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.currentData(Qt::EditRole).toString(), itemText);
    }

    for (int i = 0; i < itemCount; ++i) {
        // now change by changing the EditRole's data
        QString itemText = QLatin1String("setItemData(EditRole) ") + QString::number(i);
        comboBox.setItemData(i, QVariant(itemText), Qt::EditRole);
    }

    for (int i = 0; i < itemCount; ++i) {
        QString itemText = QLatin1String("setItemData(EditRole) ") + QString::number(i);
        QCOMPARE(comboBox.itemText(i), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::EditRole).toString(), itemText);

        comboBox.setCurrentIndex(i);
        QCOMPARE(comboBox.currentIndex(), i);
        QCOMPARE(comboBox.currentText(), itemText);
        QCOMPARE(comboBox.currentData(Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.currentData(Qt::EditRole).toString(), itemText);
    }

    comboBox.clear();


    // set additional user data in the addItem call
    for (int i = 0; i < itemCount; ++i) {
        const QString iS = QString::number(i);
        comboBox.addItem(QLatin1String("item text ") + iS, QVariant(QLatin1String("item data ") + iS));
    }

    for (int i = 0; i < itemCount; ++i) {
        const QString iS = QString::number(i);
        QString itemText = QLatin1String("item text ") + iS;
        QString itemDataText = QLatin1String("item data ") + iS;
        QCOMPARE(comboBox.itemData(i, Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::EditRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i).toString(), itemDataText);

        comboBox.setCurrentIndex(i);
        QCOMPARE(comboBox.currentIndex(), i);
        QCOMPARE(comboBox.currentData(Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.currentData(Qt::EditRole).toString(), itemText);
        QCOMPARE(comboBox.currentData().toString(), itemDataText);

    }

    comboBox.clear();


    // additional roles, setItemData
    // UserRole + 0 -> string
    // UserRole + 1 -> double
    // UserRole + 2 -> icon
    QString qtlogoPath = QFINDTESTDATA("qtlogo.png");
    QIcon icon = QIcon(QPixmap(qtlogoPath));
    for (int i = 0; i < itemCount; ++i) {
        const QString iS = QString::number(i);
        QString itemText = QLatin1String("item text ") + iS;
        QString itemDataText = QLatin1String("item data ") + iS;
        double d = i;
        comboBox.addItem(itemText);
        comboBox.setItemData(i, QVariant(itemDataText), Qt::UserRole);
        comboBox.setItemData(i, QVariant(d), Qt::UserRole + 1);
        comboBox.setItemData(i, QVariant::fromValue(icon), Qt::UserRole + 2);
    }

    for (int i = 0; i < itemCount; ++i) {
        const QString iS = QString::number(i);
        QString itemText = QLatin1String("item text ") + iS;
        QString itemDataText = QLatin1String("item data ") + iS;
        double d = i;
        QCOMPARE(comboBox.itemData(i, Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::EditRole).toString(), itemText);
        QCOMPARE(comboBox.itemData(i, Qt::UserRole).toString(), itemDataText);
        QCOMPARE(comboBox.itemData(i, Qt::UserRole + 1).toDouble(), d);
        QCOMPARE(comboBox.itemData(i, Qt::UserRole + 2).value<QIcon>(), icon);

        comboBox.setCurrentIndex(i);
        QCOMPARE(comboBox.currentData(Qt::DisplayRole).toString(), itemText);
        QCOMPARE(comboBox.currentData(Qt::EditRole).toString(), itemText);
        QCOMPARE(comboBox.currentData(Qt::UserRole).toString(), itemDataText);
        QCOMPARE(comboBox.currentData(Qt::UserRole + 1).toDouble(), d);
        QCOMPARE(comboBox.currentData(Qt::UserRole + 2).value<QIcon>(), icon);
    }
}

void tst_QComboBox::task_QTBUG_31146_popupCompletion()
{
    QComboBox comboBox;
    comboBox.setEditable(true);
    comboBox.setAutoCompletion(true);
    comboBox.setInsertPolicy(QComboBox::NoInsert);
    comboBox.completer()->setCaseSensitivity(Qt::CaseInsensitive);
    comboBox.completer()->setCompletionMode(QCompleter::PopupCompletion);

    comboBox.addItems(QStringList() << QStringLiteral("item") << QStringLiteral("item"));

    comboBox.show();
    comboBox.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&comboBox));

    QCOMPARE(comboBox.currentIndex(), 0);

    comboBox.lineEdit()->selectAll();
    QTest::keyClicks(comboBox.lineEdit(), "item");

    QTest::keyClick(comboBox.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(comboBox.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(comboBox.completer()->popup(), Qt::Key_Enter);
    QCOMPARE(comboBox.currentIndex(), 1);

    comboBox.lineEdit()->selectAll();
    QTest::keyClicks(comboBox.lineEdit(), "item");

    QTest::keyClick(comboBox.completer()->popup(), Qt::Key_Up);
    QTest::keyClick(comboBox.completer()->popup(), Qt::Key_Up);
    QTest::keyClick(comboBox.completer()->popup(), Qt::Key_Enter);
    QCOMPARE(comboBox.currentIndex(), 0);
}

void tst_QComboBox::task_QTBUG_41288_completerChangesCurrentIndex()
{
    QComboBox comboBox;
    comboBox.setEditable(true);

    comboBox.addItems(QStringList() << QStringLiteral("111") << QStringLiteral("222"));

    comboBox.show();
    comboBox.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&comboBox));

    {
        // change currentIndex() by keyboard
        comboBox.lineEdit()->selectAll();
        QTest::keyClicks(comboBox.lineEdit(), "222");
        QTest::keyClick(comboBox.lineEdit(), Qt::Key_Enter);
        QCOMPARE(comboBox.currentIndex(), 1);

        QTest::keyClick(&comboBox, Qt::Key_Up);
        comboBox.lineEdit()->selectAll();
        QTest::keyClick(comboBox.lineEdit(), Qt::Key_Enter);
        QCOMPARE(comboBox.currentIndex(), 0);
    }

    {
        // change currentIndex() programmatically
        comboBox.lineEdit()->selectAll();
        QTest::keyClicks(comboBox.lineEdit(), "222");
        QTest::keyClick(comboBox.lineEdit(), Qt::Key_Enter);
        QCOMPARE(comboBox.currentIndex(), 1);

        comboBox.setCurrentIndex(0);
        comboBox.lineEdit()->selectAll();
        QTest::keyClick(comboBox.lineEdit(), Qt::Key_Enter);
        QCOMPARE(comboBox.currentIndex(), 0);
    }
}

namespace {
    struct SetReadOnly {
        QComboBox *cb;
        explicit SetReadOnly(QComboBox *cb) : cb(cb) {}
        void operator()() const
        { cb->setEditable(false); }
    };
}

void tst_QComboBox::task_QTBUG_54191_slotOnEditTextChangedSetsComboBoxToReadOnly()
{
    QComboBox cb;
    cb.addItems(QStringList() << "one" << "two");
    cb.setEditable(true);
    cb.setCurrentIndex(0);

    connect(&cb, &QComboBox::editTextChanged,
            SetReadOnly(&cb));

    cb.setCurrentIndex(1);
    // the real test is that it didn't crash...
    QCOMPARE(cb.currentIndex(), 1);
}

void tst_QComboBox::keyboardSelection()
{
    QComboBox comboBox;
    const int keyboardInterval = QApplication::keyboardInputInterval();
    QStringList list;
    list << "OA" << "OB" << "OC" << "OO" << "OP" << "PP";
    comboBox.addItems(list);

    // Clear any remaining keyboard input from previous tests.
    QTest::qWait(keyboardInterval);
    QTest::keyClicks(&comboBox, "oo", Qt::NoModifier, 50);
    QCOMPARE(comboBox.currentText(), list.at(3));

    QTest::qWait(keyboardInterval);
    QTest::keyClicks(&comboBox, "op", Qt::NoModifier, 50);
    QCOMPARE(comboBox.currentText(), list.at(4));

    QTest::keyClick(&comboBox, Qt::Key_P, Qt::NoModifier, keyboardInterval);
    QCOMPARE(comboBox.currentText(), list.at(5));

    QTest::keyClick(&comboBox, Qt::Key_O, Qt::NoModifier, keyboardInterval);
    QCOMPARE(comboBox.currentText(), list.at(0));

    QTest::keyClick(&comboBox, Qt::Key_O, Qt::NoModifier, keyboardInterval);
    QCOMPARE(comboBox.currentText(), list.at(1));
}

void tst_QComboBox::updateDelegateOnEditableChange()
{

    QComboBox box;
    box.addItem(QStringLiteral("Foo"));
    box.addItem(QStringLiteral("Bar"));
    box.setEditable(false);

    QComboBoxPrivate *d = static_cast<QComboBoxPrivate *>(QComboBoxPrivate::get(&box));

    {
        bool menuDelegateBefore = qobject_cast<QComboMenuDelegate *>(box.itemDelegate()) != 0;
        d->updateDelegate();
        bool menuDelegateAfter = qobject_cast<QComboMenuDelegate *>(box.itemDelegate()) != 0;
        QCOMPARE(menuDelegateAfter, menuDelegateBefore);
    }

    box.setEditable(true);

    {
        bool menuDelegateBefore = qobject_cast<QComboMenuDelegate *>(box.itemDelegate()) != 0;
        d->updateDelegate();
        bool menuDelegateAfter = qobject_cast<QComboMenuDelegate *>(box.itemDelegate()) != 0;
        QCOMPARE(menuDelegateAfter, menuDelegateBefore);
    }
}

void tst_QComboBox::task_QTBUG_39088_inputMethodHints()
{
    QComboBox box;
    box.setEditable(true);
    box.setInputMethodHints(Qt::ImhNoPredictiveText);
    QCOMPARE(box.lineEdit()->inputMethodHints(), Qt::ImhNoPredictiveText);
}

void tst_QComboBox::respectChangedOwnershipOfItemView()
{
    QComboBox box1;
    QComboBox box2;
    QTableView *v1 = new QTableView;
    box1.setView(v1);

    QSignalSpy spy1(v1, SIGNAL(destroyed()));
    box2.setView(v1); // Ownership should now be transferred to box2


    QTableView *v2 = new QTableView(&box1);
    box1.setView(v2);  // Here we do not expect v1 to be deleted
    QApplication::processEvents();
    QCOMPARE(spy1.count(), 0);

    QSignalSpy spy2(v2, SIGNAL(destroyed()));
    box1.setView(v1);
    QCOMPARE(spy2.count(), 1);
}

void tst_QComboBox::task_QTBUG_49831_scrollerNotActivated()
{
    QStringList modelData;
    for (int i = 0; i < 1000; i++)
        modelData << QStringLiteral("Item %1").arg(i);
    QStringListModel model(modelData);

    QComboBox box;
    box.setModel(&model);
    box.setCurrentIndex(500);
    box.show();
    QTest::qWaitForWindowExposed(&box);
    QTest::mouseMove(&box, QPoint(5, 5), 100);
    box.showPopup();
    QFrame *container = box.findChild<QComboBoxPrivateContainer *>();
    QVERIFY(container);
    QTest::qWaitForWindowExposed(container);

    QList<QComboBoxPrivateScroller *> scrollers = container->findChildren<QComboBoxPrivateScroller *>();
    // Not all styles support scrollers. We rely only on those platforms that do to catch any regression.
    if (!scrollers.isEmpty()) {
        Q_FOREACH (QComboBoxPrivateScroller *scroller, scrollers) {
            if (scroller->isVisible()) {
                QSignalSpy doScrollSpy(scroller, SIGNAL(doScroll(int)));
                QTest::mouseMove(scroller, QPoint(5, 5), 500);
                QTRY_VERIFY(doScrollSpy.count() > 0);
            }
        }
    }
}

class QTBUG_56693_Model : public QStandardItemModel
{
public:
    QTBUG_56693_Model(QObject *parent = Q_NULLPTR)
        : QStandardItemModel(parent)
    { }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::FontRole) {
            if (index.row() < 5) {
                QFont font = QApplication::font();
                font.setItalic(true);
                return font;
            } else {
                return QApplication::font();
            }
        }
        return QStandardItemModel::data(index, role);
    }
};

class QTBUG_56693_ProxyStyle : public QProxyStyle
{
public:
    QTBUG_56693_ProxyStyle(QStyle *style)
        : QProxyStyle(style), italicItemsNo(0)
    {

    }

    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p, const QWidget *w = Q_NULLPTR) const override
    {
        if (element == CE_MenuItem)
            if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(opt))
                if (menuItem->font.italic())
                    italicItemsNo++;

        baseStyle()->drawControl(element, opt, p, w);
    }

    mutable int italicItemsNo;
};

void tst_QComboBox::task_QTBUG_56693_itemFontFromModel()
{
    QComboBox box;
    if (!qobject_cast<QComboMenuDelegate *>(box.itemDelegate()))
        QSKIP("Only for combo boxes using QComboMenuDelegate");

    QTBUG_56693_Model model;
    box.setModel(&model);

    QTBUG_56693_ProxyStyle *proxyStyle = new QTBUG_56693_ProxyStyle(box.style());
    box.setStyle(proxyStyle);
    box.setFont(QApplication::font());

    for (int i = 0; i < 10; i++)
        box.addItem(QLatin1String("Item ") + QString::number(i));

    box.show();
    QTest::qWaitForWindowExposed(&box);
    box.showPopup();
    QFrame *container = box.findChild<QComboBoxPrivateContainer *>();
    QVERIFY(container);
    QTest::qWaitForWindowExposed(container);

    QCOMPARE(proxyStyle->italicItemsNo, 5);

    box.hidePopup();
}

QTEST_MAIN(tst_QComboBox)
#include "tst_qcombobox.moc"
