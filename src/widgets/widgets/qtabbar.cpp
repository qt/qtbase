// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qlayoutengine_p.h"
#if QT_CONFIG(itemviews)
#include "qabstractitemdelegate.h"
#endif
#include "qapplication.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylepainter.h"
#if QT_CONFIG(tabwidget)
#include "qtabwidget.h"
#endif
#if QT_CONFIG(tooltip)
#include "qtooltip.h"
#endif
#if QT_CONFIG(whatsthis)
#include "qwhatsthis.h"
#endif
#include "private/qtextengine_p.h"
#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif
#ifdef Q_OS_MACOS
#include <qpa/qplatformnativeinterface.h>
#endif

#include "qdebug.h"
#include "private/qapplication_p.h"
#include "private/qtabbar_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
class CloseButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit CloseButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override
        { return sizeHint(); }
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};
}

QMovableTabWidget::QMovableTabWidget(QWidget *parent)
    : QWidget(parent)
{
}

void QMovableTabWidget::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    update();
}

void QMovableTabWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.drawPixmap(0, 0, m_pixmap);
}

void QTabBarPrivate::updateMacBorderMetrics()
{
#if defined(Q_OS_MACOS)
    Q_Q(QTabBar);
    // Extend the unified title and toolbar area to cover the tab bar iff
    // 1) the tab bar is in document mode
    // 2) the tab bar is directly below an "unified" area.
    // The extending itself is done in the Cocoa platform plugin and Mac style,
    // this function registers geometry and visibility state for the tab bar.

    // Calculate geometry
    int upper, lower;
    if (documentMode) {
        QPoint windowPos = q->mapTo(q->window(), QPoint(0,0));
        upper = windowPos.y();
        int tabStripHeight = q->tabSizeHint(0).height();
        int pixelTweak = -3;
        lower = upper + tabStripHeight + pixelTweak;
    } else {
        upper = 0;
        lower = 0;
    }

    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    if (!nativeInterface)
        return;
    quintptr identifier = reinterpret_cast<quintptr>(q);

    // Set geometry
    QPlatformNativeInterface::NativeResourceForIntegrationFunction function =
        nativeInterface->nativeResourceFunctionForIntegration("registerContentBorderArea");
    if (!function)
        return; // Not Cocoa platform plugin.
    typedef void (*RegisterContentBorderAreaFunction)(QWindow *window, quintptr identifier, int upper, int lower);
    (reinterpret_cast<RegisterContentBorderAreaFunction>(function))(q->window()->windowHandle(), identifier, upper, lower);

    // Set visibility state
    function = nativeInterface->nativeResourceFunctionForIntegration("setContentBorderAreaEnabled");
    if (!function)
        return;
    typedef void (*SetContentBorderAreaEnabledFunction)(QWindow *window, quintptr identifier, bool enable);
    (reinterpret_cast<SetContentBorderAreaEnabledFunction>(function))(q->window()->windowHandle(), identifier, q->isVisible());
#endif
}

/*!
    \internal
    This is basically QTabBar::initStyleOption() but
    without the expensive QFontMetrics::elidedText() call.
*/

void QTabBarPrivate::initBasicStyleOption(QStyleOptionTab *option, int tabIndex) const
{
    Q_Q(const QTabBar);
    const int totalTabs = tabList.size();

    if (!option || (tabIndex < 0 || tabIndex >= totalTabs))
        return;

    const QTabBarPrivate::Tab &tab = *tabList.at(tabIndex);
    option->initFrom(q);
    option->state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
    option->rect = q->tabRect(tabIndex);
    const bool isCurrent = tabIndex == currentIndex;
    option->row = 0;
    if (tabIndex == pressedIndex)
        option->state |= QStyle::State_Sunken;
    if (isCurrent)
        option->state |= QStyle::State_Selected;
    if (isCurrent && q->hasFocus())
        option->state |= QStyle::State_HasFocus;
    if (!tab.enabled)
        option->state &= ~QStyle::State_Enabled;
    if (q->isActiveWindow())
        option->state |= QStyle::State_Active;
    if (!dragInProgress && option->rect == hoverRect)
        option->state |= QStyle::State_MouseOver;
    option->shape = shape;
    option->text = tab.text;

    if (tab.textColor.isValid())
        option->palette.setColor(q->foregroundRole(), tab.textColor);
    option->icon = tab.icon;
    option->iconSize = q->iconSize();  // Will get the default value then.

    option->leftButtonSize = tab.leftWidget ? tab.leftWidget->size() : QSize();
    option->rightButtonSize = tab.rightWidget ? tab.rightWidget->size() : QSize();
    option->documentMode = documentMode;

    if (tabIndex > 0 && tabIndex - 1 == currentIndex)
        option->selectedPosition = QStyleOptionTab::PreviousIsSelected;
    else if (tabIndex + 1 < totalTabs && tabIndex + 1 == currentIndex)
        option->selectedPosition = QStyleOptionTab::NextIsSelected;
    else
        option->selectedPosition = QStyleOptionTab::NotAdjacent;

    const bool paintBeginning = (tabIndex == firstVisible) || (dragInProgress && tabIndex == pressedIndex + 1);
    const bool paintEnd = (tabIndex == lastVisible) || (dragInProgress && tabIndex == pressedIndex - 1);
    if (paintBeginning) {
        if (paintEnd)
            option->position = QStyleOptionTab::OnlyOneTab;
        else
            option->position = QStyleOptionTab::Beginning;
    } else if (paintEnd) {
        option->position = QStyleOptionTab::End;
    } else {
        option->position = QStyleOptionTab::Middle;
    }

#if QT_CONFIG(tabwidget)
    if (const QTabWidget *tw = qobject_cast<const QTabWidget *>(q->parentWidget())) {
        option->features |= QStyleOptionTab::HasFrame;
        if (tw->cornerWidget(Qt::TopLeftCorner) || tw->cornerWidget(Qt::BottomLeftCorner))
            option->cornerWidgets |= QStyleOptionTab::LeftCornerWidget;
        if (tw->cornerWidget(Qt::TopRightCorner) || tw->cornerWidget(Qt::BottomRightCorner))
            option->cornerWidgets |= QStyleOptionTab::RightCornerWidget;
    }
#endif
    option->tabIndex = tabIndex;
}

/*!
    Initialize \a option with the values from the tab at \a tabIndex. This method
    is useful for subclasses when they need a QStyleOptionTab,
    but don't want to fill in all the information themselves.

    \sa QStyleOption::initFrom(), QTabWidget::initStyleOption()
*/
void QTabBar::initStyleOption(QStyleOptionTab *option, int tabIndex) const
{
    Q_D(const QTabBar);
    d->initBasicStyleOption(option, tabIndex);

    QRect textRect = style()->subElementRect(QStyle::SE_TabBarTabText, option, this);
    option->text = fontMetrics().elidedText(option->text, d->elideMode, textRect.width(),
                        Qt::TextShowMnemonic);
}

/*!
    \class QTabBar
    \brief The QTabBar class provides a tab bar, e.g. for use in tabbed dialogs.

    \ingroup basicwidgets
    \inmodule QtWidgets

    QTabBar is straightforward to use; it draws the tabs using one of
    the predefined \l{QTabBar::Shape}{shapes}, and emits a
    signal when a tab is selected. It can be subclassed to tailor the
    look and feel. Qt also provides a ready-made \l{QTabWidget}.

    Each tab has a tabText(), an optional tabIcon(), an optional
    tabToolTip(), optional tabWhatsThis() and optional tabData().
    The tabs's attributes can be changed with setTabText(), setTabIcon(),
    setTabToolTip(), setTabWhatsThis and setTabData(). Each tabs can be
    enabled or disabled individually with setTabEnabled().

    Each tab can display text in a distinct color. The current text color
    for a tab can be found with the tabTextColor() function. Set the text
    color for a particular tab with setTabTextColor().

    Tabs are added using addTab(), or inserted at particular positions
    using insertTab(). The total number of tabs is given by
    count(). Tabs can be removed from the tab bar with
    removeTab(). Combining removeTab() and insertTab() allows you to
    move tabs to different positions.

    The \l shape property defines the tabs' appearance. The choice of
    shape is a matter of taste, although tab dialogs (for preferences
    and similar) invariably use \l RoundedNorth.
    Tab controls in windows other than dialogs almost
    always use either \l RoundedSouth or \l TriangularSouth. Many
    spreadsheets and other tab controls in which all the pages are
    essentially similar use \l TriangularSouth, whereas \l
    RoundedSouth is used mostly when the pages are different (e.g. a
    multi-page tool palette). The default in QTabBar is \l
    RoundedNorth.

    The most important part of QTabBar's API is the currentChanged()
    signal.  This is emitted whenever the current tab changes (even at
    startup, when the current tab changes from 'none'). There is also
    a slot, setCurrentIndex(), which can be used to select a tab
    programmatically. The function currentIndex() returns the index of
    the current tab, \l count holds the number of tabs.

    QTabBar creates automatic mnemonic keys in the manner of QAbstractButton;
    e.g. if a tab's label is "\&Graphics", Alt+G becomes a shortcut
    key for switching to that tab.

    The following virtual functions may need to be reimplemented in
    order to tailor the look and feel or store extra data with each
    tab:

    \list
    \li tabSizeHint() calcuates the size of a tab.
    \li tabInserted() notifies that a new tab was added.
    \li tabRemoved() notifies that a tab was removed.
    \li tabLayoutChange() notifies that the tabs have been re-laid out.
    \li paintEvent() paints all tabs.
    \endlist

    For subclasses, you might also need the tabRect() functions which
    returns the visual geometry of a single tab.

    \table 100%
    \row \li \inlineimage fusion-tabbar.png Screenshot of a Fusion style tab bar
         \li A tab bar shown in the \l{Qt Widget Gallery}{Fusion widget style}.
    \row \li \inlineimage fusion-tabbar-truncated.png Screenshot of a truncated Fusion tab bar
         \li A truncated tab bar shown in the Fusion widget style.
    \endtable

    \sa QTabWidget
*/

/*!
    \enum QTabBar::Shape

    This enum type lists the built-in shapes supported by QTabBar. Treat these
    as hints as some styles may not render some of the shapes. However,
    position should be honored.

    \value RoundedNorth  The normal rounded look above the pages

    \value RoundedSouth  The normal rounded look below the pages

    \value RoundedWest  The normal rounded look on the left side of the pages

    \value RoundedEast  The normal rounded look on the right side the pages

    \value TriangularNorth  Triangular tabs above the pages.

    \value TriangularSouth  Triangular tabs similar to those used in
    the Excel spreadsheet, for example

    \value TriangularWest  Triangular tabs on the left of the pages.

    \value TriangularEast  Triangular tabs on the right of the pages.
*/

/*!
    \fn void QTabBar::currentChanged(int index)

    This signal is emitted when the tab bar's current tab changes. The
    new current has the given \a index, or -1 if there isn't a new one
    (for example, if there are no tab in the QTabBar)
*/

/*!
    \fn void QTabBar::tabCloseRequested(int index)
    \since 4.5

    This signal is emitted when the close button on a tab is clicked.
    The \a index is the index that should be removed.

    \sa setTabsClosable()
*/

/*!
    \fn void QTabBar::tabMoved(int from, int to)
    \since 4.5

    This signal is emitted when the tab has moved the tab
    at index position \a from to index position \a to.

    note: QTabWidget will automatically move the page when
    this signal is emitted from its tab bar.

    \sa moveTab()
*/

