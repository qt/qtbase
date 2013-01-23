/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtGui/qgraphicsview.h>
#include <QtGui/qpixmapcache.h>
#include <QtGui/qdesktopwidget.h>

#include "mainview.h"
#include "dummydatagen.h"
#include "simplelist.h"
#include "itemrecyclinglist.h"
#include "simplelist.h"
#include "theme.h"
#include "commandline.h"

class tst_GraphicsViewBenchmark : public QObject
{
    Q_OBJECT
public:
    enum ListType {
        Simple,
        Recycling,
        None
    };

    enum ScrollStep {
        Slow = 2,
        Normal = 8,
        Fast = 64
    };

    tst_GraphicsViewBenchmark(Settings *settings)
        : mSettings(settings), mMainView(0), currentListSize(-1), currentListType(None) {}
    ~tst_GraphicsViewBenchmark() {}

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

private slots:
    // Benchmarks:
    void createAndFillList_data();
    void createAndFillList();
    void add100ItemsToBeginningOfList_data();
    void add100ItemsToBeginningOfList();
    void remove100ItemsFromBeginningOfList_data();
    void remove100ItemsFromBeginningOfList();
    void deleteList_data();
    void deleteList();
    void themeChange_data();
    void themeChange();
    void update_data();
    void update();
    void scroll_data();
    void scroll();

private:
    Settings *mSettings;
    MainView *mMainView;
    DummyDataGenerator mDataGenerator;
    int currentListSize;
    ListType currentListType;

    void resetView();
    void ensureListSizeAndType(int listSize, ListType listType);
    void ensureTheme(Theme::Themes theme);
    void ensureRotationAngle(int rotation);
    void ensureSubtreeCache(bool enable);
    void ensureImageBasedRendering(bool enable);
    void insertListData();
    inline void setTestWidget(QGraphicsWidget *widget, int listSize, ListType listType)
    {
        currentListSize = listSize;
        currentListType = listType;
        mMainView->setTestWidget(widget);
    }
};

Q_DECLARE_METATYPE(tst_GraphicsViewBenchmark::ListType)
Q_DECLARE_METATYPE(Theme::Themes)
Q_DECLARE_METATYPE(tst_GraphicsViewBenchmark::ScrollStep)

const int AddRemoveCount = 100;

static ListItem *newSimpleListItem(DummyDataGenerator &dataGenerator, const int id)
{
    ListItem *item = new ListItem();
    item->setText(dataGenerator.randomName(), ListItem::FirstPos );
    item->setText(dataGenerator.randomPhoneNumber(QString("%1").arg(id)), ListItem::SecondPos );
    item->setIcon(new IconItem(dataGenerator.randomIconItem(), item), ListItem::LeftIcon );
    item->setIcon(new IconItem(dataGenerator.randomStatusItem(), item), ListItem::RightIcon);
    item->setFont(Theme::p()->font(Theme::ContactName), ListItem::FirstPos);
    item->setFont(Theme::p()->font(Theme::ContactNumber), ListItem::SecondPos);
    item->setBorderPen(Theme::p()->listItemBorderPen());
    item->setRounding(Theme::p()->listItemRounding());
    item->icon(ListItem::LeftIcon)->setRotation(Theme::p()->iconRotation(ListItem::LeftIcon));
    item->icon(ListItem::RightIcon)->setRotation(Theme::p()->iconRotation(ListItem::RightIcon));
    item->icon(ListItem::LeftIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::LeftIcon));
    item->icon(ListItem::RightIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::RightIcon));
    item->icon(ListItem::LeftIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::LeftIcon));
    item->icon(ListItem::RightIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::RightIcon));
    return item;
}

