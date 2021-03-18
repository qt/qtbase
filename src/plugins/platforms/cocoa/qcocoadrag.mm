/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoadrag.h"
#include "qmacclipboard.h"
#include "qcocoahelpers.h"
#ifndef QT_NO_WIDGETS
#include <QtWidgets/qwidget.h>
#endif
#include <QtGui/private/qcoregraphics_p.h>
#include <QtCore/qsysinfo.h>

#include <vector>

QT_BEGIN_NAMESPACE

static const int dragImageMaxChars = 26;

QCocoaDrag::QCocoaDrag() :
    m_drag(nullptr)
{
    m_lastEvent = nil;
    m_lastView = nil;
}

QCocoaDrag::~QCocoaDrag()
{
    [m_lastEvent release];
}

void QCocoaDrag::setLastMouseEvent(NSEvent *event, NSView *view)
{
    [m_lastEvent release];
    m_lastEvent = [event copy];
    m_lastView = view;
}

QMimeData *QCocoaDrag::dragMimeData()
{
    if (m_drag)
        return m_drag->mimeData();

    return nullptr;
}

Qt::DropAction QCocoaDrag::defaultAction(Qt::DropActions possibleActions,
                                           Qt::KeyboardModifiers modifiers) const
{
    Qt::DropAction default_action = Qt::IgnoreAction;

    if (currentDrag()) {
        default_action = currentDrag()->defaultAction();
        possibleActions = currentDrag()->supportedActions();
    }

    if (default_action == Qt::IgnoreAction) {
        //This means that the drag was initiated by QDrag::start and we need to
        //preserve the old behavior
        default_action = Qt::CopyAction;
    }

    if (modifiers & Qt::ControlModifier && modifiers & Qt::AltModifier)
        default_action = Qt::LinkAction;
    else if (modifiers & Qt::AltModifier)
        default_action = Qt::CopyAction;
    else if (modifiers & Qt::ControlModifier)
        default_action = Qt::MoveAction;

#ifdef QDND_DEBUG
    qDebug("possible actions : %s", dragActionsToString(possibleActions).latin1());
#endif

    // Check if the action determined is allowed
    if (!(possibleActions & default_action)) {
        if (possibleActions & Qt::CopyAction)
            default_action = Qt::CopyAction;
        else if (possibleActions & Qt::MoveAction)
            default_action = Qt::MoveAction;
        else if (possibleActions & Qt::LinkAction)
            default_action = Qt::LinkAction;
        else
            default_action = Qt::IgnoreAction;
    }

#ifdef QDND_DEBUG
    qDebug("default action : %s", dragActionsToString(default_action).latin1());
#endif

    return default_action;
}


Qt::DropAction QCocoaDrag::drag(QDrag *o)
{
    m_drag = o;
    m_executed_drop_action = Qt::IgnoreAction;

    QMacPasteboard dragBoard(CFStringRef(NSPasteboardNameDrag), QMacInternalPasteboardMime::MIME_DND);
    m_drag->mimeData()->setData(QLatin1String("application/x-qt-mime-type-name"), QByteArray("dummy"));
    dragBoard.setMimeData(m_drag->mimeData(), QMacPasteboard::LazyRequest);

    if (maybeDragMultipleItems())
        return m_executed_drop_action;

    QPoint hotSpot = m_drag->hotSpot();
    QPixmap pm = dragPixmap(m_drag, hotSpot);
    NSImage *dragImage = [NSImage imageFromQImage:pm.toImage()];
    Q_ASSERT(dragImage);

    NSPoint event_location = [m_lastEvent locationInWindow];
    NSWindow *theWindow = [m_lastEvent window];
    Q_ASSERT(theWindow);
    event_location.x -= hotSpot.x();
    CGFloat flippedY = dragImage.size.height - hotSpot.y();
    event_location.y -= flippedY;
    NSSize mouseOffset_unused = NSMakeSize(0.0, 0.0);
    NSPasteboard *pboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];

    [theWindow dragImage:dragImage
        at:event_location
        offset:mouseOffset_unused
        event:m_lastEvent
        pasteboard:pboard
        source:m_lastView
        slideBack:YES];

    m_drag = nullptr;
    return m_executed_drop_action;
}