/*!
    \fn void QTabBar::tabBarClicked(int index)

    This signal is emitted when user clicks on a tab at an \a index.

    \a index is the index of a clicked tab, or -1 if no tab is under the cursor.

    \since 5.2
*/

/*!
    \fn void QTabBar::tabBarDoubleClicked(int index)

    This signal is emitted when the user double clicks on a tab at \a index.

    \a index refers to the tab clicked, or -1 if no tab is under the cursor.

    \since 5.2
*/

void QTabBarPrivate::init()
{
    Q_Q(QTabBar);
    leftB = new QToolButton(q);
    leftB->setObjectName(u"ScrollLeftButton"_s);
    leftB->setAutoRepeat(true);
    QObject::connect(leftB, SIGNAL(clicked()), q, SLOT(_q_scrollTabs()));
    leftB->hide();
    rightB = new QToolButton(q);
    rightB->setObjectName(u"ScrollRightButton"_s);
    rightB->setAutoRepeat(true);
    QObject::connect(rightB, SIGNAL(clicked()), q, SLOT(_q_scrollTabs()));
    rightB->hide();
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled()) {
        leftB->setFocusPolicy(Qt::NoFocus);
        rightB->setFocusPolicy(Qt::NoFocus);
        q->setFocusPolicy(Qt::NoFocus);
    } else
#endif
        q->setFocusPolicy(Qt::TabFocus);

#if QT_CONFIG(accessibility)
    leftB->setAccessibleName(QTabBar::tr("Scroll Left"));
    rightB->setAccessibleName(QTabBar::tr("Scroll Right"));
#endif
    q->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    elideMode = Qt::TextElideMode(q->style()->styleHint(QStyle::SH_TabBar_ElideMode, nullptr, q));
    useScrollButtons = !q->style()->styleHint(QStyle::SH_TabBar_PreferNoArrows, nullptr, q);
}

int QTabBarPrivate::indexAtPos(const QPoint &p) const
{
    Q_Q(const QTabBar);
    if (q->tabRect(currentIndex).contains(p))
        return currentIndex;
    for (int i = 0; i < tabList.size(); ++i)
        if (tabList.at(i)->enabled && q->tabRect(i).contains(p))
            return i;
    return -1;
}

void QTabBarPrivate::layoutTabs()
{
    Q_Q(QTabBar);
    layoutDirty = false;
    QSize size = q->size();
    int last, available;
    int maxExtent;
    bool vertTabs = verticalTabs(shape);
    int tabChainIndex = 0;
    int hiddenTabs = 0;

    Qt::Alignment tabAlignment = Qt::Alignment(q->style()->styleHint(QStyle::SH_TabBar_Alignment, nullptr, q));
    QList<QLayoutStruct> tabChain(tabList.size() + 2);

    // We put an empty item at the front and back and set its expansive attribute
    // depending on tabAlignment and expanding.
    tabChain[tabChainIndex].init();
    tabChain[tabChainIndex].expansive = (!expanding)
                                        && (tabAlignment != Qt::AlignLeft)
                                        && (tabAlignment != Qt::AlignJustify);
    tabChain[tabChainIndex].empty = true;
    ++tabChainIndex;

    // We now go through our list of tabs and set the minimum size and the size hint
    // This will allow us to elide text if necessary. Since we don't set
    // a maximum size, tabs will EXPAND to fill up the empty space.
    // Since tab widget is rather *ahem* strict about keeping the geometry of the
    // tab bar to its absolute minimum, this won't bleed through, but will show up
    // if you use tab bar on its own (a.k.a. not a bug, but a feature).
    // Update: if expanding is false, we DO set a maximum size to prevent the tabs
    // being wider than necessary.
    if (!vertTabs) {
        int minx = 0;
        int x = 0;
        int maxHeight = 0;
        for (int i = 0; i < tabList.size(); ++i) {
            const auto tab = tabList.at(i);
            if (!tab->visible) {
                ++hiddenTabs;
                continue;
            }
            QSize sz = q->tabSizeHint(i);
            tab->maxRect = QRect(x, 0, sz.width(), sz.height());
            x += sz.width();
            maxHeight = qMax(maxHeight, sz.height());
            sz = q->minimumTabSizeHint(i);
            tab->minRect = QRect(minx, 0, sz.width(), sz.height());
            minx += sz.width();
            tabChain[tabChainIndex].init();
            tabChain[tabChainIndex].sizeHint = tab->maxRect.width();
            tabChain[tabChainIndex].minimumSize = sz.width();
            tabChain[tabChainIndex].empty = false;
            tabChain[tabChainIndex].expansive = true;

            if (!expanding)
                tabChain[tabChainIndex].maximumSize = tabChain[tabChainIndex].sizeHint;
            ++tabChainIndex;
        }

        last = minx;
        available = size.width();
        maxExtent = maxHeight;
    } else {
        int miny = 0;
        int y = 0;
        int maxWidth = 0;
        for (int i = 0; i < tabList.size(); ++i) {
            auto tab = tabList.at(i);
            if (!tab->visible) {
                ++hiddenTabs;
                continue;
            }
            QSize sz = q->tabSizeHint(i);
            tab->maxRect = QRect(0, y, sz.width(), sz.height());
            y += sz.height();
            maxWidth = qMax(maxWidth, sz.width());
            sz = q->minimumTabSizeHint(i);
            tab->minRect = QRect(0, miny, sz.width(), sz.height());
            miny += sz.height();
            tabChain[tabChainIndex].init();
            tabChain[tabChainIndex].sizeHint = tab->maxRect.height();
            tabChain[tabChainIndex].minimumSize = sz.height();
            tabChain[tabChainIndex].empty = false;
            tabChain[tabChainIndex].expansive = true;

            if (!expanding)
                tabChain[tabChainIndex].maximumSize = tabChain[tabChainIndex].sizeHint;
            ++tabChainIndex;
        }

        last = miny;
        available = size.height();
        maxExtent = maxWidth;
    }

    // Mirror our front item.
    tabChain[tabChainIndex].init();
    tabChain[tabChainIndex].expansive = (!expanding)
                                        && (tabAlignment != Qt::AlignRight)
                                        && (tabAlignment != Qt::AlignJustify);
    tabChain[tabChainIndex].empty = true;
    Q_ASSERT(tabChainIndex == tabChain.size() - 1 - hiddenTabs); // add an assert just to make sure.

    // Do the calculation
    qGeomCalc(tabChain, 0, tabChain.size(), 0, qMax(available, last), 0);

    // Use the results
    hiddenTabs = 0;
    for (int i = 0; i < tabList.size(); ++i) {
        auto tab = tabList.at(i);
        if (!tab->visible) {
            tab->rect = QRect();
            ++hiddenTabs;
            continue;
        }
        const QLayoutStruct &lstruct = tabChain.at(i + 1 - hiddenTabs);
        if (!vertTabs)
            tab->rect.setRect(lstruct.pos, 0, lstruct.size, maxExtent);
        else
            tab->rect.setRect(0, lstruct.pos, maxExtent, lstruct.size);
    }

    if (useScrollButtons && tabList.size() && last > available) {
        const QRect scrollRect = normalizedScrollRect(0);

        Q_Q(QTabBar);
        QStyleOption opt;
        opt.initFrom(q);
        QRect scrollButtonLeftRect = q->style()->subElementRect(QStyle::SE_TabBarScrollLeftButton, &opt, q);
        QRect scrollButtonRightRect = q->style()->subElementRect(QStyle::SE_TabBarScrollRightButton, &opt, q);
        int scrollButtonWidth = q->style()->pixelMetric(QStyle::PM_TabBarScrollButtonWidth, &opt, q);

        // Normally SE_TabBarScrollLeftButton should have the same width as PM_TabBarScrollButtonWidth.
        // But if that is not the case, we set the actual button width to PM_TabBarScrollButtonWidth, and
        // use the extra space from SE_TabBarScrollLeftButton as margins towards the tabs.
        if (vertTabs) {
            scrollButtonLeftRect.setHeight(scrollButtonWidth);
            scrollButtonRightRect.setY(scrollButtonRightRect.bottom() + 1 - scrollButtonWidth);
            scrollButtonRightRect.setHeight(scrollButtonWidth);
            leftB->setArrowType(Qt::UpArrow);
            rightB->setArrowType(Qt::DownArrow);
        } else if (q->layoutDirection() == Qt::RightToLeft) {
            scrollButtonRightRect.setWidth(scrollButtonWidth);
            scrollButtonLeftRect.setX(scrollButtonLeftRect.right() + 1 - scrollButtonWidth);
            scrollButtonLeftRect.setWidth(scrollButtonWidth);
            leftB->setArrowType(Qt::RightArrow);
            rightB->setArrowType(Qt::LeftArrow);
        } else {
            scrollButtonLeftRect.setWidth(scrollButtonWidth);
            scrollButtonRightRect.setX(scrollButtonRightRect.right() + 1 - scrollButtonWidth);
            scrollButtonRightRect.setWidth(scrollButtonWidth);
            leftB->setArrowType(Qt::LeftArrow);
            rightB->setArrowType(Qt::RightArrow);
        }

        leftB->setGeometry(scrollButtonLeftRect);
        leftB->setEnabled(false);
        leftB->show();

        rightB->setGeometry(scrollButtonRightRect);
        rightB->setEnabled(last + scrollRect.left() > scrollRect.x() + scrollRect.width());
        rightB->show();
    } else {
        rightB->hide();
        leftB->hide();
    }

    layoutWidgets();
    q->tabLayoutChange();
}