static RecycledListItem *newRecyclingListItem(DummyDataGenerator &dataGenerator, const int id)
{
    RecycledListItem *item = new RecycledListItem();
    item->item()->setText(dataGenerator.randomName(), ListItem::FirstPos );
    item->item()->setText(dataGenerator.randomPhoneNumber(QString("%1").arg(id)), ListItem::SecondPos );
    item->item()->setIcon(new IconItem(dataGenerator.randomIconItem()), ListItem::LeftIcon );
    item->item()->setIcon(new IconItem(dataGenerator.randomStatusItem()), ListItem::RightIcon);
    item->item()->setFont(Theme::p()->font(Theme::ContactName), ListItem::FirstPos);
    item->item()->setFont(Theme::p()->font(Theme::ContactNumber), ListItem::SecondPos);
    item->item()->setBorderPen(Theme::p()->listItemBorderPen());
    item->item()->setRounding(Theme::p()->listItemRounding());
    item->item()->icon(ListItem::LeftIcon)->setRotation(Theme::p()->iconRotation(ListItem::LeftIcon));
    item->item()->icon(ListItem::RightIcon)->setRotation(Theme::p()->iconRotation(ListItem::RightIcon));
    item->item()->icon(ListItem::LeftIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::LeftIcon));
    item->item()->icon(ListItem::RightIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::RightIcon));
    item->item()->icon(ListItem::LeftIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::LeftIcon));
    item->item()->icon(ListItem::RightIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::RightIcon));
    return item;
}

static void fillList(DummyDataGenerator &dataGenerator, int itemCount, QGraphicsWidget *list)
{
    if (SimpleList *simpleList = qobject_cast<SimpleList *>(list)) {
        for (int i = 0; i < itemCount; ++i)
            simpleList->addItem(newSimpleListItem(dataGenerator, i));
    } else if (ItemRecyclingList *recyclingList = qobject_cast<ItemRecyclingList *>(list)) {
        for (int i = 0; i < itemCount; ++i)
            recyclingList->addItem(newRecyclingListItem(dataGenerator, i));
    } else {
        qFatal("fillList: internal error");
    }
}

void tst_GraphicsViewBenchmark::resetView()
{
    if (QGraphicsWidget *widget = mMainView->takeTestWidget()) {
        delete widget;
        currentListSize = -1;
        currentListType = None;
        QTest::qWait(50);
    } else {
        if (currentListSize != -1)
            qFatal("tst_GraphicsViewBenchmark::resetView: internal error: wrong list size");
        if (currentListType != None)
            qFatal("tst_GraphicsViewBenchmark::resetView: internal error: wrong list type");
    }
    ensureTheme(Theme::Blue);
    ensureRotationAngle(0);
    ensureSubtreeCache(false);
    ensureImageBasedRendering(false);
}

void tst_GraphicsViewBenchmark::ensureListSizeAndType(int listSize, ListType listType)
{
    if (currentListSize != listSize || currentListType != listType) {
        resetView();
        if (listType == Simple) {
            SimpleList *list = new SimpleList;
            fillList(mDataGenerator, listSize, list);
            setTestWidget(list, listSize, listType);
        } else if (listType == Recycling) {
            ItemRecyclingList *list = new ItemRecyclingList;
            fillList(mDataGenerator, listSize, list);
            setTestWidget(list, listSize, listType);
        }
        QTest::qWait(50);
        return;
    }

    // Okay, we're supposed to have the right list type and size. Make sure we actually have it.
    QGraphicsWidget *widget = mMainView->testWidget();
    if (!widget) {
        if (currentListType != None || currentListSize != -1)
            qFatal("tst_GraphicsViewBenchmark::ensureListSizeAndType: internal error: no test widget");
        return;
    }

    if (listType == Simple) {
        SimpleList *list = qobject_cast<SimpleList *>(widget);
        if (!list)
            qFatal("tst_GraphicsViewBenchmark::ensureListSizeAndType: internal error: wrong list type");
        if (list->itemCount() != listSize)
            qFatal("tst_GraphicsViewBenchmark::ensureListSizeAndType: internal error: wrong list size");
    } else if (listType == Recycling){
        ItemRecyclingList *list = qobject_cast<ItemRecyclingList *>(widget);
        if (!list)
            qFatal("tst_GraphicsViewBenchmark::ensureListSizeAndType: internal error: wrong list type");
        if (list->rows() != listSize)
            qFatal("tst_GraphicsViewBenchmark::ensureListSizeAndType: internal error: wrong list size");
    }
}

