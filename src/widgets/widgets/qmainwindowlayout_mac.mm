/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include <private/qmainwindowlayout_p.h>
#include <qtoolbar.h>
#include <private/qtoolbarlayout_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qtoolbar_p.h>

#include <private/qcocoatoolbardelegate_mac_p.h>
#import  <private/qcocoawindowdelegate_mac_p.h>

QT_BEGIN_NAMESPACE
#ifdef QT_NAMESPACE

// namespace up the stuff
#define SS(x) #x
#define S0(x) SS(x)
#define S "org.qt-project.qt-" S0(QT_NAMESPACE) ".qmainwindow.qtoolbarInHIToolbar"
#define SToolbar "org.qt-project.qt-" S0(QT_NAMESPACE) ".hitoolbar-qtoolbar"
#define SNSToolbar "org.qt-project.qt-" S0(QT_NAMESPACE) ".qtoolbarInNSToolbar"
#define MacToolbar "org.qt-project.qt-" S0(QT_NAMESPACE) ".qmainwindow.mactoolbar"

static NSString *kQToolBarNSToolbarIdentifier = @SNSToolbar;
static CFStringRef kQMainWindowMacToolbarID = CFSTR(MacToolbar);
#undef SS
#undef S0
#undef S
#undef SToolbar
#undef SNSToolbar
#undef MacToolbar

#else
static NSString *kQToolBarNSToolbarIdentifier = @"org.qt-project.qt.qmainwindow.qtoolbarInNSToolbar";
static CFStringRef kQMainWindowMacToolbarID = CFSTR("org.qt-project.qt.qmainwindow.mactoolbar");
#endif // QT_NAMESPACE


#ifndef kWindowUnifiedTitleAndToolbarAttribute
#define kWindowUnifiedTitleAndToolbarAttribute (1 << 7)
#endif

void QMainWindowLayout::updateHIToolBarStatus()
{
    bool useMacToolbar = layoutState.mainWindow->unifiedTitleAndToolBarOnMac();

    layoutState.mainWindow->setUpdatesEnabled(false);  // reduces a little bit of flicker, not all though
    QMacCocoaAutoReleasePool pool;
    NSView *cView = [qt_mac_window_for(layoutState.mainWindow) contentView];
    if (useMacToolbar) {
        [cView setPostsFrameChangedNotifications:YES];
        [[NSNotificationCenter defaultCenter] addObserver: [QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate]
                                                 selector: @selector(syncContentViewFrame:)
                                                     name: NSViewFrameDidChangeNotification
                                                   object: cView];
    }
    if (!useMacToolbar) {
        macWindowToolbarShow(layoutState.mainWindow, false);
        // Move everything out of the HIToolbar into the main toolbar.
        while (!qtoolbarsInUnifiedToolbarList.isEmpty()) {
            // Should shrink the list by one every time.
            QToolBar *toolbar = qtoolbarsInUnifiedToolbarList.first();
            unifiedSurface->removeToolbar(toolbar);
            layoutState.mainWindow->addToolBar(Qt::TopToolBarArea, toolbar);
        }
        macWindowToolbarSet(qt_mac_window_for(layoutState.mainWindow), 0);
    } else {
        QList<QToolBar *> toolbars = layoutState.mainWindow->findChildren<QToolBar *>();
        for (int i = 0; i < toolbars.size(); ++i) {
            QToolBar *toolbar = toolbars.at(i);
            if (toolBarArea(toolbar) == Qt::TopToolBarArea) {
                // Do this here, because we are in an in-between state.
                removeWidget(toolbar);
                layoutState.mainWindow->addToolBar(Qt::TopToolBarArea, toolbar);
            }
        }
        syncUnifiedToolbarVisibility();
    }
    if (!useMacToolbar) {
        [cView setPostsFrameChangedNotifications:NO];
        [[NSNotificationCenter defaultCenter] removeObserver: [QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate]
                                                        name: NSViewFrameDidChangeNotification
                                                      object: cView];
    }
    layoutState.mainWindow->setUpdatesEnabled(true);
}