QRect QTabBarPrivate::normalizedScrollRect(int index)
{
    // "Normalized scroll rect" means return the free space on the tab bar
    // that doesn't overlap with scroll buttons or tear indicators, and
    // always return the rect as horizontal Qt::LeftToRight, even if the
    // tab bar itself is in a different orientation.

    Q_Q(QTabBar);
    // If scrollbuttons are not visible, then there's no tear either, and
    // the entire widget is the scroll rect.
    if (leftB->isHidden())
        return verticalTabs(shape) ? q->rect().transposed() : q->rect();

    QStyleOptionTab opt;
    q->initStyleOption(&opt, currentIndex);
    opt.rect = q->rect();

    QRect scrollButtonLeftRect = q->style()->subElementRect(QStyle::SE_TabBarScrollLeftButton, &opt, q);
    QRect scrollButtonRightRect = q->style()->subElementRect(QStyle::SE_TabBarScrollRightButton, &opt, q);
    QRect tearLeftRect = q->style()->subElementRect(QStyle::SE_TabBarTearIndicatorLeft, &opt, q);
    QRect tearRightRect = q->style()->subElementRect(QStyle::SE_TabBarTearIndicatorRight, &opt, q);

    if (verticalTabs(shape)) {
        int topEdge, bottomEdge;
        bool leftButtonIsOnTop = scrollButtonLeftRect.y() < q->height() / 2;
        bool rightButtonIsOnTop = scrollButtonRightRect.y() < q->height() / 2;

        if (leftButtonIsOnTop && rightButtonIsOnTop) {
            topEdge = scrollButtonRightRect.bottom() + 1;
            bottomEdge = q->height();
        } else if (!leftButtonIsOnTop && !rightButtonIsOnTop) {
            topEdge = 0;
            bottomEdge = scrollButtonLeftRect.top();
        } else {
            topEdge = scrollButtonLeftRect.bottom() + 1;
            bottomEdge = scrollButtonRightRect.top();
        }

        bool tearTopVisible = index != 0 && topEdge != -scrollOffset;
        bool tearBottomVisible = index != tabList.size() - 1 && bottomEdge != tabList.constLast()->rect.bottom() + 1 - scrollOffset;
        if (tearTopVisible && !tearLeftRect.isNull())
            topEdge = tearLeftRect.bottom() + 1;
        if (tearBottomVisible && !tearRightRect.isNull())
            bottomEdge = tearRightRect.top();

        return QRect(topEdge, 0, bottomEdge - topEdge, q->height());
    } else {
        if (q->layoutDirection() == Qt::RightToLeft) {
            scrollButtonLeftRect = QStyle::visualRect(Qt::RightToLeft, q->rect(), scrollButtonLeftRect);
            scrollButtonRightRect = QStyle::visualRect(Qt::RightToLeft, q->rect(), scrollButtonRightRect);
            tearLeftRect = QStyle::visualRect(Qt::RightToLeft, q->rect(), tearLeftRect);
            tearRightRect = QStyle::visualRect(Qt::RightToLeft, q->rect(), tearRightRect);
        }

        int leftEdge, rightEdge;
        bool leftButtonIsOnLeftSide = scrollButtonLeftRect.x() < q->width() / 2;
        bool rightButtonIsOnLeftSide = scrollButtonRightRect.x() < q->width() / 2;

        if (leftButtonIsOnLeftSide && rightButtonIsOnLeftSide) {
            leftEdge = scrollButtonRightRect.right() + 1;
            rightEdge = q->width();
        } else if (!leftButtonIsOnLeftSide && !rightButtonIsOnLeftSide) {
            leftEdge = 0;
            rightEdge = scrollButtonLeftRect.left();
        } else {
            leftEdge = scrollButtonLeftRect.right() + 1;
            rightEdge = scrollButtonRightRect.left();
        }

        bool tearLeftVisible = index != 0 && leftEdge != -scrollOffset;
        bool tearRightVisible = index != tabList.size() - 1 && rightEdge != tabList.constLast()->rect.right() + 1 - scrollOffset;
        if (tearLeftVisible && !tearLeftRect.isNull())
            leftEdge = tearLeftRect.right() + 1;
        if (tearRightVisible && !tearRightRect.isNull())
            rightEdge = tearRightRect.left();

        return QRect(leftEdge, 0, rightEdge - leftEdge, q->height());
    }
}

int QTabBarPrivate::hoveredTabIndex() const
{
    if (dragInProgress)
        return currentIndex;
    if (hoverIndex >= 0)
        return hoverIndex;
    return -1;
}

void QTabBarPrivate::makeVisible(int index)
{
    Q_Q(QTabBar);
    if (!validIndex(index))
        return;

    const QRect tabRect = tabList.at(index)->rect;
    const int oldScrollOffset = scrollOffset;
    const bool horiz = !verticalTabs(shape);
    const int available = horiz ? q->width() : q->height();
    const int tabStart = horiz ? tabRect.left() : tabRect.top();
    const int tabEnd = horiz ? tabRect.right() : tabRect.bottom();
    const int lastTabEnd = horiz ? tabList.constLast()->rect.right() : tabList.constLast()->rect.bottom();
    const QRect scrollRect = normalizedScrollRect(index);
    const QRect entireScrollRect = normalizedScrollRect(0); // ignore tears
    const int scrolledTabBarStart = qMax(1, scrollRect.left() + scrollOffset);
    const int scrolledTabBarEnd = qMin(lastTabEnd - 1, scrollRect.right() + scrollOffset);

    if (tabStart < scrolledTabBarStart) {
        // Tab is outside on the left, so scroll left.
        scrollOffset = tabStart - scrollRect.left();
    } else if (tabEnd > scrolledTabBarEnd) {
        // Tab is outside on the right, so scroll right.
        scrollOffset = qMax(0, tabEnd - scrollRect.right());
    } else if (scrollOffset + entireScrollRect.width() > lastTabEnd + 1) {
        // fill any free space on the right without overshooting
        scrollOffset = qMax(0, lastTabEnd - entireScrollRect.width() + 1);
    } else if (available >= lastTabEnd) {
        // the entire tabbar fits, reset scroll
        scrollOffset = 0;
    }

    leftB->setEnabled(scrollOffset > -scrollRect.left());
    rightB->setEnabled(scrollOffset < lastTabEnd - scrollRect.right());

    if (oldScrollOffset != scrollOffset) {
        q->update();
        layoutWidgets();
    }
}

void QTabBarPrivate::killSwitchTabTimer()
{
    Q_Q(QTabBar);
    if (switchTabTimerId) {
        q->killTimer(switchTabTimerId);
        switchTabTimerId = 0;
    }
    switchTabCurrentIndex = -1;
}

void QTabBarPrivate::layoutTab(int index)
{
    Q_Q(QTabBar);
    Q_ASSERT(index >= 0);

    const Tab *tab = tabList.at(index);
    bool vertical = verticalTabs(shape);
    if (!(tab->leftWidget || tab->rightWidget))
        return;

    QStyleOptionTab opt;
    q->initStyleOption(&opt, index);
    if (tab->leftWidget) {
        QRect rect = q->style()->subElementRect(QStyle::SE_TabBarTabLeftButton, &opt, q);
        QPoint p = rect.topLeft();
        if ((index == pressedIndex) || paintWithOffsets) {
            if (vertical)
                p.setY(p.y() + tab->dragOffset);
            else
                p.setX(p.x() + tab->dragOffset);
        }
        tab->leftWidget->move(p);
    }
    if (tab->rightWidget) {
        QRect rect = q->style()->subElementRect(QStyle::SE_TabBarTabRightButton, &opt, q);
        QPoint p = rect.topLeft();
        if ((index == pressedIndex) || paintWithOffsets) {
            if (vertical)
                p.setY(p.y() + tab->dragOffset);
            else
                p.setX(p.x() + tab->dragOffset);
        }
        tab->rightWidget->move(p);
    }
}

void QTabBarPrivate::layoutWidgets(int start)
{
    Q_Q(QTabBar);
    for (int i = start; i < q->count(); ++i) {
        layoutTab(i);
    }
}

void QTabBarPrivate::autoHideTabs()
{
    Q_Q(QTabBar);

    if (autoHide)
        q->setVisible(q->count() > 1);
}

void QTabBarPrivate::_q_closeTab()
{
    Q_Q(QTabBar);
    QObject *object = q->sender();
    int tabToClose = -1;
    QTabBar::ButtonPosition closeSide = (QTabBar::ButtonPosition)q->style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, q);
    for (int i = 0; i < tabList.size(); ++i) {
        if (closeSide == QTabBar::LeftSide) {
            if (tabList.at(i)->leftWidget == object) {
                tabToClose = i;
                break;
            }
        } else {
            if (tabList.at(i)->rightWidget == object) {
                tabToClose = i;
                break;
            }
        }
    }
    if (tabToClose != -1)
        emit q->tabCloseRequested(tabToClose);
}

void QTabBarPrivate::_q_scrollTabs()
{
    Q_Q(QTabBar);
    const QObject *sender = q->sender();
    const bool horizontal = !verticalTabs(shape);
    const QRect scrollRect = normalizedScrollRect().translated(scrollOffset, 0);

    int i = -1;

    if (sender == leftB) {
        for (i = tabList.size() - 1; i >= 0; --i) {
            int start = horizontal ? tabList.at(i)->rect.left() : tabList.at(i)->rect.top();
            if (start < scrollRect.left()) {
                makeVisible(i);
                return;
            }
        }
    } else if (sender == rightB) {
        for (i = 0; i < tabList.size(); ++i) {
            const auto tabRect = tabList.at(i)->rect;
            int start = horizontal ? tabRect.left() : tabRect.top();
            int end = horizontal ? tabRect.right() : tabRect.bottom();
            if (end > scrollRect.right() && start > scrollOffset) {
                makeVisible(i);
                return;
            }
        }
    }
}

void QTabBarPrivate::refresh()
{
    Q_Q(QTabBar);

    // be safe in case a subclass is also handling move with the tabs
    if (pressedIndex != -1
        && movable
        && mouseButtons == Qt::NoButton) {
        moveTabFinished(pressedIndex);
        if (!validIndex(pressedIndex))
            pressedIndex = -1;
    }

    if (!q->isVisible()) {
        layoutDirty = true;
    } else {
        layoutTabs();
        makeVisible(currentIndex);
        q->update();
        q->updateGeometry();
    }
}

/*!
    Creates a new tab bar with the given \a parent.
*/
QTabBar::QTabBar(QWidget* parent)
    :QWidget(*new QTabBarPrivate, parent, { })
{
    Q_D(QTabBar);
    d->init();
}


/*!
    Destroys the tab bar.
*/
QTabBar::~QTabBar()
{
}

/*!
    \property QTabBar::shape
    \brief the shape of the tabs in the tab bar

    Possible values for this property are described by the Shape enum.
*/


QTabBar::Shape QTabBar::shape() const
{
    Q_D(const QTabBar);
    return d->shape;
}

void QTabBar::setShape(Shape shape)
{
    Q_D(QTabBar);
    if (d->shape == shape)
        return;
    d->shape = shape;
    d->refresh();
}

/*!
    \property QTabBar::drawBase
    \brief defines whether or not tab bar should draw its base.

    If true then QTabBar draws a base in relation to the styles overlap.
    Otherwise only the tabs are drawn.

    \sa QStyle::pixelMetric(), QStyle::PM_TabBarBaseOverlap, QStyleOptionTabBarBase
*/

void QTabBar::setDrawBase(bool drawBase)
{
    Q_D(QTabBar);
    if (d->drawBase == drawBase)
        return;
    d->drawBase = drawBase;
    update();
}

bool QTabBar::drawBase() const
{
    Q_D(const QTabBar);
    return d->drawBase;
}

/*!
    Adds a new tab with text \a text. Returns the new
    tab's index.
*/
int QTabBar::addTab(const QString &text)
{
    return insertTab(-1, text);
}

/*!
    \overload

    Adds a new tab with icon \a icon and text \a
    text. Returns the new tab's index.
*/
int QTabBar::addTab(const QIcon& icon, const QString &text)
{
    return insertTab(-1, icon, text);
}

/*!
    Inserts a new tab with text \a text at position \a index. If \a
    index is out of range, the new tab is appended. Returns the new
    tab's index.
*/
int QTabBar::insertTab(int index, const QString &text)
{
    return insertTab(index, QIcon(), text);
}