void tst_GraphicsViewBenchmark::ensureTheme(Theme::Themes theme)
{
    if (Theme::p()->theme() != theme) {
        Theme::p()->setTheme(theme);
        // The theme change itself can take a lot of time, so make
        // sure we give it a little bit time to stabilize *after*
        // the changes, hence sendPostedEvents(); qWait();
        QApplication::sendPostedEvents();
        QTest::qWait(50);
    }
}

void tst_GraphicsViewBenchmark::ensureRotationAngle(int angle)
{
    const bool useTwoColumns = angle != 0;
    bool wait = false;
    if (mMainView->rotationAngle() != angle) {
        mMainView->rotateContent(-mMainView->rotationAngle() + angle);
        wait = true;
    }
    if (QGraphicsWidget *widget = mMainView->testWidget()) {
        if (SimpleList *list = qobject_cast<SimpleList *>(widget)) {
            if (list->twoColumns() != useTwoColumns) {
                list->setTwoColumns(useTwoColumns);
                wait = true;
            }
        } else if (ItemRecyclingList *list = qobject_cast<ItemRecyclingList *>(widget)) {
            if (list->twoColumns() != useTwoColumns) {
                list->setTwoColumns(useTwoColumns);
                wait = true;
            }
        }
    }
    if (wait)
        QTest::qWait(50);
}

void tst_GraphicsViewBenchmark::ensureSubtreeCache(bool enable)
{
    QGraphicsWidget *widget = mMainView->testWidget();
    if (!widget)
        return;

    if (SimpleList *list = qobject_cast<SimpleList *>(widget)) {
        if (list->listItemCaching() != enable) {
            list->setListItemCaching(enable);
            QTest::qWait(50);
        }
    } else if (ItemRecyclingList *list = qobject_cast<ItemRecyclingList *>(widget)) {
        if (list->listItemCaching() != enable) {
            list->setListItemCaching(enable);
            QTest::qWait(50);
        }
    }
    QPixmapCache::clear();
}

void tst_GraphicsViewBenchmark::ensureImageBasedRendering(bool enable)
{
    if (mMainView->imageBasedRendering() != enable) {
        mMainView->setImageBasedRendering(enable);
        QTest::qWait(50);
    }
}

void tst_GraphicsViewBenchmark::insertListData()
{
    QTest::addColumn<int>("listSize");
    QTest::addColumn<ListType>("listType");

    QTest::newRow("Simple list containing 10 items") << 10 << Simple;
    QTest::newRow("Recycling list containing 10 items") << 10 << Recycling;
    QTest::newRow("Simple list containing 50 items") << 50 << Simple;
    QTest::newRow("Recycling list containing 50 items") << 50 << Recycling;
    QTest::newRow("Simple list containing 500 items") << 500 << Simple;
    QTest::newRow("Recycling list containing 500 items") << 500 << Recycling;
}

void tst_GraphicsViewBenchmark::initTestCase()
{
    mMainView = new MainView(mSettings->options() & Settings::UseOpenGL,
                             mSettings->options() & Settings::OutputFps);

    if (mSettings->size().width() > 0 && mSettings->size().height() > 0) {
        mMainView->resize(mSettings->size().width(), mSettings->size().height());
        mMainView->show();
    } else if (QApplication::desktop()->width() < 360 || QApplication::desktop()->height() < 640) {
        mMainView->showFullScreen();
    } else {
        mMainView->resize(360, 640);
        mMainView->show();
    }

    mDataGenerator.Reset();
    SimpleList *list = new SimpleList;
    list->setListItemCaching(false);
    mMainView->setTestWidget(list);
    fillList(mDataGenerator, 5, list);
    mMainView->takeTestWidget();
    delete list;

    currentListSize = -1;
    currentListType = None;

    QTest::qWaitForWindowShown(mMainView);
}

void tst_GraphicsViewBenchmark::cleanupTestCase()
{
    delete mMainView;
    mMainView = 0;
}