bool QCocoaDrag::maybeDragMultipleItems()
{
    Q_ASSERT(m_drag && m_drag->mimeData());
    Q_ASSERT(m_executed_drop_action == Qt::IgnoreAction);

    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::MacOSMojave) {
        // -dragImage: stopped working in 10.14 first.
        return false;
    }

    const QMacAutoReleasePool pool;

    NSWindow *theWindow = [m_lastEvent window];
    Q_ASSERT(theWindow);

    if (![theWindow.contentView respondsToSelector:@selector(draggingSession:sourceOperationMaskForDraggingContext:)])
        return false;

    auto *sourceView = static_cast<NSView<NSDraggingSource>*>(theWindow.contentView);

    const auto &qtUrls = m_drag->mimeData()->urls();
    NSPasteboard *dragBoard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];

    if (qtUrls.size() <= 1) {
        // Good old -dragImage: works perfectly for this ...
        return false;
    }

    std::vector<NSPasteboardItem *> nonUrls;
    for (NSPasteboardItem *item in dragBoard.pasteboardItems) {
        bool isUrl = false;
        for (NSPasteboardType type in item.types) {
            using NSStringRef = NSString *;
            if ([type isEqualToString:NSStringRef(kUTTypeFileURL)]) {
                isUrl = true;
                break;
            }
        }

        if (!isUrl)
            nonUrls.push_back(item);
    }

    QPoint hotSpot = m_drag->hotSpot();
    const auto pixmap = dragPixmap(m_drag, hotSpot);
    NSImage *dragImage = [NSImage imageFromQImage:pixmap.toImage()];
    Q_ASSERT(dragImage);

    NSMutableArray<NSDraggingItem *> *dragItems = [[[NSMutableArray alloc] init] autorelease];
    const NSPoint itemLocation = m_drag->hotSpot().toCGPoint();
    // 0. We start from URLs, which can be actually in a list (thus technically
    // only ONE item in the pasteboard. The fact it's only one does not help, we are
    // still getting an exception because of the number of items/images mismatch ...
    // We only set the image for the first item and nil for the rest, the image already
    // contains a combined picture for all urls we drag.
    auto imageOrNil = dragImage;
    for (const auto &qtUrl : qtUrls) {
        NSURL *nsUrl = qtUrl.toNSURL();
        auto *newItem = [[[NSDraggingItem alloc] initWithPasteboardWriter:nsUrl] autorelease];
        const NSRect itemFrame = NSMakeRect(itemLocation.x, itemLocation.y,
                                            dragImage.size.width,
                                            dragImage.size.height);

        [newItem setDraggingFrame:itemFrame contents:imageOrNil];
        imageOrNil = nil;
        [dragItems addObject:newItem];
    }
    // 1. Repeat for non-url items, if any:
    for (auto *pbItem : nonUrls) {
        auto *newItem = [[[NSDraggingItem alloc] initWithPasteboardWriter:pbItem] autorelease];
        const NSRect itemFrame = NSMakeRect(itemLocation.x, itemLocation.y,
                                            dragImage.size.width,
                                            dragImage.size.height);
        [newItem setDraggingFrame:itemFrame contents:imageOrNil];
        [dragItems addObject:newItem];
    }

    [sourceView beginDraggingSessionWithItems:dragItems event:m_lastEvent source:sourceView];
    internalDragLoop.exec();
    return true;
}

void QCocoaDrag::setAcceptedAction(Qt::DropAction act)
{
    m_executed_drop_action = act;
}

void QCocoaDrag::exitDragLoop()
{
    if (internalDragLoop.isRunning())
        internalDragLoop.exit();
}


QPixmap QCocoaDrag::dragPixmap(QDrag *drag, QPoint &hotSpot) const
{
    const QMimeData* data = drag->mimeData();
    QPixmap pm = drag->pixmap();

    if (pm.isNull()) {
        QFont f(qApp->font());
        f.setPointSize(12);
        QFontMetrics fm(f);

        if (data->hasImage()) {
            const QImage img = data->imageData().value<QImage>();
            if (!img.isNull()) {
                pm = QPixmap::fromImage(img).scaledToWidth(dragImageMaxChars *fm.averageCharWidth());
            }
        }

        if (pm.isNull() && (data->hasText() || data->hasUrls()) ) {
            QString s = data->hasText() ? data->text() : data->urls().first().toString();
            if (s.length() > dragImageMaxChars)
                s = s.left(dragImageMaxChars -3) + QChar(0x2026);
            if (!s.isEmpty()) {
                const int width = fm.horizontalAdvance(s);
                const int height = fm.height();
                if (width > 0 && height > 0) {
                    qreal dpr = 1.0;
                    if (const QWindow *sourceWindow = qobject_cast<QWindow *>(drag->source())) {
                        dpr = sourceWindow->devicePixelRatio();
                    }
#ifndef QT_NO_WIDGETS
                    else if (const QWidget *sourceWidget = qobject_cast<QWidget *>(drag->source())) {
                        if (const QWindow *sourceWindow = sourceWidget->window()->windowHandle())
                            dpr = sourceWindow->devicePixelRatio();
                    }
#endif
                    else {
                        if (const QWindow *focusWindow = qApp->focusWindow())
                            dpr = focusWindow->devicePixelRatio();
                    }
                    pm = QPixmap(width * dpr, height * dpr);
                    pm.setDevicePixelRatio(dpr);
                    QPainter p(&pm);
                    p.fillRect(0, 0, pm.width(), pm.height(), Qt::color0);
                    p.setPen(Qt::color1);
                    p.setFont(f);
                    p.drawText(0, fm.ascent(), s);
                    p.end();
                    hotSpot = QPoint(pm.width() / 2, pm.height() / 2);
                }
            }
        }
    }

    if (pm.isNull())
        pm = defaultPixmap();

    return pm;
}

QCocoaDropData::QCocoaDropData(NSPasteboard *pasteboard)
{
    dropPasteboard = reinterpret_cast<CFStringRef>(const_cast<const NSString *>([pasteboard name]));
    CFRetain(dropPasteboard);
}

QCocoaDropData::~QCocoaDropData()
{
    CFRelease(dropPasteboard);
}

QStringList QCocoaDropData::formats_sys() const
{
    QStringList formats;
    PasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return formats;
    }
    formats = QMacPasteboard(board, QMacInternalPasteboardMime::MIME_DND).formats();
    return formats;
}

QVariant QCocoaDropData::retrieveData_sys(const QString &mimeType, QVariant::Type type) const
{
    QVariant data;
    PasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return data;
    }
    data = QMacPasteboard(board, QMacInternalPasteboardMime::MIME_DND).retrieveData(mimeType, type);
    CFRelease(board);
    return data;
}

bool QCocoaDropData::hasFormat_sys(const QString &mimeType) const
{
    bool has = false;
    PasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return has;
    }
    has = QMacPasteboard(board, QMacInternalPasteboardMime::MIME_DND).hasFormat(mimeType);
    CFRelease(board);
    return has;
}


QT_END_NAMESPACE