/*!\overload

    Inserts a new tab with icon \a icon and text \a text at position
    \a index. If \a index is out of range, the new tab is
    appended. Returns the new tab's index.

    If the QTabBar was empty before this function is called, the inserted tab
    becomes the current tab.

    Inserting a new tab at an index less than or equal to the current index
    will increment the current index, but keep the current tab.
*/
int QTabBar::insertTab(int index, const QIcon& icon, const QString &text)
{
    Q_D(QTabBar);
    if (!d->validIndex(index)) {
        index = d->tabList.size();
        d->tabList.append(new QTabBarPrivate::Tab(icon, text));
    } else {
        d->tabList.insert(index, new QTabBarPrivate::Tab(icon, text));
    }
#ifndef QT_NO_SHORTCUT
    d->tabList.at(index)->shortcutId = grabShortcut(QKeySequence::mnemonic(text));
#endif
    d->firstVisible = qMax(qMin(index, d->firstVisible), 0);
    d->refresh();
    if (d->tabList.size() == 1)
        setCurrentIndex(index);
    else if (index <= d->currentIndex)
        ++d->currentIndex;

    if (index <= d->lastVisible)
        ++d->lastVisible;
    else
        d->lastVisible = index;

    if (d->closeButtonOnTabs) {
        QStyleOptionTab opt;
        initStyleOption(&opt, index);
        ButtonPosition closeSide = (ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, this);
        QAbstractButton *closeButton = new CloseButton(this);
        connect(closeButton, SIGNAL(clicked()), this, SLOT(_q_closeTab()));
        setTabButton(index, closeSide, closeButton);
    }

    for (const auto tab : std::as_const(d->tabList)) {
        if (tab->lastTab >= index)
            ++tab->lastTab;
    }

    if (tabAt(d->mousePosition) == index) {
        d->hoverIndex = index;
        d->hoverRect = tabRect(index);
    }

    tabInserted(index);
    d->autoHideTabs();
    return index;
}


/*!
    Removes the tab at position \a index.

    \sa SelectionBehavior
 */
void QTabBar::removeTab(int index)
{
    Q_D(QTabBar);
    if (d->validIndex(index)) {
        auto removedTab = d->tabList.at(index);
        if (d->dragInProgress)
            d->moveTabFinished(d->pressedIndex);

#ifndef QT_NO_SHORTCUT
        releaseShortcut(d->tabList.at(index)->shortcutId);
#endif
        if (removedTab->leftWidget) {
            removedTab->leftWidget->hide();
            removedTab->leftWidget->deleteLater();
            removedTab->leftWidget = nullptr;
        }
        if (removedTab->rightWidget) {
            removedTab->rightWidget->hide();
            removedTab->rightWidget->deleteLater();
            removedTab->rightWidget = nullptr;
        }

        int newIndex = removedTab->lastTab;
        d->tabList.removeAt(index);
        delete removedTab;
        for (auto tab : std::as_const(d->tabList)) {
            if (tab->lastTab == index)
                tab->lastTab = -1;
            if (tab->lastTab > index)
                --tab->lastTab;
        }

        d->calculateFirstLastVisible(index, false, true);

        if (index == d->currentIndex) {
            // The current tab is going away, in order to make sure
            // we emit that "current has changed", we need to reset this
            // around.
            d->currentIndex = -1;
            if (d->tabList.size() > 0) {
                switch(d->selectionBehaviorOnRemove) {
                case SelectPreviousTab:
                    if (newIndex > index)
                        newIndex--;
                    if (d->validIndex(newIndex) && d->tabList.at(newIndex)->visible)
                        break;
                    Q_FALLTHROUGH();
                case SelectRightTab:
                    newIndex = qBound(d->firstVisible, index, d->lastVisible);
                    break;
                case SelectLeftTab:
                    newIndex = qBound(d->firstVisible, index-1, d->lastVisible);
                    break;
                default:
                    break;
                }

                if (d->validIndex(newIndex)) {
                    // don't loose newIndex's old through setCurrentIndex
                    int bump = d->tabList.at(newIndex)->lastTab;
                    setCurrentIndex(newIndex);
                    d->tabList.at(newIndex)->lastTab = bump;
                } else {
                    // we had a valid current index, but there are no visible tabs left
                    emit currentChanged(-1);
                }
            } else {
                emit currentChanged(-1);
            }
        } else if (index < d->currentIndex) {
            setCurrentIndex(d->currentIndex - 1);
        }
        d->refresh();
        d->autoHideTabs();
        if (d->hoverRect.isValid()) {
            update(d->hoverRect);
            d->hoverIndex = tabAt(d->mousePosition);
            if (d->validIndex(d->hoverIndex)) {
                d->hoverRect = tabRect(d->hoverIndex);
                update(d->hoverRect);
            } else {
                d->hoverRect = QRect();
            }
        }
        tabRemoved(index);
    }
}


/*!
    Returns \c true if the tab at position \a index is enabled; otherwise
    returns \c false.
*/
bool QTabBar::isTabEnabled(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->enabled;
    return false;
}

/*!
    If \a enabled is true then the tab at position \a index is
    enabled; otherwise the item at position \a index is disabled.
*/
void QTabBar::setTabEnabled(int index, bool enabled)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        tab->enabled = enabled;
#ifndef QT_NO_SHORTCUT
        setShortcutEnabled(tab->shortcutId, enabled);
#endif
        update();
        if (!enabled && index == d->currentIndex)
            setCurrentIndex(d->selectNewCurrentIndexFrom(index+1));
        else if (enabled && !isTabVisible(d->currentIndex))
            setCurrentIndex(d->selectNewCurrentIndexFrom(index));
    }
}


/*!
    Returns true if the tab at position \a index is visible; otherwise
    returns false.
    \since 5.15
*/
bool QTabBar::isTabVisible(int index) const
{
    Q_D(const QTabBar);
    if (d->validIndex(index))
        return d->tabList.at(index)->visible;
    return false;
}

/*!
    If \a visible is true, make the tab at position \a index visible,
    otherwise make it hidden.
    \since 5.15
*/
void QTabBar::setTabVisible(int index, bool visible)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        d->layoutDirty = (visible != tab->visible);
        if (!d->layoutDirty)
            return;
        tab->visible = visible;
        if (tab->leftWidget)
            tab->leftWidget->setVisible(visible);
        if (tab->rightWidget)
            tab->rightWidget->setVisible(visible);
#ifndef QT_NO_SHORTCUT
        setShortcutEnabled(tab->shortcutId, visible);
#endif
        d->calculateFirstLastVisible(index, visible, false);
        if (!visible && index == d->currentIndex) {
            const int newindex = d->selectNewCurrentIndexFrom(index+1);
            setCurrentIndex(newindex);
        }
        update();
    }
}


/*!
    Returns the text of the tab at position \a index, or an empty
    string if \a index is out of range.
*/
QString QTabBar::tabText(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->text;
    return QString();
}

/*!
    Sets the text of the tab at position \a index to \a text.
*/
void QTabBar::setTabText(int index, const QString &text)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        d->textSizes.remove(tab->text);
        tab->text = text;
#ifndef QT_NO_SHORTCUT
        releaseShortcut(tab->shortcutId);
        tab->shortcutId = grabShortcut(QKeySequence::mnemonic(text));
        setShortcutEnabled(tab->shortcutId, tab->enabled);
#endif
        d->refresh();
    }
}

/*!
    Returns the text color of the tab with the given \a index, or a invalid
    color if \a index is out of range.

    \sa setTabTextColor()
*/
QColor QTabBar::tabTextColor(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->textColor;
    return QColor();
}

/*!
    Sets the color of the text in the tab with the given \a index to the specified \a color.

    If an invalid color is specified, the tab will use the QTabBar foreground role instead.

    \sa tabTextColor()
*/
void QTabBar::setTabTextColor(int index, const QColor &color)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        tab->textColor = color;
        update(tabRect(index));
    }
}

/*!
    Returns the icon of the tab at position \a index, or a null icon
    if \a index is out of range.
*/
QIcon QTabBar::tabIcon(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->icon;
    return QIcon();
}

/*!
    Sets the icon of the tab at position \a index to \a icon.
*/
void QTabBar::setTabIcon(int index, const QIcon & icon)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        bool simpleIconChange = (!icon.isNull() && !tab->icon.isNull());
        tab->icon = icon;
        if (simpleIconChange)
            update(tabRect(index));
        else
            d->refresh();
    }
}

#if QT_CONFIG(tooltip)
/*!
    Sets the tool tip of the tab at position \a index to \a tip.
*/
void QTabBar::setTabToolTip(int index, const QString & tip)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index))
        tab->toolTip = tip;
}

/*!
    Returns the tool tip of the tab at position \a index, or an empty
    string if \a index is out of range.
*/
QString QTabBar::tabToolTip(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->toolTip;
    return QString();
}
#endif // QT_CONFIG(tooltip)

#if QT_CONFIG(whatsthis)
/*!
    \since 4.1

    Sets the What's This help text of the tab at position \a index
    to \a text.
*/
void QTabBar::setTabWhatsThis(int index, const QString &text)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index))
        tab->whatsThis = text;
}

/*!
    \since 4.1

    Returns the What's This help text of the tab at position \a index,
    or an empty string if \a index is out of range.
*/
QString QTabBar::tabWhatsThis(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->whatsThis;
    return QString();
}

#endif // QT_CONFIG(whatsthis)

/*!
    Sets the data of the tab at position \a index to \a data.
*/
void QTabBar::setTabData(int index, const QVariant & data)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index))
        tab->data = data;
}

/*!
    Returns the data of the tab at position \a index, or a null
    variant if \a index is out of range.
*/
QVariant QTabBar::tabData(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->data;
    return QVariant();
}

/*!
    Returns the visual rectangle of the tab at position \a
    index, or a null rectangle if \a index is hidden, or out of range.
*/
QRect QTabBar::tabRect(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index)) {
        if (d->layoutDirty)
            const_cast<QTabBarPrivate*>(d)->layoutTabs();
        if (!tab->visible)
            return QRect();
        QRect r = tab->rect;
        if (verticalTabs(d->shape))
            r.translate(0, -d->scrollOffset);
        else
            r.translate(-d->scrollOffset, 0);
        if (!verticalTabs(d->shape))
            r = QStyle::visualRect(layoutDirection(), rect(), r);
        return r;
    }
    return QRect();
}

/*!
    \since 4.3
    Returns the index of the tab that covers \a position or -1 if no
    tab covers \a position;
*/