void tst_GraphicsViewBenchmark::init()
{
    // Make sure we don't have pending events in the queue.
    // Yes, each test run takes a little bit longer, but the results are more stable.
    QTest::qWait(150);
}

void tst_GraphicsViewBenchmark::createAndFillList_data()
{
    insertListData();
}

void tst_GraphicsViewBenchmark::createAndFillList()
{
    QFETCH(int, listSize);
    QFETCH(ListType, listType);

    resetView();

    if (listType == Simple) {
        QBENCHMARK {
            SimpleList *list = new SimpleList;
            setTestWidget(list, listSize, listType);
            fillList(mDataGenerator, listSize, list);
        }
    } else {
        QBENCHMARK {
            ItemRecyclingList *list = new ItemRecyclingList;
            setTestWidget(list, listSize, listType);
            fillList(mDataGenerator, listSize, list);
        }
    }

    resetView();
}

void tst_GraphicsViewBenchmark::add100ItemsToBeginningOfList_data()
{
    insertListData();
}

void tst_GraphicsViewBenchmark::add100ItemsToBeginningOfList()
{
    QFETCH(int, listSize);
    QFETCH(ListType, listType);

    resetView();

    if (listType == Simple) {
        SimpleList *list = new SimpleList;
        fillList(mDataGenerator, listSize, list);
        setTestWidget(list, listSize, listType);
        QTest::qWait(50);
        QBENCHMARK {
            for (int i = 0; i < AddRemoveCount; ++i)
                list->insertItem(0, newSimpleListItem(mDataGenerator, i));
        }
    } else {
        ItemRecyclingList *list = new ItemRecyclingList;
        fillList(mDataGenerator, listSize, list);
        setTestWidget(list, listSize, listType);
        QTest::qWait(50);
        QBENCHMARK {
            for (int i = 0; i < AddRemoveCount; ++i)
                list->insertItem(0, newRecyclingListItem(mDataGenerator, i));
        }
    }

    resetView();
}

void tst_GraphicsViewBenchmark::remove100ItemsFromBeginningOfList_data()
{
    insertListData();
}

void tst_GraphicsViewBenchmark::remove100ItemsFromBeginningOfList()
{
    QFETCH(int, listSize);
    QFETCH(ListType, listType);

    resetView();

    if (listType == Simple) {
        SimpleList *list = new SimpleList;
        fillList(mDataGenerator, listSize, list);
        setTestWidget(list, listSize, listType);
        QTest::qWait(50);
        QBENCHMARK {
            for (int i = 0; i < AddRemoveCount; ++i)
                delete list->takeItem(0);
        }
    } else {
        ItemRecyclingList *list = new ItemRecyclingList;
        fillList(mDataGenerator, listSize, list);
        setTestWidget(list, listSize, listType);
        QTest::qWait(50);
        QBENCHMARK {
            for (int i = 0; i < AddRemoveCount; ++i)
                delete list->takeItem(0);
        }
    }

    resetView();
}

void tst_GraphicsViewBenchmark::deleteList_data()
{
    insertListData();
    QTest::newRow("Simple list containing 1000 items") << 1000 << Simple;
    QTest::newRow("Recycling list containing 1000 items") << 1000 << Recycling;
}

void tst_GraphicsViewBenchmark::deleteList()
{
    QFETCH(int, listSize);
    QFETCH(ListType, listType);

    if (listSize < 500)
        return; // Too small to measure.

    QGraphicsWidget *list = 0;
    if (listType == Simple)
        list = new SimpleList;
    else
        list = new ItemRecyclingList;
    fillList(mDataGenerator, listSize, list);
    QTest::qWait(20);

    QBENCHMARK_ONCE {
        delete list;
    }
}