void QMainWindowLayout::insertIntoMacToolbar(QToolBar *before, QToolBar *toolbar)
{
    // This layering could go on to one more level, but I decided to stop here.
    // The HIToolbar and NSToolbar APIs are fairly similar as you will see.
    if (toolbar == 0)
        return;

    // toolbar will now become native (if not already) since we need
    // an nsview for it inside the corresponding NSToolbarItem.
    // Setting isInUnifiedToolbar will (among other things) stop alien
    // siblings from becoming native when this happends since the toolbar
    // will not overlap with other children of the QMainWindow. NB: Switching
    // unified toolbar off after this stage is not supported, as this means
    // that either the menubar must be alien again, or the sibling must
    // be backed by an nsview to protect from overlapping issues:
    toolbar->d_func()->isInUnifiedToolbar = true;

    QToolBarLayout *toolbarLayout = static_cast<QToolBarLayout *>(toolbar->layout());
    toolbarSaveState.insert(toolbar, ToolBarSaveState(toolbar->isMovable(), toolbar->maximumSize()));

    if (toolbarLayout->hasExpandFlag() == false)
        toolbar->setMaximumSize(toolbar->sizeHint());

    toolbar->setMovable(false);
    toolbarLayout->setUsePopupMenu(true);
    // Make the toolbar a child of the mainwindow to avoid creating a window.
    toolbar->setParent(layoutState.mainWindow);

    toolbar->winId();  // Now create the OSViewRef.
    layoutState.mainWindow->createWinId();

    OSWindowRef window = qt_mac_window_for(layoutState.mainWindow);
    int beforeIndex = qtoolbarsInUnifiedToolbarList.indexOf(before);
    if (beforeIndex == -1)
        beforeIndex = qtoolbarsInUnifiedToolbarList.size();

    int toolbarIndex = qtoolbarsInUnifiedToolbarList.indexOf(toolbar);

    QMacCocoaAutoReleasePool pool;
    NSToolbar *macToolbar = [window toolbar];
    if (macToolbar == nil) {
        macToolbar = [[NSToolbar alloc] initWithIdentifier:(NSString *)kQMainWindowMacToolbarID];
        [macToolbar setDisplayMode:NSToolbarDisplayModeIconOnly];
        [macToolbar setSizeMode:NSToolbarSizeModeRegular];
        [macToolbar setDelegate:[[QT_MANGLE_NAMESPACE(QCocoaToolBarDelegate) alloc] initWithMainWindowLayout:this]];
        [window setToolbar:macToolbar];
        [macToolbar release];
    }
    if (toolbarIndex != -1) {
        qtoolbarsInUnifiedToolbarList.removeAt(toolbarIndex);
        [macToolbar removeItemAtIndex:toolbarIndex];
    }
    qtoolbarsInUnifiedToolbarList.insert(beforeIndex, toolbar);

    // Adding to the unified toolbar surface for the raster engine.
    if (layoutState.mainWindow->windowSurface()) {
        QPoint offset(0, 0);
        for (int i = 0; i < beforeIndex; ++i) {
            offset.setX(offset.x() + qtoolbarsInUnifiedToolbarList.at(i)->size().width());
        }
        unifiedSurface->insertToolbar(toolbar, offset);
    }

    NSString *toolbarID = kQToolBarNSToolbarIdentifier;
    toolbarID = [toolbarID stringByAppendingFormat:@"%p", toolbar];
    cocoaItemIDToToolbarHash.insert(qt_mac_NSStringToQString(toolbarID), toolbar);
    [macToolbar insertItemWithItemIdentifier:toolbarID atIndex:beforeIndex];
}

void QMainWindowLayout::updateUnifiedToolbarOffset()
{
    QPoint offset(0, 0);

    for (int i = 1; i < qtoolbarsInUnifiedToolbarList.length(); ++i) {
        offset.setX(offset.x() + qtoolbarsInUnifiedToolbarList.at(i - 1)->size().width());
        qtoolbarsInUnifiedToolbarList.at(i)->d_func()->toolbar_offset = offset;
    }
}


void QMainWindowLayout::removeFromMacToolbar(QToolBar *toolbar)
{
    QHash<void *, QToolBar *>::iterator it = unifiedToolbarHash.begin();
    while (it != unifiedToolbarHash.end()) {
        if (it.value() == toolbar) {
            // Rescue our HIView and set it on the mainWindow again.
            bool saveVisible = !toolbar->isHidden();
            toolbar->setParent(0);
            toolbar->setParent(parentWidget());
            toolbar->setVisible(saveVisible);
            ToolBarSaveState saveState = toolbarSaveState.value(toolbar);
            static_cast<QToolBarLayout *>(toolbar->layout())->setUsePopupMenu(false);
            toolbar->setMovable(saveState.movable);
            toolbar->setMaximumSize(saveState.maximumSize);
            toolbarSaveState.remove(toolbar);
            NSToolbarItem *item = static_cast<NSToolbarItem *>(it.key());
            [[qt_mac_window_for(layoutState.mainWindow->window()) toolbar]
                removeItemAtIndex:toolbarItemsCopy.indexOf(item)];
             unifiedToolbarHash.remove(item);
             qtoolbarsInUnifiedToolbarList.removeAll(toolbar);
            break;
        }
        ++it;
    }
}

void QMainWindowLayout::cleanUpMacToolbarItems()
{
    QMacCocoaAutoReleasePool pool;
    for (int i = 0; i < toolbarItemsCopy.size(); ++i) {
        NSToolbarItem *item = static_cast<NSToolbarItem *>(toolbarItemsCopy.at(i));
        [item setView:0];
        CFRelease(toolbarItemsCopy.at(i));
    }
    toolbarItemsCopy.clear();
    unifiedToolbarHash.clear();

    OSWindowRef window = qt_mac_window_for(layoutState.mainWindow);
    NSToolbar *macToolbar = [window toolbar];
    if (macToolbar) {
      [[macToolbar delegate] release];
      [macToolbar setDelegate:nil];
    }
}

void QMainWindowLayout::fixSizeInUnifiedToolbar(QToolBar *tb) const
{
    QHash<void *, QToolBar *>::const_iterator it = unifiedToolbarHash.constBegin();
    NSToolbarItem *item = nil;
    while (it != unifiedToolbarHash.constEnd()) {
        if (tb == it.value()) {
            item = static_cast<NSToolbarItem *>(it.key());
            break;
        }
        ++it;
    }
    if (item) {
        QMacCocoaAutoReleasePool pool;
        QWidgetItem layoutItem(tb);
        QSize size = layoutItem.maximumSize();
        NSSize nssize = NSMakeSize(size.width(), size.height());
        [item setMaxSize:nssize];
        size = layoutItem.minimumSize();
        nssize.width = size.width();
        nssize.height = size.height();
        [item setMinSize:nssize];
    }
}

void QMainWindowLayout::syncUnifiedToolbarVisibility()
{
    if (blockVisiblityCheck)
        return;

    Q_ASSERT(layoutState.mainWindow->unifiedTitleAndToolBarOnMac());
    bool show = false;
    const int ToolBarCount = qtoolbarsInUnifiedToolbarList.count();
    for (int i = 0; i < ToolBarCount; ++i) {
        if (qtoolbarsInUnifiedToolbarList.at(i)->isVisible()) {
            show = true;
            break;
        }
    }
    macWindowToolbarShow(layoutState.mainWindow, show);
}

QT_END_NAMESPACE