int QTabBar::tabAt(const QPoint &position) const
{
    Q_D(const QTabBar);
    if (d->validIndex(d->currentIndex)
        && tabRect(d->currentIndex).contains(position)) {
        return d->currentIndex;
    }
    const int max = d->tabList.size();
    for (int i = 0; i < max; ++i) {
        if (tabRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

/*!
    \property QTabBar::currentIndex
    \brief the index of the tab bar's visible tab

    The current index is -1 if there is no current tab.
*/

int QTabBar::currentIndex() const
{
    Q_D(const QTabBar);
    if (d->validIndex(d->currentIndex))
        return d->currentIndex;
    return -1;
}


void QTabBar::setCurrentIndex(int index)
{
    Q_D(QTabBar);
    if (d->dragInProgress && d->pressedIndex != -1)
        return;
    if (d->currentIndex == index)
        return;

    int oldIndex = d->currentIndex;
    if (auto tab = d->at(index)) {
        d->currentIndex = index;
        // If the size hint depends on whether the tab is selected (for instance a style
        // sheet rule that sets a bold font on the 'selected' tab) then we need to
        // re-layout the entire tab bar. To minimize the cost, do that only if the
        // size hint changes for the tab that becomes the current tab (the old current tab
        // will most certainly do the same). QTBUG-6905
        if (tabRect(index).size() != tabSizeHint(index))
            d->layoutTabs();
        update();
        if (!isVisible())
            d->layoutDirty = true;
        else
            d->makeVisible(index);
        if (d->validIndex(oldIndex)) {
            tab->lastTab = oldIndex;
            d->layoutTab(oldIndex);
        }
        d->layoutTab(index);
#if QT_CONFIG(accessibility)
        if (QAccessible::isActive()) {
            if (hasFocus()) {
                QAccessibleEvent focusEvent(this, QAccessible::Focus);
                focusEvent.setChild(index);
                QAccessible::updateAccessibility(&focusEvent);
            }
            QAccessibleEvent selectionEvent(this, QAccessible::Selection);
            selectionEvent.setChild(index);
            QAccessible::updateAccessibility(&selectionEvent);
        }
#endif
        emit currentChanged(index);
    }
}

/*!
    \property QTabBar::iconSize
    \brief The size for icons in the tab bar
    \since 4.1

    The default value is style-dependent. \c iconSize is a maximum
    size; icons that are smaller are not scaled up.

    \sa QTabWidget::iconSize
*/
QSize QTabBar::iconSize() const
{
    Q_D(const QTabBar);
    if (d->iconSize.isValid())
        return d->iconSize;
    int iconExtent = style()->pixelMetric(QStyle::PM_TabBarIconSize, nullptr, this);
    return QSize(iconExtent, iconExtent);

}

void QTabBar::setIconSize(const QSize &size)
{
    Q_D(QTabBar);
    d->iconSize = size;
    d->layoutDirty = true;
    update();
    updateGeometry();
}

/*!
    \property QTabBar::count
    \brief the number of tabs in the tab bar
*/

int QTabBar::count() const
{
    Q_D(const QTabBar);
    return d->tabList.size();
}


/*!\reimp
 */
QSize QTabBar::sizeHint() const
{
    Q_D(const QTabBar);
    if (d->layoutDirty)
        const_cast<QTabBarPrivate*>(d)->layoutTabs();
    QRect r;
    for (const auto tab : d->tabList) {
        if (tab->visible)
            r = r.united(tab->maxRect);
    }
    return r.size();
}

/*!\reimp
 */
QSize QTabBar::minimumSizeHint() const
{
    Q_D(const QTabBar);
    if (d->layoutDirty)
        const_cast<QTabBarPrivate*>(d)->layoutTabs();
    if (!d->useScrollButtons) {
        QRect r;
        for (const auto tab : d->tabList) {
            if (tab->visible)
                r = r.united(tab->minRect);
        }
        return r.size();
    }
    if (verticalTabs(d->shape))
        return QSize(sizeHint().width(), d->rightB->sizeHint().height() * 2 + 75);
    else
        return QSize(d->rightB->sizeHint().width() * 2 + 75, sizeHint().height());
}

// Compute the most-elided possible text, for minimumSizeHint
static QString computeElidedText(Qt::TextElideMode mode, const QString &text)
{
    if (text.size() <= 3)
        return text;

    static const auto Ellipses = "..."_L1;
    QString ret;
    switch (mode) {
    case Qt::ElideRight:
        ret = QStringView{text}.left(2) + Ellipses;
        break;
    case Qt::ElideMiddle:
        ret = QStringView{text}.left(1) + Ellipses + QStringView{text}.right(1);
        break;
    case Qt::ElideLeft:
        ret = Ellipses + QStringView{text}.right(2);
        break;
    case Qt::ElideNone:
        ret = text;
        break;
    }
    return ret;
}

/*!
    Returns the minimum tab size hint for the tab at position \a index.
    \since 5.0
*/

QSize QTabBar::minimumTabSizeHint(int index) const
{
    Q_D(const QTabBar);
    QTabBarPrivate::Tab *tab = d->tabList.at(index);
    QString oldText = tab->text;
    tab->text = computeElidedText(d->elideMode, oldText);
    QSize size = tabSizeHint(index);
    tab->text = oldText;
    return size;
}

/*!
    Returns the size hint for the tab at position \a index.
*/
QSize QTabBar::tabSizeHint(int index) const
{
    //Note: this must match with the computations in QCommonStylePrivate::tabLayout
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index)) {
        QStyleOptionTab opt;
        d->initBasicStyleOption(&opt, index);
        opt.text = tab->text;
        QSize iconSize = tab->icon.isNull() ? QSize(0, 0) : opt.iconSize;
        int hframe = style()->pixelMetric(QStyle::PM_TabBarTabHSpace, &opt, this);
        int vframe = style()->pixelMetric(QStyle::PM_TabBarTabVSpace, &opt, this);
        const QFontMetrics fm = fontMetrics();

        int maxWidgetHeight = qMax(opt.leftButtonSize.height(), opt.rightButtonSize.height());
        int maxWidgetWidth = qMax(opt.leftButtonSize.width(), opt.rightButtonSize.width());

        int widgetWidth = 0;
        int widgetHeight = 0;
        int padding = 0;
        if (!opt.leftButtonSize.isEmpty()) {
            padding += 4;
            widgetWidth += opt.leftButtonSize.width();
            widgetHeight += opt.leftButtonSize.height();
        }
        if (!opt.rightButtonSize.isEmpty()) {
            padding += 4;
            widgetWidth += opt.rightButtonSize.width();
            widgetHeight += opt.rightButtonSize.height();
        }
        if (!opt.icon.isNull())
            padding += 4;

        QHash<QString, QSize>::iterator it = d->textSizes.find(tab->text);
        if (it == d->textSizes.end())
           it = d->textSizes.insert(tab->text, fm.size(Qt::TextShowMnemonic, tab->text));
        const int textWidth = it.value().width();
        QSize csz;
        if (verticalTabs(d->shape)) {
            csz = QSize( qMax(maxWidgetWidth, qMax(fm.height(), iconSize.height())) + vframe,
                         textWidth + iconSize.width() + hframe + widgetHeight + padding);
        } else {
            csz = QSize(textWidth + iconSize.width() + hframe + widgetWidth + padding,
                  qMax(maxWidgetHeight, qMax(fm.height(), iconSize.height())) + vframe);
        }

        QSize retSize = style()->sizeFromContents(QStyle::CT_TabBarTab, &opt, csz, this);
        return retSize;
    }
    return QSize();
}

/*!
  This virtual handler is called after a new tab was added or
  inserted at position \a index.

  \sa tabRemoved()
 */
void QTabBar::tabInserted(int index)
{
    Q_UNUSED(index);
}

/*!
  This virtual handler is called after a tab was removed from
  position \a index.

  \sa tabInserted()
 */
void QTabBar::tabRemoved(int index)
{
    Q_UNUSED(index);
}

/*!
  This virtual handler is called whenever the tab layout changes.

  \sa tabRect()
 */
void QTabBar::tabLayoutChange()
{
}


/*!\reimp
 */
void QTabBar::showEvent(QShowEvent *)
{
    Q_D(QTabBar);
    if (d->layoutDirty)
        d->refresh();
    if (!d->validIndex(d->currentIndex))
        setCurrentIndex(0);
    else
        d->makeVisible(d->currentIndex);
    d->updateMacBorderMetrics();
}

/*!\reimp
 */
void QTabBar::hideEvent(QHideEvent *)
{
    Q_D(QTabBar);
    d->updateMacBorderMetrics();
}

/*!\reimp
 */
bool QTabBar::event(QEvent *event)
{
    Q_D(QTabBar);
    switch (event->type()) {
    case QEvent::HoverMove:
    case QEvent::HoverEnter: {
        QHoverEvent *he = static_cast<QHoverEvent *>(event);
        d->mousePosition = he->position().toPoint();
        if (!d->hoverRect.contains(d->mousePosition)) {
            if (d->hoverRect.isValid())
                update(d->hoverRect);
            d->hoverIndex = tabAt(d->mousePosition);
            if (d->validIndex(d->hoverIndex)) {
                d->hoverRect = tabRect(d->hoverIndex);
                update(d->hoverRect);
            } else {
                d->hoverRect = QRect();
            }
        }
        return true;
    }
    case QEvent::HoverLeave: {
        d->mousePosition = {-1, -1};
        if (d->hoverRect.isValid())
            update(d->hoverRect);
        d->hoverIndex = -1;
        d->hoverRect = QRect();
#if QT_CONFIG(wheelevent)
        d->accumulatedAngleDelta = QPoint();
#endif
        return true;
    }
#if QT_CONFIG(tooltip)
    case QEvent::ToolTip:
        if (const QTabBarPrivate::Tab *tab = d->at(tabAt(static_cast<QHelpEvent*>(event)->pos()))) {
            if (!tab->toolTip.isEmpty()) {
                QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), tab->toolTip, this);
                return true;
            }
        }
        break;
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
    case QEvent::QEvent::QueryWhatsThis: {
        const QTabBarPrivate::Tab *tab = d->at(d->indexAtPos(static_cast<QHelpEvent*>(event)->pos()));
        if (!tab || tab->whatsThis.isEmpty())
            event->ignore();
        return true;
    }
    case QEvent::WhatsThis:
        if (const QTabBarPrivate::Tab *tab = d->at(d->indexAtPos(static_cast<QHelpEvent*>(event)->pos()))) {
            if (!tab->whatsThis.isEmpty()) {
                QWhatsThis::showText(static_cast<QHelpEvent*>(event)->globalPos(),
                                     tab->whatsThis, this);
                return true;
            }
        }
        break;
#endif // QT_CONFIG(whatsthis)
#ifndef QT_NO_SHORTCUT

    case QEvent::Shortcut: {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(event);
        for (int i = 0; i < d->tabList.size(); ++i) {
            const QTabBarPrivate::Tab *tab = d->tabList.at(i);
            if (tab->shortcutId == se->shortcutId()) {
                setCurrentIndex(i);
                return true;
            }
        }
    }
        break;
#endif
    case QEvent::Move:
        d->updateMacBorderMetrics();
        break;
#if QT_CONFIG(draganddrop)

    case QEvent::DragEnter:
        if (d->changeCurrentOnDrag)
            event->accept();
        break;
    case QEvent::DragMove:
        if (d->changeCurrentOnDrag) {
            const int tabIndex = tabAt(static_cast<QDragMoveEvent *>(event)->position().toPoint());
            if (isTabEnabled(tabIndex) && d->switchTabCurrentIndex != tabIndex) {
                d->switchTabCurrentIndex = tabIndex;
                if (d->switchTabTimerId)
                    killTimer(d->switchTabTimerId);
                d->switchTabTimerId = startTimer(style()->styleHint(QStyle::SH_TabBar_ChangeCurrentDelay));
            }
            event->ignore();
        }
        break;
    case QEvent::DragLeave:
    case QEvent::Drop:
        d->killSwitchTabTimer();
        event->ignore();
        break;
#endif
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        d->mousePosition = static_cast<QMouseEvent *>(event)->position().toPoint();
        d->mouseButtons = static_cast<QMouseEvent *>(event)->buttons();
        break;
    default:
        break;
    }

    return QWidget::event(event);
}

/*!\reimp
 */
void QTabBar::resizeEvent(QResizeEvent *)
{
    Q_D(QTabBar);
    if (d->layoutDirty)
        updateGeometry();

    // when resizing, we want to keep the scroll offset as much as possible
    d->layoutTabs();

    d->makeVisible(d->currentIndex);
}

/*!\reimp
 */