void tst_GraphicsViewBenchmark::themeChange_data()
{
    QTest::addColumn<int>("listSize");
    QTest::addColumn<ListType>("listType");
    QTest::addColumn<Theme::Themes>("fromTheme");
    QTest::addColumn<Theme::Themes>("toTheme");

    QTest::newRow("From Blue to Lime, simple list containing 10 items") << 10 << Simple << Theme::Blue << Theme::Lime;
    QTest::newRow("From Lime to Blue, simple list containing 10 items") << 10 << Simple << Theme::Lime << Theme::Blue;

    QTest::newRow("From Blue to Lime, recycling list containing 10 items") << 10 << Recycling << Theme::Blue << Theme::Lime;
    QTest::newRow("From Lime to Blue, recycling list containing 10 items") << 10 << Recycling << Theme::Lime << Theme::Blue;

    QTest::newRow("From Blue to Lime, simple list containing 50 items") << 50 << Simple << Theme::Blue << Theme::Lime;
    QTest::newRow("From Lime to Blue, simple list containing 50 items") << 50 << Simple << Theme::Lime << Theme::Blue;

    QTest::newRow("From Blue to Lime, recycling list containing 50 items") << 50 << Recycling << Theme::Blue << Theme::Lime;
    QTest::newRow("From Lime to Blue, recycling list containing 50 items") << 50 << Recycling << Theme::Lime << Theme::Blue;

    QTest::newRow("From Blue to Lime, simple list containing 500 items") << 500 << Simple << Theme::Blue << Theme::Lime;
    QTest::newRow("From Lime to Blue, simple list containing 500 items") << 500 << Simple << Theme::Lime << Theme::Blue;

    QTest::newRow("From Blue to Lime, recycling list containing 500 items") << 500 << Recycling << Theme::Blue << Theme::Lime;
    QTest::newRow("From Lime to Blue, recycling list containing 500 items") << 500 << Recycling << Theme::Lime << Theme::Blue;
}

void tst_GraphicsViewBenchmark::themeChange()
{
    QFETCH(int, listSize);
    QFETCH(ListType, listType);
    QFETCH(Theme::Themes, fromTheme);
    QFETCH(Theme::Themes, toTheme);

    if (fromTheme == toTheme)
        qFatal("tst_GraphicsViewBenchmark::themeChange: to and from theme is the same");

    ensureListSizeAndType(listSize, listType);
    ensureTheme(fromTheme);

    QBENCHMARK {
        Theme::p()->setTheme(toTheme);
    }
}

static inline QLatin1String stringForTheme(Theme::Themes theme)
{
    if (theme == Theme::Blue)
        return QLatin1String("Blue");
    return QLatin1String("Lime");
}

static inline QLatin1String stringForListType(tst_GraphicsViewBenchmark::ListType type)
{
    if (type == tst_GraphicsViewBenchmark::Simple)
        return QLatin1String("Simple");
    if (type == tst_GraphicsViewBenchmark::Recycling)
        return QLatin1String("Recycling");
    return QLatin1String("None");
}

static inline QLatin1String stringForScrollStep(tst_GraphicsViewBenchmark::ScrollStep step)
{
    if (step == tst_GraphicsViewBenchmark::Slow)
        return QLatin1String("Slow");
    if (step == tst_GraphicsViewBenchmark::Normal)
        return QLatin1String("Normal");
    return QLatin1String("Fast");
}

static inline QString rowString(int listSize, tst_GraphicsViewBenchmark::ListType listType,
                                Theme::Themes theme, int toImage, int cache, int angle)
{
    return QString("Items=%1, List=%2, Theme=%3, RenderToImage=%4, Cache=%5, RotAngle=%6")
           .arg(QString::number(listSize)).arg(stringForListType(listType))
           .arg(stringForTheme(theme)).arg(QString::number(toImage))
           .arg(QString::number(cache)).arg(QString::number(angle));
}

static inline QString rowString(int listSize, tst_GraphicsViewBenchmark::ListType listType,
                                Theme::Themes theme, int cache, int angle,
                                tst_GraphicsViewBenchmark::ScrollStep step)
{
    return QString("Items=%1, List=%2, Theme=%3, Cache=%4, RotAngle=%5, Speed=%6")
           .arg(QString::number(listSize)).arg(stringForListType(listType))
           .arg(stringForTheme(theme)).arg(QString::number(cache))
           .arg(QString::number(angle)).arg(stringForScrollStep(step));
}