void QTabBar::paintEvent(QPaintEvent *)
{
    Q_D(QTabBar);

    QStyleOptionTabBarBase optTabBase;
    QTabBarPrivate::initStyleBaseOption(&optTabBase, this, size());

    QStylePainter p(this);
    int selected = -1;
    int cutLeft = -1;
    int cutRight = -1;
    bool vertical = verticalTabs(d->shape);
    QStyleOptionTab cutTabLeft;
    QStyleOptionTab cutTabRight;
    selected = d->currentIndex;
    if (d->dragInProgress)
        selected = d->pressedIndex;
    const QRect scrollRect = d->normalizedScrollRect();

    for (int i = 0; i < d->tabList.size(); ++i)
         optTabBase.tabBarRect |= tabRect(i);

    optTabBase.selectedTabRect = tabRect(selected);

    if (d->drawBase)
        p.drawPrimitive(QStyle::PE_FrameTabBarBase, optTabBase);

    // the buttons might be semi-transparent or not fill their rect, but we don't
    // want the tab underneath to shine through, so clip the button area; QTBUG-50866
    if (d->leftB->isVisible() || d->rightB->isVisible()) {
        QStyleOption opt;
        opt.initFrom(this);
        QRegion buttonRegion;
        if (d->leftB->isVisible())
            buttonRegion |= style()->subElementRect(QStyle::SE_TabBarScrollLeftButton, &opt, this);
        if (d->rightB->isVisible())
            buttonRegion |= style()->subElementRect(QStyle::SE_TabBarScrollRightButton, &opt, this);
        if (!buttonRegion.isEmpty())
            p.setClipRegion(QRegion(rect()) - buttonRegion);
    }

    for (int i = 0; i < d->tabList.size(); ++i) {
        const auto tab = d->tabList.at(i);
        if (!tab->visible)
            continue;
        QStyleOptionTab tabOption;
        initStyleOption(&tabOption, i);
        if (d->paintWithOffsets && tab->dragOffset != 0) {
            if (vertical) {
                tabOption.rect.moveTop(tabOption.rect.y() + tab->dragOffset);
            } else {
                tabOption.rect.moveLeft(tabOption.rect.x() + tab->dragOffset);
            }
        }
        if (!(tabOption.state & QStyle::State_Enabled)) {
            tabOption.palette.setCurrentColorGroup(QPalette::Disabled);
        }

        // If this tab is partially obscured, make a note of it so that we can pass the information
        // along when we draw the tear.
        QRect tabRect = tab->rect;
        int tabStart = vertical ? tabRect.top() : tabRect.left();
        int tabEnd = vertical ? tabRect.bottom() : tabRect.right();
        if (tabStart < scrollRect.left() + d->scrollOffset) {
            cutLeft = i;
            cutTabLeft = tabOption;
        } else if (tabEnd > scrollRect.right() + d->scrollOffset) {
            cutRight = i;
            cutTabRight = tabOption;
        }

        // Don't bother drawing a tab if the entire tab is outside of the visible tab bar.
        if ((!vertical && (tabOption.rect.right() < 0 || tabOption.rect.left() > width()))
            || (vertical && (tabOption.rect.bottom() < 0 || tabOption.rect.top() > height())))
            continue;

        optTabBase.tabBarRect |= tabOption.rect;
        if (i == selected)
            continue;

        p.drawControl(QStyle::CE_TabBarTab, tabOption);
    }

    // Draw the selected tab last to get it "on top"
    if (selected >= 0) {
        QStyleOptionTab tabOption;
        const auto tab = d->tabList.at(selected);
        initStyleOption(&tabOption, selected);

        if (d->paintWithOffsets && tab->dragOffset != 0) {
            // if the drag offset is != 0, a move is in progress (drag or animation)
            // => set the tab position to Moving to preserve the rect
            tabOption.position = QStyleOptionTab::TabPosition::Moving;

            if (vertical)
                tabOption.rect.moveTop(tabOption.rect.y() + tab->dragOffset);
            else
                tabOption.rect.moveLeft(tabOption.rect.x() + tab->dragOffset);
        }

        // Calculate the rect of a moving tab
        const int taboverlap = style()->pixelMetric(QStyle::PM_TabBarTabOverlap, nullptr, this);
        const QRect &movingRect = verticalTabs(d->shape)
                ? tabOption.rect.adjusted(0, -taboverlap, 0, taboverlap)
                : tabOption.rect.adjusted(-taboverlap, 0, taboverlap, 0);

        // If a drag is in process, set the moving tab's geometry here
        // (in an animation, it is already set)
        if (d->dragInProgress)
            d->movingTab->setGeometry(movingRect);

        p.drawControl(QStyle::CE_TabBarTab, tabOption);
    }

    // Only draw the tear indicator if necessary. Most of the time we don't need too.
    if (d->leftB->isVisible() && cutLeft >= 0) {
        cutTabLeft.rect = rect();
        cutTabLeft.rect = style()->subElementRect(QStyle::SE_TabBarTearIndicatorLeft, &cutTabLeft, this);
        p.drawPrimitive(QStyle::PE_IndicatorTabTearLeft, cutTabLeft);
    }

    if (d->rightB->isVisible() && cutRight >= 0) {
        cutTabRight.rect = rect();
        cutTabRight.rect = style()->subElementRect(QStyle::SE_TabBarTearIndicatorRight, &cutTabRight, this);
        p.drawPrimitive(QStyle::PE_IndicatorTabTearRight, cutTabRight);
    }
}

/*
    When index changes visibility, we have to find first & last visible indexes.
    If remove is set, we force both
 */
void QTabBarPrivate::calculateFirstLastVisible(int index, bool visible, bool remove)
{
    if (visible) {
        firstVisible = qMin(index, firstVisible);
        lastVisible  = qMax(index, lastVisible);
    } else {
        if (remove || (index == firstVisible)) {
            firstVisible = -1;
            for (int i = 0; i < tabList.size(); ++i) {
                if (tabList.at(i)->visible) {
                    firstVisible = i;
                    break;
                }
            }
        }
        if (remove || (index == lastVisible)) {
            lastVisible = -1;
            for (int i = tabList.size() - 1; i >= 0; --i) {
                if (tabList.at(i)->visible) {
                    lastVisible = i;
                    break;
                }
            }
        }
    }
}

/*
    Selects the new current index starting at "fromIndex". If "fromIndex" is visible we're done.
    Else it tries any index AFTER fromIndex, then any BEFORE fromIndex and, if everything fails,
    it returns -1 indicating that no index is available
 */
int QTabBarPrivate::selectNewCurrentIndexFrom(int fromIndex)
{
    int newindex = -1;
    for (int i = fromIndex; i < tabList.size(); ++i) {
        if (at(i)->visible && at(i)->enabled) {
          newindex = i;
          break;
        }
    }
    if (newindex < 0) {
        for (int i = fromIndex-1; i > -1; --i) {
            if (at(i)->visible && at(i)->enabled) {
              newindex = i;
              break;
            }
        }
    }

    return newindex;
}

/*
    Given that index at position from moved to position to where return where index goes.
 */
int QTabBarPrivate::calculateNewPosition(int from, int to, int index) const
{
    if (index == from)
        return to;

    int start = qMin(from, to);
    int end = qMax(from, to);
    if (index >= start && index <= end)
        index += (from < to) ? -1 : 1;
    return index;
}

/*!
    Moves the item at index position \a from to index position \a to.
    \since 4.5

    \sa tabMoved(), tabLayoutChange()
 */
void QTabBar::moveTab(int from, int to)
{
    Q_D(QTabBar);
    if (from == to
        || !d->validIndex(from)
        || !d->validIndex(to))
        return;

    auto &fromTab = *d->tabList.at(from);
    auto &toTab = *d->tabList.at(to);

    bool vertical = verticalTabs(d->shape);
    int oldPressedPosition = 0;
    if (d->pressedIndex != -1) {
        // Record the position of the pressed tab before reordering the tabs.
        oldPressedPosition = vertical ? d->tabList.at(d->pressedIndex)->rect.y()
                                      : d->tabList.at(d->pressedIndex)->rect.x();
    }

    // Update the locations of the tabs first
    int start = qMin(from, to);
    int end = qMax(from, to);
    int width = vertical ? fromTab.rect.height() : fromTab.rect.width();
    if (from < to)
        width *= -1;
    bool rtl = isRightToLeft();
    for (int i = start; i <= end; ++i) {
        if (i == from)
            continue;
        auto &tab = *d->tabList.at(i);
        if (vertical)
            tab.rect.moveTop(tab.rect.y() + width);
        else
            tab.rect.moveLeft(tab.rect.x() + width);
        int direction = -1;
        if (rtl && !vertical)
            direction *= -1;
        if (tab.dragOffset != 0)
            tab.dragOffset += (direction * width);
    }

    if (vertical) {
        if (from < to)
            fromTab.rect.moveTop(toTab.rect.bottom() + 1);
        else
            fromTab.rect.moveTop(toTab.rect.top() - width);
    } else {
        if (from < to)
            fromTab.rect.moveLeft(toTab.rect.right() + 1);
        else
            fromTab.rect.moveLeft(toTab.rect.left() - width);
    }

    // Move the actual data structures
    d->tabList.move(from, to);

    // update lastTab locations
    for (const auto tab : std::as_const(d->tabList))
        tab->lastTab = d->calculateNewPosition(from, to, tab->lastTab);

    // update external variables
    int previousIndex = d->currentIndex;
    d->currentIndex = d->calculateNewPosition(from, to, d->currentIndex);

    // If we are in the middle of a drag update the dragStartPosition
    if (d->pressedIndex != -1) {
        d->pressedIndex = d->calculateNewPosition(from, to, d->pressedIndex);
        const auto pressedTab = d->tabList.at(d->pressedIndex);
        int newPressedPosition = vertical ? pressedTab->rect.top() : pressedTab->rect.left();
        int diff = oldPressedPosition - newPressedPosition;
        if (isRightToLeft() && !vertical)
            diff *= -1;
        if (vertical)
            d->dragStartPosition.setY(d->dragStartPosition.y() - diff);
        else
            d->dragStartPosition.setX(d->dragStartPosition.x() - diff);
    }

    d->layoutWidgets(start);
    update();
    emit tabMoved(from, to);
    if (previousIndex != d->currentIndex)
        emit currentChanged(d->currentIndex);
    emit tabLayoutChange();
}

void QTabBarPrivate::slide(int from, int to)
{
    Q_Q(QTabBar);
    if (from == to
            || !validIndex(from)
            || !validIndex(to))
        return;
    bool vertical = verticalTabs(shape);
    int preLocation = vertical ? q->tabRect(from).y() : q->tabRect(from).x();
    q->setUpdatesEnabled(false);
    q->moveTab(from, to);
    q->setUpdatesEnabled(true);
    int postLocation = vertical ? q->tabRect(to).y() : q->tabRect(to).x();
    int length = postLocation - preLocation;
    tabList.at(to)->dragOffset -= length;
    tabList.at(to)->startAnimation(this, ANIMATION_DURATION);
}

void QTabBarPrivate::moveTab(int index, int offset)
{
    if (!validIndex(index))
        return;
    tabList.at(index)->dragOffset = offset;
    layoutTab(index); // Make buttons follow tab
    q_func()->update();
}

/*!\reimp
*/
void QTabBar::mousePressEvent(QMouseEvent *event)
{
    Q_D(QTabBar);

    const QPoint pos = event->position().toPoint();
    const bool isEventInCornerButtons = (!d->leftB->isHidden() && d->leftB->geometry().contains(pos))
                                     || (!d->rightB->isHidden() && d->rightB->geometry().contains(pos));
    if (!isEventInCornerButtons) {
        const int index = d->indexAtPos(pos);
        emit tabBarClicked(index);
    }

    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    // Be safe!
    if (d->pressedIndex != -1 && d->movable)
        d->moveTabFinished(d->pressedIndex);

    d->pressedIndex = d->indexAtPos(event->position().toPoint());

    if (d->validIndex(d->pressedIndex)) {
        QStyleOptionTabBarBase optTabBase;
        optTabBase.initFrom(this);
        optTabBase.documentMode = d->documentMode;
        if (event->type() == style()->styleHint(QStyle::SH_TabBar_SelectMouseType, &optTabBase, this))
            setCurrentIndex(d->pressedIndex);
        else
            repaint(tabRect(d->pressedIndex));
        if (d->movable) {
            d->dragStartPosition = event->position().toPoint();
        }
    }
}

/*!\reimp
 */
void QTabBar::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QTabBar);
    if (d->movable) {
        // Be safe!
        if (d->pressedIndex != -1
            && event->buttons() == Qt::NoButton)
            d->moveTabFinished(d->pressedIndex);

        // Start drag
        if (!d->dragInProgress && d->pressedIndex != -1) {
            if ((event->position().toPoint() - d->dragStartPosition).manhattanLength() > QApplication::startDragDistance()) {
                d->dragInProgress = true;
                d->setupMovableTab();
            }
        }

        if (event->buttons() == Qt::LeftButton
            && d->dragInProgress
            && d->validIndex(d->pressedIndex)) {
            bool vertical = verticalTabs(d->shape);
            int dragDistance;
            if (vertical) {
                dragDistance = (event->position().toPoint().y() - d->dragStartPosition.y());
            } else {
                dragDistance = (event->position().toPoint().x() - d->dragStartPosition.x());
            }
            d->tabList.at(d->pressedIndex)->dragOffset = dragDistance;

            QRect startingRect = tabRect(d->pressedIndex);
            if (vertical)
                startingRect.moveTop(startingRect.y() + dragDistance);
            else
                startingRect.moveLeft(startingRect.x() + dragDistance);

            int overIndex;
            if (dragDistance < 0)
                overIndex = tabAt(startingRect.topLeft());
            else
                overIndex = tabAt(startingRect.topRight());

            if (overIndex != d->pressedIndex && overIndex != -1) {
                int offset = 1;
                if (isRightToLeft() && !vertical)
                    offset *= -1;
                if (dragDistance < 0) {
                    dragDistance *= -1;
                    offset *= -1;
                }
                for (int i = d->pressedIndex;
                     offset > 0 ? i < overIndex : i > overIndex;
                     i += offset) {
                    QRect overIndexRect = tabRect(overIndex);
                    int needsToBeOver = (vertical ? overIndexRect.height() : overIndexRect.width()) / 2;
                    if (dragDistance > needsToBeOver)
                        d->slide(i + offset, d->pressedIndex);
                }
            }
            // Buttons needs to follow the dragged tab
            if (d->pressedIndex != -1)
                d->layoutTab(d->pressedIndex);

            update();
        }
    }

    if (event->buttons() != Qt::LeftButton) {
        event->ignore();
        return;
    }
}

void QTabBarPrivate::setupMovableTab()
{
    Q_Q(QTabBar);
    if (!movingTab)
        movingTab = new QMovableTabWidget(q);

    int taboverlap = q->style()->pixelMetric(QStyle::PM_TabBarTabOverlap, nullptr ,q);
    QRect grabRect = q->tabRect(pressedIndex);
    if (verticalTabs(shape))
        grabRect.adjust(0, -taboverlap, 0, taboverlap);
    else
        grabRect.adjust(-taboverlap, 0, taboverlap, 0);

    QPixmap grabImage(grabRect.size() * q->devicePixelRatio());
    grabImage.setDevicePixelRatio(q->devicePixelRatio());
    grabImage.fill(Qt::transparent);
    QStylePainter p(&grabImage, q);

    QStyleOptionTab tab;
    q->initStyleOption(&tab, pressedIndex);
    tab.position = QStyleOptionTab::Moving;
    if (verticalTabs(shape))
        tab.rect.moveTopLeft(QPoint(0, taboverlap));
    else
        tab.rect.moveTopLeft(QPoint(taboverlap, 0));
    p.drawControl(QStyle::CE_TabBarTab, tab);
    p.end();

    movingTab->setPixmap(grabImage);
    movingTab->setGeometry(grabRect);
    movingTab->raise();

    // Re-arrange widget order to avoid overlaps
    const auto &pressedTab = *tabList.at(pressedIndex);
    if (pressedTab.leftWidget)
        pressedTab.leftWidget->raise();
    if (pressedTab.rightWidget)
        pressedTab.rightWidget->raise();
    if (leftB)
        leftB->raise();
    if (rightB)
        rightB->raise();
    movingTab->setVisible(true);
}

void QTabBarPrivate::moveTabFinished(int index)
{
    Q_Q(QTabBar);
    bool cleanup = (pressedIndex == index) || (pressedIndex == -1) || !validIndex(index);
    bool allAnimationsFinished = true;
#if QT_CONFIG(animation)
    for (const auto tab : std::as_const(tabList)) {
        if (tab->animation && tab->animation->state() == QAbstractAnimation::Running) {
            allAnimationsFinished = false;
            break;
        }
    }
#endif // animation
    if (allAnimationsFinished && cleanup) {
        if (movingTab)
            movingTab->setVisible(false); // We might not get a mouse release
        for (auto tab : std::as_const(tabList)) {
            tab->dragOffset = 0;
        }
        if (pressedIndex != -1 && movable) {
            pressedIndex = -1;
            dragInProgress = false;
            dragStartPosition = QPoint();
        }
        layoutWidgets();
    } else {
        if (!validIndex(index))
            return;
        tabList.at(index)->dragOffset = 0;
    }
    q->update();
}

/*!\reimp
*/
void QTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QTabBar);
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    if (d->movable && d->dragInProgress && d->validIndex(d->pressedIndex)) {
        int length = d->tabList.at(d->pressedIndex)->dragOffset;
        int width = verticalTabs(d->shape)
            ? tabRect(d->pressedIndex).height()
            : tabRect(d->pressedIndex).width();
        int duration = qMin(ANIMATION_DURATION,
                (qAbs(length) * ANIMATION_DURATION) / width);
        d->tabList.at(d->pressedIndex)->startAnimation(d, duration);
        d->dragInProgress = false;
        d->movingTab->setVisible(false);
        d->dragStartPosition = QPoint();
    }

    // mouse release event might happen outside the tab, so keep the pressed index
    int oldPressedIndex = d->pressedIndex;
    int i = d->indexAtPos(event->position().toPoint()) == d->pressedIndex ? d->pressedIndex : -1;
    d->pressedIndex = -1;
    QStyleOptionTabBarBase optTabBase;
    optTabBase.initFrom(this);
    optTabBase.documentMode = d->documentMode;
    const bool selectOnRelease =
            (style()->styleHint(QStyle::SH_TabBar_SelectMouseType, &optTabBase, this) == QEvent::MouseButtonRelease);
    if (selectOnRelease)
        setCurrentIndex(i);
    if (d->validIndex(oldPressedIndex))
        update(tabRect(oldPressedIndex));
}

/*!\reimp
 */
void QTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QTabBar);
    const QPoint pos = event->position().toPoint();
    const bool isEventInCornerButtons = (!d->leftB->isHidden() && d->leftB->geometry().contains(pos))
                                        || (!d->rightB->isHidden() && d->rightB->geometry().contains(pos));
    if (!isEventInCornerButtons)
        emit tabBarDoubleClicked(tabAt(pos));

    mousePressEvent(event);
}

/*!\reimp
 */
void QTabBar::keyPressEvent(QKeyEvent *event)
{
    Q_D(QTabBar);
    if (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right) {
        event->ignore();
        return;
    }
    int offset = event->key() == (isRightToLeft() ? Qt::Key_Right : Qt::Key_Left) ? -1 : 1;
    d->setCurrentNextEnabledIndex(offset);
}

/*!\reimp
 */
#if QT_CONFIG(wheelevent)
void QTabBar::wheelEvent(QWheelEvent *event)
{
    Q_D(QTabBar);
    if (style()->styleHint(QStyle::SH_TabBar_AllowWheelScrolling)) {
        const bool wheelVertical = qAbs(event->angleDelta().y()) > qAbs(event->angleDelta().x());
        const bool tabsVertical = verticalTabs(d->shape);
        if (event->device()->capabilities().testFlag(QInputDevice::Capability::PixelScroll)) {
            // For wheels/touch pads with pixel precision, scroll the tab bar if
            // it has the right orientation.
            int delta = 0;
            if (tabsVertical == wheelVertical)
                delta = wheelVertical ? event->pixelDelta().y() : event->pixelDelta().x();
            if (layoutDirection() == Qt::RightToLeft)
                delta = -delta;
            if (delta && d->validIndex(d->lastVisible)) {
                const int oldScrollOffset = d->scrollOffset;
                const QRect lastTabRect = d->tabList.at(d->lastVisible)->rect;
                const QRect scrollRect = d->normalizedScrollRect(d->lastVisible);
                int scrollRectExtent = scrollRect.right();
                if (!d->leftB->isVisible())
                    scrollRectExtent += tabsVertical ? d->leftB->height() : d->leftB->width();
                if (!d->rightB->isVisible())
                    scrollRectExtent += tabsVertical ? d->rightB->height() : d->rightB->width();

                const int maxScrollOffset = qMax((tabsVertical ?
                                                  lastTabRect.bottom() :
                                                  lastTabRect.right()) - scrollRectExtent, 0);
                d->scrollOffset = qBound(0, d->scrollOffset - delta, maxScrollOffset);
                d->leftB->setEnabled(d->scrollOffset > -scrollRect.left());
                d->rightB->setEnabled(maxScrollOffset > d->scrollOffset);
                if (oldScrollOffset != d->scrollOffset) {
                    event->accept();
                    update();
                    return;
                }
            }
        } else {
            d->accumulatedAngleDelta += event->angleDelta();
            const int xSteps = d->accumulatedAngleDelta.x() / QWheelEvent::DefaultDeltasPerStep;
            const int ySteps = d->accumulatedAngleDelta.y() / QWheelEvent::DefaultDeltasPerStep;
            int offset = 0;
            if (xSteps > 0 || ySteps > 0) {
                offset = -1;
                d->accumulatedAngleDelta = QPoint();
            } else if (xSteps < 0 || ySteps < 0) {
                offset = 1;
                d->accumulatedAngleDelta = QPoint();
            }
            const int oldCurrentIndex = d->currentIndex;
            d->setCurrentNextEnabledIndex(offset);
            if (oldCurrentIndex != d->currentIndex) {
                event->accept();
                return;
            }
        }
        QWidget::wheelEvent(event);
    }
}
#endif // QT_CONFIG(wheelevent)

void QTabBarPrivate::setCurrentNextEnabledIndex(int offset)
{
    Q_Q(QTabBar);
    for (int index = currentIndex + offset; validIndex(index); index += offset) {
        if (tabList.at(index)->enabled && tabList.at(index)->visible) {
            q->setCurrentIndex(index);
            break;
        }
    }
}

/*!\reimp
 */
void QTabBar::changeEvent(QEvent *event)
{
    Q_D(QTabBar);
    switch (event->type()) {
    case QEvent::StyleChange:
        if (!d->elideModeSetByUser)
            d->elideMode = Qt::TextElideMode(style()->styleHint(QStyle::SH_TabBar_ElideMode, nullptr, this));
        if (!d->useScrollButtonsSetByUser)
            d->useScrollButtons = !style()->styleHint(QStyle::SH_TabBar_PreferNoArrows, nullptr, this);
        Q_FALLTHROUGH();
    case QEvent::FontChange:
        d->textSizes.clear();
        d->refresh();
        break;
    default:
        break;
    }

    QWidget::changeEvent(event);
}