void tst_GraphicsViewBenchmark::update_data()
{
    QTest::addColumn<int>("listSize");
    QTest::addColumn<ListType>("listType");
    QTest::addColumn<Theme::Themes>("theme");
    QTest::addColumn<bool>("renderToImage");
    QTest::addColumn<bool>("subtreeCache");
    QTest::addColumn<int>("rotationAngle");

    QList<ListType> listTypes;
    listTypes << Simple << Recycling;

    QList<int> listSizes;
    listSizes << 10 << 50 << 500;

    QList<Theme::Themes> themes;
    themes << Theme::Blue << Theme::Lime;

    QList<int> rotationAngles;
    rotationAngles << 0 << 90;

    // Generate rows:
    foreach (ListType listType, listTypes) {
        foreach (int listSize, listSizes) {
            foreach (int angle, rotationAngles) {
                foreach (Theme::Themes theme, themes) {
                    for (int toImage = 0; toImage < 2; ++toImage) {
                        for (int cache = 0; cache < 2; ++cache) {
                            QString string = rowString(listSize, listType, theme, toImage, cache, angle);
                            QTest::newRow(string.toLatin1()) << listSize << listType << theme << bool(toImage)
                                                             << bool(cache) << angle;
                        }
                    }
                }
            }
        }
    }
}

void tst_GraphicsViewBenchmark::update()
{
    QFETCH(int, listSize);
    QFETCH(ListType, listType);
    QFETCH(Theme::Themes, theme);
    QFETCH(bool, renderToImage);
    QFETCH(bool, subtreeCache);
    QFETCH(int, rotationAngle);

    mMainView->viewport()->setUpdatesEnabled(false);

    ensureListSizeAndType(listSize, listType);
    ensureTheme(theme);
    ensureRotationAngle(rotationAngle);
    ensureSubtreeCache(subtreeCache);
    ensureImageBasedRendering(renderToImage);

    QEventLoop loop;
    QObject::connect(mMainView, SIGNAL(repainted()), &loop, SLOT(quit()));
    QTimer::singleShot(4000, &loop, SLOT(quit()));
    // Dry run (especially important when cache is enabled).
    // NB! setUpdatesEnabled triggers an update().
    mMainView->viewport()->setUpdatesEnabled(true);
    loop.exec(QEventLoop::AllEvents | QEventLoop::ExcludeUserInputEvents| QEventLoop::ExcludeSocketNotifiers);
    QTest::qWait(50);

    QTimer::singleShot(4000, &loop, SLOT(quit()));
    QBENCHMARK {
        mMainView->viewport()->update();
        loop.exec(QEventLoop::AllEvents | QEventLoop::ExcludeUserInputEvents| QEventLoop::ExcludeSocketNotifiers);
    }
}

void tst_GraphicsViewBenchmark::scroll_data()
{
    QTest::addColumn<int>("listSize");
    QTest::addColumn<ListType>("listType");
    QTest::addColumn<Theme::Themes>("theme");
    QTest::addColumn<bool>("subtreeCache");
    QTest::addColumn<int>("rotationAngle");
    QTest::addColumn<ScrollStep>("scrollStep");

    QList<ListType> listTypes;
    listTypes << Simple << Recycling;

    QList<int> listSizes;
    listSizes << 10 << 50 << 500;

    QList<Theme::Themes> themes;
    themes << Theme::Blue << Theme::Lime;

    QList<int> rotationAngles;
    rotationAngles << 0 << 90;

    QList<ScrollStep> scrollSteps;
    scrollSteps << Slow << Normal << Fast;

    // Generate rows:
    foreach (ListType listType, listTypes) {
        foreach (int listSize, listSizes) {
            foreach (int angle, rotationAngles) {
                foreach (ScrollStep step, scrollSteps) {
                    foreach (Theme::Themes theme, themes) {
                        for (int cache = 0; cache < 2; ++cache) {
                            QString string = rowString(listSize, listType, theme, cache, angle, step);
                            QTest::newRow(string.toLatin1()) << listSize << listType << theme
                                                             << bool(cache) << angle << step;
                        }
                    }
                }
            }
        }
    }
}