/*!
    \reimp
*/
void QTabBar::timerEvent(QTimerEvent *event)
{
    Q_D(QTabBar);
    if (event->timerId() == d->switchTabTimerId) {
        killTimer(d->switchTabTimerId);
        d->switchTabTimerId = 0;
        setCurrentIndex(d->switchTabCurrentIndex);
        d->switchTabCurrentIndex = -1;
    }
    QWidget::timerEvent(event);
}

/*!
    \property QTabBar::elideMode
    \brief how to elide text in the tab bar
    \since 4.2

    This property controls how items are elided when there is not
    enough space to show them for a given tab bar size.

    By default the value is style-dependent.

    \sa QTabWidget::elideMode, usesScrollButtons, QStyle::SH_TabBar_ElideMode
*/

Qt::TextElideMode QTabBar::elideMode() const
{
    Q_D(const QTabBar);
    return d->elideMode;
}

void QTabBar::setElideMode(Qt::TextElideMode mode)
{
    Q_D(QTabBar);
    d->elideMode = mode;
    d->elideModeSetByUser = true;
    d->textSizes.clear();
    d->refresh();
}

/*!
    \property QTabBar::usesScrollButtons
    \brief Whether or not a tab bar should use buttons to scroll tabs when it
    has many tabs.
    \since 4.2

    When there are too many tabs in a tab bar for its size, the tab bar can either choose
    to expand its size or to add buttons that allow you to scroll through the tabs.

    By default the value is style-dependent.

    \sa elideMode, QTabWidget::usesScrollButtons, QStyle::SH_TabBar_PreferNoArrows
*/
bool QTabBar::usesScrollButtons() const
{
    return d_func()->useScrollButtons;
}

void QTabBar::setUsesScrollButtons(bool useButtons)
{
    Q_D(QTabBar);
    d->useScrollButtonsSetByUser = true;
    if (d->useScrollButtons == useButtons)
        return;
    d->useScrollButtons = useButtons;
    d->refresh();
}

/*!
    \property QTabBar::tabsClosable
    \brief Whether or not a tab bar should place close buttons on each tab
    \since 4.5

    When tabsClosable is set to true a close button will appear on the tab on
    either the left or right hand side depending upon the style.  When the button
    is clicked the tab the signal tabCloseRequested will be emitted.

    By default the value is false.

    \sa setTabButton(), tabRemoved()
*/

bool QTabBar::tabsClosable() const
{
    Q_D(const QTabBar);
    return d->closeButtonOnTabs;
}

void QTabBar::setTabsClosable(bool closable)
{
    Q_D(QTabBar);
    if (d->closeButtonOnTabs == closable)
        return;
    d->closeButtonOnTabs = closable;
    ButtonPosition closeSide = (ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, this);
    if (!closable) {
        for (auto tab : std::as_const(d->tabList)) {
            if (closeSide == LeftSide && tab->leftWidget) {
                tab->leftWidget->deleteLater();
                tab->leftWidget = nullptr;
            }
            if (closeSide == RightSide && tab->rightWidget) {
                tab->rightWidget->deleteLater();
                tab->rightWidget = nullptr;
            }
        }
    } else {
        bool newButtons = false;
        for (int i = 0; i < d->tabList.size(); ++i) {
            if (tabButton(i, closeSide))
                continue;
            newButtons = true;
            QAbstractButton *closeButton = new CloseButton(this);
            connect(closeButton, SIGNAL(clicked()), this, SLOT(_q_closeTab()));
            setTabButton(i, closeSide, closeButton);
        }
        if (newButtons)
            d->layoutTabs();
    }
    update();
}

/*!
    \enum QTabBar::ButtonPosition
    \since 4.5

    This enum type lists the location of the widget on a tab.

    \value LeftSide Left side of the tab.

    \value RightSide Right side of the tab.

*/

/*!
    \enum QTabBar::SelectionBehavior
    \since 4.5

    This enum type lists the behavior of QTabBar when a tab is removed
    and the tab being removed is also the current tab.

    \value SelectLeftTab  Select the tab to the left of the one being removed.

    \value SelectRightTab  Select the tab to the right of the one being removed.

    \value SelectPreviousTab  Select the previously selected tab.

*/

/*!
    \property QTabBar::selectionBehaviorOnRemove
    \brief What tab should be set as current when removeTab is called if
    the removed tab is also the current tab.
    \since 4.5

    By default the value is SelectRightTab.

    \sa removeTab()
*/


QTabBar::SelectionBehavior QTabBar::selectionBehaviorOnRemove() const
{
    Q_D(const QTabBar);
    return d->selectionBehaviorOnRemove;
}

void QTabBar::setSelectionBehaviorOnRemove(QTabBar::SelectionBehavior behavior)
{
    Q_D(QTabBar);
    d->selectionBehaviorOnRemove = behavior;
}

/*!
    \property QTabBar::expanding
    \brief When expanding is true QTabBar will expand the tabs to use the empty space.
    \since 4.5

    By default the value is true.

    \sa QTabWidget::documentMode
*/

bool QTabBar::expanding() const
{
    Q_D(const QTabBar);
    return d->expanding;
}

void QTabBar::setExpanding(bool enabled)
{
    Q_D(QTabBar);
    if (d->expanding == enabled)
        return;
    d->expanding = enabled;
    d->layoutTabs();
}

/*!
    \property QTabBar::movable
    \brief This property holds whether the user can move the tabs
    within the tabbar area.

    \since 4.5

    By default, this property is \c false;
*/

bool QTabBar::isMovable() const
{
    Q_D(const QTabBar);
    return d->movable;
}

void QTabBar::setMovable(bool movable)
{
    Q_D(QTabBar);
    d->movable = movable;
}


/*!
    \property QTabBar::documentMode
    \brief Whether or not the tab bar is rendered in a mode suitable for the main window.
    \since 4.5

    This property is used as a hint for styles to draw the tabs in a different
    way then they would normally look in a tab widget.  On \macos this will
    look similar to the tabs in Safari or Sierra's Terminal.app.

    \sa QTabWidget::documentMode
*/
bool QTabBar::documentMode() const
{
    return d_func()->documentMode;
}

void QTabBar::setDocumentMode(bool enabled)
{
    Q_D(QTabBar);

    d->documentMode = enabled;
    d->updateMacBorderMetrics();
}

/*!
    \property QTabBar::autoHide
    \brief If true, the tab bar is automatically hidden when it contains less
    than 2 tabs.
    \since 5.4

    By default, this property is false.

    \sa QWidget::visible
*/

bool QTabBar::autoHide() const
{
    Q_D(const QTabBar);
    return d->autoHide;
}

void QTabBar::setAutoHide(bool hide)
{
    Q_D(QTabBar);
    if (d->autoHide == hide)
        return;

    d->autoHide = hide;
    if (hide)
        d->autoHideTabs();
    else
        setVisible(true);
}

/*!
    \property QTabBar::changeCurrentOnDrag
    \brief If true, then the current tab is automatically changed when dragging
    over the tabbar.
    \since 5.4

    \note You should also set acceptDrops property to true to make this feature
    work.

    By default, this property is false.
*/

bool QTabBar::changeCurrentOnDrag() const
{
    Q_D(const QTabBar);
    return d->changeCurrentOnDrag;
}

void QTabBar::setChangeCurrentOnDrag(bool change)
{
    Q_D(QTabBar);
    d->changeCurrentOnDrag = change;
    if (!change)
        d->killSwitchTabTimer();
}

/*!
    Sets \a widget on the tab \a index.  The widget is placed
    on the left or right hand side depending on the \a position.
    \since 4.5

    Any previously set widget in \a position is hidden. Setting \a widget
    to \nullptr will hide the current widget at \a position.

    The tab bar will take ownership of the widget and so all widgets set here
    will be deleted by the tab bar when it is destroyed unless you separately
    reparent the widget after setting some other widget (or \nullptr).

    \sa tabsClosable()
  */
void QTabBar::setTabButton(int index, ButtonPosition position, QWidget *widget)
{
    Q_D(QTabBar);
    if (index < 0 || index >= d->tabList.size())
        return;
    if (widget) {
        widget->setParent(this);
        // make sure our left and right widgets stay on top
        widget->lower();
        widget->show();
    }
    auto &tab = *d->tabList.at(index);
    if (position == LeftSide) {
        if (tab.leftWidget)
            tab.leftWidget->hide();
        tab.leftWidget = widget;
    } else {
        if (tab.rightWidget)
            tab.rightWidget->hide();
        tab.rightWidget = widget;
    }
    d->layoutTabs();
    d->refresh();
    update();
}

/*!
    Returns the widget set a tab \a index and \a position or \nullptr
    if one is not set.
  */
QWidget *QTabBar::tabButton(int index, ButtonPosition position) const
{
    Q_D(const QTabBar);
    if (const auto tab = d->at(index)) {
        return position == LeftSide ? tab->leftWidget
                                    : tab->rightWidget;
    }
    return nullptr;
}

#if QT_CONFIG(accessibility)
/*!
    Sets the accessibleName of the tab at position \a index to \a name.
*/
void QTabBar::setAccessibleTabName(int index, const QString &name)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        tab->accessibleName = name;
        QAccessibleEvent event(this, QAccessible::NameChanged);
        event.setChild(index);
        QAccessible::updateAccessibility(&event);
    }
}

/*!
    Returns the accessibleName of the tab at position \a index, or an empty
    string if \a index is out of range.
*/
QString QTabBar::accessibleTabName(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->accessibleName;
    return QString();
}
#endif // QT_CONFIG(accessibility)

CloseButton::CloseButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif
#if QT_CONFIG(tooltip)
    setToolTip(tr("Close Tab"));
#endif
    resize(sizeHint());
}

QSize CloseButton::sizeHint() const
{
    ensurePolished();
    int width = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, nullptr, this);
    int height = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, nullptr, this);
    return QSize(width, height);
}

void CloseButton::enterEvent(QEnterEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::enterEvent(event);
}

void CloseButton::leaveEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::leaveEvent(event);
}

void CloseButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    opt.state |= QStyle::State_AutoRaise;
    if (isEnabled() && underMouse() && !isChecked() && !isDown())
        opt.state |= QStyle::State_Raised;
    if (isChecked())
        opt.state |= QStyle::State_On;
    if (isDown())
        opt.state |= QStyle::State_Sunken;

    if (const QTabBar *tb = qobject_cast<const QTabBar *>(parent())) {
        int index = tb->currentIndex();
        QTabBar::ButtonPosition position = (QTabBar::ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, tb);
        if (tb->tabButton(index, position) == this)
            opt.state |= QStyle::State_Selected;
    }

    style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &opt, &p, this);
}

#if QT_CONFIG(animation)
void QTabBarPrivate::Tab::TabBarAnimation::updateCurrentValue(const QVariant &current)
{
    priv->moveTab(priv->tabList.indexOf(tab), current.toInt());
}

void QTabBarPrivate::Tab::TabBarAnimation::updateState(QAbstractAnimation::State newState, QAbstractAnimation::State)
{
    if (newState == Stopped) priv->moveTabFinished(priv->tabList.indexOf(tab));
}
#endif

QT_END_NAMESPACE

#include "moc_qtabbar.cpp"
#include "qtabbar.moc"