void tst_GraphicsViewBenchmark::scroll()
{
    QFETCH(int, listSize);
    QFETCH(ListType, listType);
    QFETCH(Theme::Themes, theme);
    QFETCH(bool, subtreeCache);
    QFETCH(int, rotationAngle);
    QFETCH(ScrollStep, scrollStep);

    mMainView->viewport()->setUpdatesEnabled(false);

    ensureListSizeAndType(listSize, listType);
    ensureTheme(theme);
    ensureRotationAngle(rotationAngle);
    ensureSubtreeCache(subtreeCache);
    ensureImageBasedRendering(false);

    ScrollBar *sb = 0;
    if (listType == Simple)
        sb = static_cast<SimpleList *>(mMainView->testWidget())->verticalScrollBar();
    else
        sb = static_cast<ItemRecyclingList *>(mMainView->testWidget())->verticalScrollBar();
    const qreal sliderStart = sb->sliderSize() / qreal(2.0);
    const qreal sliderTarget = sliderStart + qreal(scrollStep);
    sb->setSliderPosition(sliderStart);

    QEventLoop loop;
    QObject::connect(mMainView, SIGNAL(repainted()), &loop, SLOT(quit()));
    QTimer::singleShot(4000, &loop, SLOT(quit()));
    // Dry run (especially important when cache is enabled).
    // NB! setUpdatesEnabled triggers an update().
    mMainView->viewport()->setUpdatesEnabled(true);
    loop.exec(QEventLoop::AllEvents | QEventLoop::ExcludeUserInputEvents| QEventLoop::ExcludeSocketNotifiers);
    QTest::qWait(50);

    QTimer::singleShot(4000, &loop, SLOT(quit()));
    QBENCHMARK {
        sb->setSliderPosition(sliderTarget);
        loop.exec(QEventLoop::AllEvents | QEventLoop::ExcludeUserInputEvents| QEventLoop::ExcludeSocketNotifiers);
    }
}

int main(int argc, char *argv[])
{
    Settings settings;
    if (!readSettingsFromCommandLine(argc, argv, settings))
        return 1;

    // Eat command line arguments.
    int aargc = 0;
    for (int i = 0; i < argc; ++i) {
        if (argv[i])
            ++aargc;
    }
    char **aargv = new char*[aargc];
    aargc = 0;
    for (int i = 0; i < argc; ++i) {
        if (argv[i])
            aargv[aargc++] = argv[i];
    }

    QApplication app(aargc, aargv);

    int returnValue = 0;
    if (settings.options() & Settings::ManualTest) {
        MainView view(settings.options() & Settings::UseOpenGL, settings.options() & Settings::OutputFps);

        DummyDataGenerator dataGenerator;
        dataGenerator.Reset();

        SimpleList *list = new SimpleList;
        if (settings.options() & Settings::UseListItemCache)
            list->setListItemCaching(true);
        else
            list->setListItemCaching(false);

        if (settings.listItemCount())
            fillList(dataGenerator, settings.listItemCount(), list);
        else
            fillList(dataGenerator, 500, list);

        view.setTestWidget(list);

        if ((settings.angle() % 360) != 0)
            view.rotateContent(settings.angle());

        if (settings.size().width() > 0 && settings.size().height() > 0) {
            view.resize(settings.size().width(), settings.size().height());
            view.show();
        } else if (QApplication::desktop()->width() < 360 || QApplication::desktop()->height() < 640) {
            view.showFullScreen();
        } else {
            view.resize(360, 640);
            view.show();
        }
        returnValue = app.exec();
    } else {
        QTEST_DISABLE_KEYPAD_NAVIGATION
        tst_GraphicsViewBenchmark tc(&settings);
        returnValue = QTest::qExec(&tc, aargc, aargv);
    }

    delete [] aargv;
    return returnValue;
}

#include "main.moc"
