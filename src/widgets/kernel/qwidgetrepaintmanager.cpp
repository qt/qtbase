// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qplatformdefs.h"

#include "qwidgetrepaintmanager_p.h"

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qapplication.h>
#include <QtGui/qpaintengine.h>
#if QT_CONFIG(graphicsview)
#include <QtWidgets/qgraphicsproxywidget.h>
#endif

#include <private/qwidget_p.h>
#include <private/qapplication_p.h>
#include <private/qpaintengine_raster_p.h>
#if QT_CONFIG(graphicseffect)
#include <private/qgraphicseffect_p.h>
#endif
#include <QtGui/private/qwindow_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <qpa/qplatformbackingstore.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QPlatformTextureList, qt_dummy_platformTextureList)

// Watches one or more QPlatformTextureLists for changes in the lock state and
// triggers a backingstore sync when all the registered lists turn into
// unlocked state. This is essential when a custom rhiFlush()
// implementation in a platform plugin is not synchronous and keeps
// holding on to the textures for some time even after returning from there.
class QPlatformTextureListWatcher : public QObject
{
    Q_OBJECT
public:
    QPlatformTextureListWatcher(QWidgetRepaintManager *repaintManager)
        : m_repaintManager(repaintManager) {}

    void watch(QPlatformTextureList *textureList) {
        connect(textureList, SIGNAL(locked(bool)), SLOT(onLockStatusChanged(bool)));
        m_locked[textureList] = textureList->isLocked();
    }

    bool isLocked() const {
        foreach (bool v, m_locked) {
            if (v)
                return true;
        }
        return false;
    }

private slots:
     void onLockStatusChanged(bool locked) {
        QPlatformTextureList *tl = static_cast<QPlatformTextureList *>(sender());
        m_locked[tl] = locked;
        if (!isLocked())
            m_repaintManager->sync();
     }

private:
     QHash<QPlatformTextureList *, bool> m_locked;
     QWidgetRepaintManager *m_repaintManager;
};

// ---------------------------------------------------------------------------

QWidgetRepaintManager::QWidgetRepaintManager(QWidget *topLevel)
    : tlw(topLevel), store(tlw->backingStore())
{
    Q_ASSERT(store);

    // Ensure all existing subsurfaces and static widgets are added to their respective lists.
    updateLists(topLevel);
}

void QWidgetRepaintManager::updateLists(QWidget *cur)
{
    if (!cur)
        return;

    QList<QObject*> children = cur->children();
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget*>(children.at(i));
        if (!child || child->isWindow())
            continue;

        updateLists(child);
    }

    if (cur->testAttribute(Qt::WA_StaticContents))
        addStaticWidget(cur);
}

QWidgetRepaintManager::~QWidgetRepaintManager()
{
    for (int c = 0; c < dirtyWidgets.size(); ++c)
        resetWidget(dirtyWidgets.at(c));
    for (int c = 0; c < dirtyRenderToTextureWidgets.size(); ++c)
        resetWidget(dirtyRenderToTextureWidgets.at(c));
}

/*!
    \internal
    Invalidates the \a r (in widget's coordinates) of the backing store, i.e.
    all widgets intersecting with the region will be repainted when the backing
    store is synced.
*/
template <class T>
void QWidgetPrivate::invalidateBackingStore(const T &r)
{
    if (r.isEmpty())
        return;

    if (QCoreApplication::closingDown())
        return;

    Q_Q(QWidget);
    if (!q->isVisible() || !q->updatesEnabled())
        return;

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    if (!tlwExtra || !tlwExtra->backingStore || !tlwExtra->repaintManager)
        return;

    T clipped(r);
    clipped &= clipRect();
    if (clipped.isEmpty())
        return;

    if (!graphicsEffect && extra && extra->hasMask) {
        QRegion masked(extra->mask);
        masked &= clipped;
        if (masked.isEmpty())
            return;

        tlwExtra->repaintManager->markDirty(masked, q,
            QWidgetRepaintManager::UpdateLater, QWidgetRepaintManager::BufferInvalid);
    } else {
        tlwExtra->repaintManager->markDirty(clipped, q,
            QWidgetRepaintManager::UpdateLater, QWidgetRepaintManager::BufferInvalid);
    }
}
// Needed by tst_QWidget
template Q_AUTOTEST_EXPORT void QWidgetPrivate::invalidateBackingStore<QRect>(const QRect &r);

static inline QRect widgetRectFor(QWidget *, const QRect &r) { return r; }
static inline QRect widgetRectFor(QWidget *widget, const QRegion &) { return widget->rect(); }

/*!
    \internal
    Marks the region of the widget as dirty (if not already marked as dirty) and
    posts an UpdateRequest event to the top-level widget (if not already posted).

    If updateTime is UpdateNow, the event is sent immediately instead of posted.

    If bufferState is BufferInvalid, all widgets intersecting with the region will be dirty.

    If the widget paints directly on screen, the event is sent to the widget
    instead of the top-level widget, and bufferState is completely ignored.
*/
template <class T>
void QWidgetRepaintManager::markDirty(const T &r, QWidget *widget, UpdateTime updateTime, BufferState bufferState)
{
    qCInfo(lcWidgetPainting) << "Marking" << r << "of" << widget << "dirty"
        << "with" << updateTime;

    Q_ASSERT(tlw->d_func()->extra);
    Q_ASSERT(tlw->d_func()->extra->topextra);
    Q_ASSERT(widget->isVisible() && widget->updatesEnabled());
    Q_ASSERT(widget->window() == tlw);
    Q_ASSERT(!r.isEmpty());

#if QT_CONFIG(graphicseffect)
    widget->d_func()->invalidateGraphicsEffectsRecursively();
#endif

    QRect widgetRect = widgetRectFor(widget, r);

    // ---------------------------------------------------------------------------

    if (widget->d_func()->shouldPaintOnScreen()) {
        if (widget->d_func()->dirty.isEmpty()) {
            widget->d_func()->dirty = r;
            sendUpdateRequest(widget, updateTime);
            return;
        } else if (qt_region_strictContains(widget->d_func()->dirty, widgetRect)) {
            if (updateTime == UpdateNow)
                sendUpdateRequest(widget, updateTime);
            return; // Already dirty
        }

        const bool eventAlreadyPosted = !widget->d_func()->dirty.isEmpty();
        widget->d_func()->dirty += r;
        if (!eventAlreadyPosted || updateTime == UpdateNow)
            sendUpdateRequest(widget, updateTime);
        return;
    }

    // ---------------------------------------------------------------------------

    if (QWidgetPrivate::get(widget)->renderToTexture) {
        if (!widget->d_func()->inDirtyList)
            addDirtyRenderToTextureWidget(widget);
        if (!updateRequestSent || updateTime == UpdateNow)
            sendUpdateRequest(tlw, updateTime);
        return;
    }

    // ---------------------------------------------------------------------------

    QRect effectiveWidgetRect = widget->d_func()->effectiveRectFor(widgetRect);
    const QPoint offset = widget->mapTo(tlw, QPoint());
    QRect translatedRect = effectiveWidgetRect.translated(offset);
#if QT_CONFIG(graphicseffect)
    // Graphics effects may exceed window size, clamp
    translatedRect = translatedRect.intersected(QRect(QPoint(), tlw->size()));
#endif
    if (qt_region_strictContains(dirty, translatedRect)) {
        if (updateTime == UpdateNow)
            sendUpdateRequest(tlw, updateTime);
        return; // Already dirty
    }

    // ---------------------------------------------------------------------------

    if (bufferState == BufferInvalid) {
        const bool eventAlreadyPosted = !dirty.isEmpty() || updateRequestSent;
#if QT_CONFIG(graphicseffect)
        if (widget->d_func()->graphicsEffect)
            dirty += widget->d_func()->effectiveRectFor(r).translated(offset);
        else
#endif
            dirty += r.translated(offset);

        if (!eventAlreadyPosted || updateTime == UpdateNow)
            sendUpdateRequest(tlw, updateTime);
        return;
    }

    // ---------------------------------------------------------------------------

    if (dirtyWidgets.isEmpty()) {
        addDirtyWidget(widget, r);
        sendUpdateRequest(tlw, updateTime);
        return;
    }

    // ---------------------------------------------------------------------------

    if (widget->d_func()->inDirtyList) {
        if (!qt_region_strictContains(widget->d_func()->dirty, effectiveWidgetRect)) {
#if QT_CONFIG(graphicseffect)
            if (widget->d_func()->graphicsEffect)
                widget->d_func()->dirty += widget->d_func()->effectiveRectFor(r);
            else
#endif
                widget->d_func()->dirty += r;
        }
    } else {
        addDirtyWidget(widget, r);
    }

    // ---------------------------------------------------------------------------

    if (updateTime == UpdateNow)
        sendUpdateRequest(tlw, updateTime);
}
template void QWidgetRepaintManager::markDirty<QRect>(const QRect &, QWidget *, UpdateTime, BufferState);
template void QWidgetRepaintManager::markDirty<QRegion>(const QRegion &, QWidget *, UpdateTime, BufferState);

void QWidgetRepaintManager::addDirtyWidget(QWidget *widget, const QRegion &rgn)
{
    if (widget && !widget->d_func()->inDirtyList && !widget->data->in_destructor) {
        QWidgetPrivate *widgetPrivate = widget->d_func();
#if QT_CONFIG(graphicseffect)
        if (widgetPrivate->graphicsEffect)
            widgetPrivate->dirty = widgetPrivate->effectiveRectFor(rgn.boundingRect());
        else
#endif // QT_CONFIG(graphicseffect)
            widgetPrivate->dirty = rgn;
        dirtyWidgets.append(widget);
        widgetPrivate->inDirtyList = true;
    }
}

void QWidgetRepaintManager::removeDirtyWidget(QWidget *w)
{
    if (!w)
        return;

    dirtyWidgets.removeAll(w);
    dirtyRenderToTextureWidgets.removeAll(w);
    resetWidget(w);

    needsFlushWidgets.removeAll(w);

    QWidgetPrivate *wd = w->d_func();
    const int n = wd->children.size();
    for (int i = 0; i < n; ++i) {
        if (QWidget *child = qobject_cast<QWidget*>(wd->children.at(i)))
            removeDirtyWidget(child);
    }
}

void QWidgetRepaintManager::resetWidget(QWidget *widget)
{
    if (widget) {
        widget->d_func()->inDirtyList = false;
        widget->d_func()->isScrolled = false;
        widget->d_func()->isMoved = false;
        widget->d_func()->dirty = QRegion();
    }
}

void QWidgetRepaintManager::addDirtyRenderToTextureWidget(QWidget *widget)
{
    if (widget && !widget->d_func()->inDirtyList && !widget->data->in_destructor) {
        QWidgetPrivate *widgetPrivate = widget->d_func();
        Q_ASSERT(widgetPrivate->renderToTexture);
        dirtyRenderToTextureWidgets.append(widget);
        widgetPrivate->inDirtyList = true;
    }
}

void QWidgetRepaintManager::sendUpdateRequest(QWidget *widget, UpdateTime updateTime)
{
    if (!widget)
        return;

    qCInfo(lcWidgetPainting) << "Sending update request to" << widget << "with" << updateTime;

    // Having every repaint() leading to a sync/flush is bad as it causes
    // compositing and waiting for vsync each and every time. Change to
    // UpdateLater, except for approx. once per frame to prevent starvation in
    // case the control does not get back to the event loop.
    QWidget *w = widget->window();
    if (updateTime == UpdateNow && w && w->windowHandle() && QWindowPrivate::get(w->windowHandle())->compositing) {
        int refresh = 60;
        QScreen *ws = w->windowHandle()->screen();
        if (ws)
            refresh = ws->refreshRate();
        QWindowPrivate *wd = QWindowPrivate::get(w->windowHandle());
        if (wd->lastComposeTime.isValid()) {
            const qint64 elapsed = wd->lastComposeTime.elapsed();
            if (elapsed <= qint64(1000.0f / refresh))
                updateTime = UpdateLater;
       }
    }

    switch (updateTime) {
    case UpdateLater:
        updateRequestSent = true;
        QCoreApplication::postEvent(widget, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
        break;
    case UpdateNow: {
        QEvent event(QEvent::UpdateRequest);
        QCoreApplication::sendEvent(widget, &event);
        break;
        }
    }
}

// ---------------------------------------------------------------------------

static bool hasPlatformWindow(QWidget *widget)
{
    return widget && widget->windowHandle() && widget->windowHandle()->handle();
}

//parent's coordinates; move whole rect; update parent and widget
//assume the screen blt has already been done, so we don't need to refresh that part
void QWidgetPrivate::moveRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    if (!q->isVisible() || (dx == 0 && dy == 0))
        return;

    QWidget *tlw = q->window();

    static const bool accelEnv = qEnvironmentVariableIntValue("QT_NO_FAST_MOVE") == 0;

    QWidget *parentWidget = q->parentWidget();
    QPoint toplevelOffset = parentWidget->mapTo(tlw, QPoint());
    QWidgetPrivate *parentPrivate = parentWidget->d_func();
    const QRect clipR(parentPrivate->clipRect());
    const QRect newRect(rect.translated(dx, dy));
    QRect destRect = rect.intersected(clipR);
    if (destRect.isValid())
        destRect = destRect.translated(dx, dy).intersected(clipR);
    const QRect sourceRect(destRect.translated(-dx, -dy));
    const QRect parentRect(rect & clipR);
    const bool nativeWithTextureChild = textureChildSeen && hasPlatformWindow(q);

    bool accelerateMove = accelEnv && isOpaque && !nativeWithTextureChild && sourceRect.isValid()
#if QT_CONFIG(graphicsview)
                          // No accelerate move for proxy widgets.
                          && !tlw->d_func()->extra->proxyWidget
#endif
                          && !isOverlapped(sourceRect) && !isOverlapped(destRect);

    if (!accelerateMove) {
        QRegion parentRegion(effectiveRectFor(parentRect));
        if (!extra || !extra->hasMask) {
            parentRegion -= newRect;
        } else {
            // invalidateBackingStore() excludes anything outside the mask
            parentRegion += newRect & clipR;
        }
        parentPrivate->invalidateBackingStore(parentRegion);
        invalidateBackingStore((newRect & clipR).translated(-data.crect.topLeft()));
    } else {
        QWidgetRepaintManager *repaintManager = QWidgetPrivate::get(tlw)->maybeRepaintManager();
        Q_ASSERT(repaintManager);
        QRegion childExpose(newRect & clipR);

        if (repaintManager->bltRect(sourceRect, dx, dy, parentWidget))
            childExpose -= destRect;

        if (!parentWidget->updatesEnabled())
            return;

        const bool childUpdatesEnabled = q->updatesEnabled();

        if (childUpdatesEnabled && !childExpose.isEmpty()) {
            childExpose.translate(-data.crect.topLeft());
            repaintManager->markDirty(childExpose, q);
            isMoved = true;
        }

        QRegion parentExpose(parentRect);
        parentExpose -= newRect;
        if (extra && extra->hasMask)
            parentExpose += QRegion(newRect) - extra->mask.translated(data.crect.topLeft());

        if (!parentExpose.isEmpty()) {
            repaintManager->markDirty(parentExpose, parentWidget);
            parentPrivate->isMoved = true;
        }

        if (childUpdatesEnabled) {
            QRegion needsFlush(sourceRect);
            needsFlush += destRect;
            repaintManager->markNeedsFlush(parentWidget, needsFlush, toplevelOffset);
        }
    }
}

//widget's coordinates; scroll within rect;  only update widget
void QWidgetPrivate::scrollRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    QWidget *tlw = q->window();

    QWidgetRepaintManager *repaintManager = QWidgetPrivate::get(tlw)->maybeRepaintManager();
    if (!repaintManager)
        return;

    static const bool accelEnv = qEnvironmentVariableIntValue("QT_NO_FAST_SCROLL") == 0;

    const QRect scrollRect = rect & clipRect();
    bool overlapped = false;
    bool accelerateScroll = accelEnv && isOpaque && !q_func()->testAttribute(Qt::WA_WState_InPaintEvent)
                            && !(overlapped = isOverlapped(scrollRect.translated(data.crect.topLeft())));

    if (!accelerateScroll) {
        if (overlapped) {
            QRegion region(scrollRect);
            subtractOpaqueSiblings(region);
            invalidateBackingStore(region);
        }else {
            invalidateBackingStore(scrollRect);
        }
    } else {
        const QPoint toplevelOffset = q->mapTo(tlw, QPoint());
        const QRect destRect = scrollRect.translated(dx, dy) & scrollRect;
        const QRect sourceRect = destRect.translated(-dx, -dy);

        QRegion childExpose(scrollRect);
        if (sourceRect.isValid()) {
            if (repaintManager->bltRect(sourceRect, dx, dy, q))
                childExpose -= destRect;
        }

        if (inDirtyList) {
            if (rect == q->rect()) {
                dirty.translate(dx, dy);
            } else {
                QRegion dirtyScrollRegion = dirty.intersected(scrollRect);
                if (!dirtyScrollRegion.isEmpty()) {
                    dirty -= dirtyScrollRegion;
                    dirtyScrollRegion.translate(dx, dy);
                    dirty += dirtyScrollRegion;
                }
            }
        }

        if (!q->updatesEnabled())
            return;

        if (!childExpose.isEmpty()) {
            repaintManager->markDirty(childExpose, q);
            isScrolled = true;
        }

        // Instead of using native scroll-on-screen, we copy from
        // backingstore, giving only one screen update for each
        // scroll, and a solid appearance
        repaintManager->markNeedsFlush(q, destRect, toplevelOffset);
    }
}

/*
    Moves the whole rect by (dx, dy) in widget's coordinate system.
    Doesn't generate any updates.
*/
bool QWidgetRepaintManager::bltRect(const QRect &rect, int dx, int dy, QWidget *widget)
{
    const QPoint pos(widget->mapTo(tlw, rect.topLeft()));
    const QRect tlwRect(QRect(pos, rect.size()));
    if (dirty.intersects(tlwRect))
        return false; // We don't want to scroll junk.
    return store->scroll(tlwRect, dx, dy);
}

// ---------------------------------------------------------------------------

static void findTextureWidgetsRecursively(QWidget *tlw, QWidget *widget,
                                          QPlatformTextureList *widgetTextures,
                                          QList<QWidget *> *nativeChildren)
{
    QWidgetPrivate *wd = QWidgetPrivate::get(widget);
    if (wd->renderToTexture) {
        QPlatformTextureList::Flags flags = wd->textureListFlags();
        const QRect rect(widget->mapTo(tlw, QPoint()), widget->size());
        QWidgetPrivate::TextureData data = wd->texture();
        widgetTextures->appendTexture(widget, data.textureLeft, data.textureRight, rect, wd->clipRect(), flags);
    }

    for (int i = 0; i < wd->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(wd->children.at(i));
        // Stop at native widgets but store them. Stop at hidden widgets too.
        if (w && !w->isWindow() && hasPlatformWindow(w))
            nativeChildren->append(w);
        if (w && !w->isWindow() && !hasPlatformWindow(w) && !w->isHidden() && QWidgetPrivate::get(w)->textureChildSeen)
            findTextureWidgetsRecursively(tlw, w, widgetTextures, nativeChildren);
    }
}

static void findAllTextureWidgetsRecursively(QWidget *tlw, QWidget *widget)
{
    // textureChildSeen does not take native child widgets into account and that's good.
    if (QWidgetPrivate::get(widget)->textureChildSeen) {
        QList<QWidget *> nativeChildren;
        auto tl = std::make_unique<QPlatformTextureList>();
        // Look for texture widgets (incl. widget itself) from 'widget' down,
        // but skip subtrees with a parent of a native child widget.
        findTextureWidgetsRecursively(tlw, widget, tl.get(), &nativeChildren);
        // tl may be empty regardless of textureChildSeen if we have native or hidden children.
        if (!tl->isEmpty())
            QWidgetPrivate::get(tlw)->topData()->widgetTextures.push_back(std::move(tl));
        // Native child widgets, if there was any, get their own separate QPlatformTextureList.
        for (QWidget *ncw : std::as_const(nativeChildren)) {
            if (QWidgetPrivate::get(ncw)->textureChildSeen)
                findAllTextureWidgetsRecursively(tlw, ncw);
        }
    }
}

static QPlatformTextureList *widgetTexturesFor(QWidget *tlw, QWidget *widget)
{
    for (const auto &tl : QWidgetPrivate::get(tlw)->topData()->widgetTextures) {
        Q_ASSERT(!tl->isEmpty());
        for (int i = 0; i < tl->count(); ++i) {
            QWidget *w = static_cast<QWidget *>(tl->source(i));
            if ((hasPlatformWindow(w) && w == widget) || (!hasPlatformWindow(w) && w->nativeParentWidget() == widget))
                return tl.get();
        }
    }

    if (QWidgetPrivate::get(widget)->textureChildSeen)
        return qt_dummy_platformTextureList();

    return nullptr;
}

// ---------------------------------------------------------------------------

/*!
    Synchronizes the \a exposedRegion of the \a exposedWidget with the backing store.

    If there are dirty widgets, including but not limited to the \a exposedWidget,
    these will be repainted first. The backingstore is then flushed to the screen,
    regardless of whether or not there were any repaints.
*/
void QWidgetRepaintManager::sync(QWidget *exposedWidget, const QRegion &exposedRegion)
{
    qCInfo(lcWidgetPainting) << "Syncing" << exposedRegion << "of" << exposedWidget;

    if (!tlw->isVisible())
        return;

    if (!exposedWidget || !hasPlatformWindow(exposedWidget)
        || !exposedWidget->isVisible() || !exposedWidget->testAttribute(Qt::WA_Mapped)
        || !exposedWidget->updatesEnabled() || exposedRegion.isEmpty()) {
        return;
    }

    // Nothing to repaint.
    if (!isDirty() && store->size().isValid()) {
        QPlatformTextureList *widgetTextures = widgetTexturesFor(tlw, exposedWidget);
        flush(exposedWidget, widgetTextures ? QRegion() : exposedRegion, widgetTextures);
        return;
    }

    // As requests to sync a specific widget typically comes from an expose event
    // we can't rely solely on our own dirty tracking to decide what to flush, and
    // need to respect the platform's request to at least flush the entire widget,
    QPoint offset = exposedWidget != tlw ? exposedWidget->mapTo(tlw, QPoint()) : QPoint();
    markNeedsFlush(exposedWidget, exposedRegion, offset);

    if (syncAllowed())
        paintAndFlush();
}

/*!
    Synchronizes the backing store, i.e. dirty areas are repainted and flushed.
*/
void QWidgetRepaintManager::sync()
{
    qCInfo(lcWidgetPainting) << "Syncing dirty widgets";

    updateRequestSent = false;
    if (qt_widget_private(tlw)->shouldDiscardSyncRequest()) {
        // If the top-level is minimized, it's not visible on the screen so we can delay the
        // update until it's shown again. In order to do that we must keep the dirty states.
        // These will be cleared when we receive the first expose after showNormal().
        // However, if the widget is not visible (isVisible() returns false), everything will
        // be invalidated once the widget is shown again, so clear all dirty states.
        if (!tlw->isVisible()) {
            dirty = QRegion();
            for (int i = 0; i < dirtyWidgets.size(); ++i)
                resetWidget(dirtyWidgets.at(i));
            dirtyWidgets.clear();
        }
        return;
    }

    if (syncAllowed())
        paintAndFlush();
}

bool QWidgetPrivate::shouldDiscardSyncRequest() const
{
    Q_Q(const QWidget);
    return !maybeTopData() || !q->testAttribute(Qt::WA_Mapped) || !q->isVisible();
}

bool QWidgetRepaintManager::syncAllowed()
{
    QTLWExtra *tlwExtra = tlw->d_func()->maybeTopData();
    if (textureListWatcher && !textureListWatcher->isLocked()) {
        textureListWatcher->deleteLater();
        textureListWatcher = nullptr;
    } else if (!tlwExtra->widgetTextures.empty()) {
        bool skipSync = false;
        for (const auto &tl : tlwExtra->widgetTextures) {
            if (tl->isLocked()) {
                if (!textureListWatcher)
                    textureListWatcher = new QPlatformTextureListWatcher(this);
                if (!textureListWatcher->isLocked())
                    textureListWatcher->watch(tl.get());
                skipSync = true;
            }
        }
        if (skipSync)  // cannot compose due to widget textures being in use
            return false;
    }
    return true;
}

static bool isDrawnInEffect(const QWidget *w)
{
#if QT_CONFIG(graphicseffect)
    do {
        if (w->graphicsEffect())
            return true;
        w = w->parentWidget();
    } while (w);
#endif
    return false;
}

void QWidgetRepaintManager::paintAndFlush()
{
    qCInfo(lcWidgetPainting) << "Painting and flushing dirty"
        << "top level" << dirty << "and dirty widgets" << dirtyWidgets;

    const bool updatesDisabled = !tlw->updatesEnabled();
    bool repaintAllWidgets = false;

    const QRect tlwRect = tlw->data->crect;
    if (!updatesDisabled && store->size() != tlwRect.size()) {
        if (hasStaticContents() && !store->size().isEmpty() ) {
            // Repaint existing dirty area and newly visible area.
            const QRect clipRect(QPoint(0, 0), store->size());
            const QRegion staticRegion(staticContents(nullptr, clipRect));
            QRegion newVisible(0, 0, tlwRect.width(), tlwRect.height());
            newVisible -= staticRegion;
            dirty += newVisible;
            store->setStaticContents(staticRegion);
        } else {
            // Repaint everything.
            dirty = QRegion(0, 0, tlwRect.width(), tlwRect.height());
            for (int i = 0; i < dirtyWidgets.size(); ++i)
                resetWidget(dirtyWidgets.at(i));
            dirtyWidgets.clear();
            repaintAllWidgets = true;
        }
    }

    if (store->size() != tlwRect.size())
        store->resize(tlwRect.size());

    if (updatesDisabled)
        return;

    // Contains everything that needs repaint.
    QRegion toClean(dirty);

    // Loop through all update() widgets and remove them from the list before they are
    // painted (in case someone calls update() in paintEvent). If the widget is opaque
    // and does not have transparent overlapping siblings, append it to the
    // opaqueNonOverlappedWidgets list and paint it directly without composition.
    QVarLengthArray<QWidget *, 32> opaqueNonOverlappedWidgets;
    for (int i = 0; i < dirtyWidgets.size(); ++i) {
        QWidget *w = dirtyWidgets.at(i);
        QWidgetPrivate *wd = w->d_func();
        if (wd->data.in_destructor)
            continue;

        // Clip with mask() and clipRect().
        wd->dirty &= wd->clipRect();
        wd->clipToEffectiveMask(wd->dirty);

        // Subtract opaque siblings and children.
        bool hasDirtySiblingsAbove = false;
        // We know for sure that the widget isn't overlapped if 'isMoved' is true.
        if (!wd->isMoved)
            wd->subtractOpaqueSiblings(wd->dirty, &hasDirtySiblingsAbove);

        // Make a copy of the widget's dirty region, to restore it in case there is an opaque
        // render-to-texture child that completely covers the widget, because otherwise the
        // render-to-texture child won't be visible, due to its parent widget not being redrawn
        // with a proper blending mask.
        const QRegion dirtyBeforeSubtractedOpaqueChildren = wd->dirty;

        // Scrolled and moved widgets must draw all children.
        if (!wd->isScrolled && !wd->isMoved)
            wd->subtractOpaqueChildren(wd->dirty, w->rect());

        if (wd->dirty.isEmpty() && wd->textureChildSeen)
            wd->dirty = dirtyBeforeSubtractedOpaqueChildren;

        if (wd->dirty.isEmpty()) {
            resetWidget(w);
            continue;
        }

        const QRegion widgetDirty(w != tlw ? wd->dirty.translated(w->mapTo(tlw, QPoint()))
                                           : wd->dirty);
        toClean += widgetDirty;

#if QT_CONFIG(graphicsview)
        if (tlw->d_func()->extra->proxyWidget) {
            resetWidget(w);
            continue;
        }
#endif

        if (!isDrawnInEffect(w) && !hasDirtySiblingsAbove && wd->isOpaque
                && !dirty.intersects(widgetDirty.boundingRect())) {
            opaqueNonOverlappedWidgets.append(w);
        } else {
            resetWidget(w);
            dirty += widgetDirty;
        }
    }
    dirtyWidgets.clear();

    // Find all render-to-texture child widgets (including self).
    // The search is cut at native widget boundaries, meaning that each native child widget
    // has its own list for the subtree below it.
    QTLWExtra *tlwExtra = tlw->d_func()->topData();
    tlwExtra->widgetTextures.clear();
    findAllTextureWidgetsRecursively(tlw, tlw);
    qt_window_private(tlw->windowHandle())->compositing = false; // will get updated in flush()

    if (toClean.isEmpty()) {
        // Nothing to repaint. However renderToTexture widgets are handled
        // specially, they are not in the regular dirty list, in order to
        // prevent triggering unnecessary backingstore painting when only the
        // texture content changes. Check if we have such widgets in the special
        // dirty list.
        QVarLengthArray<QWidget *, 16> paintPending;
        const int numPaintPending = dirtyRenderToTextureWidgets.size();
        paintPending.reserve(numPaintPending);
        for (int i = 0; i < numPaintPending; ++i) {
            QWidget *w = dirtyRenderToTextureWidgets.at(i);
            paintPending << w;
            resetWidget(w);
        }
        dirtyRenderToTextureWidgets.clear();
        for (int i = 0; i < numPaintPending; ++i) {
            QWidget *w = paintPending[i];
            w->d_func()->sendPaintEvent(w->rect());
            if (w != tlw) {
                QWidget *npw = w->nativeParentWidget();
                if (hasPlatformWindow(w) || (npw && npw != tlw)) {
                    if (!hasPlatformWindow(w))
                        w = npw;
                    markNeedsFlush(w);
                }
            }
        }

        // We might have newly exposed areas on the screen if this function was
        // called from sync(QWidget *, QRegion)), so we have to make sure those
        // are flushed. We also need to composite the renderToTexture widgets.
        flush();

        return;
    }

    for (const auto &tl : tlwExtra->widgetTextures) {
        for (int i = 0; i < tl->count(); ++i) {
            QWidget *w = static_cast<QWidget *>(tl->source(i));
            if (dirtyRenderToTextureWidgets.contains(w)) {
                const QRect rect = tl->geometry(i); // mapped to the tlw already
                // Set a flag to indicate that the paint event for this
                // render-to-texture widget must not to be optimized away.
                w->d_func()->renderToTextureReallyDirty = 1;
                dirty += rect;
                toClean += rect;
            }
        }
    }
    for (int i = 0; i < dirtyRenderToTextureWidgets.size(); ++i)
        resetWidget(dirtyRenderToTextureWidgets.at(i));
    dirtyRenderToTextureWidgets.clear();

#if QT_CONFIG(graphicsview)
    if (tlw->d_func()->extra->proxyWidget) {
        updateStaticContentsSize();
        dirty = QRegion();
        updateRequestSent = false;
        for (const QRect &rect : toClean)
            tlw->d_func()->extra->proxyWidget->update(rect);
        return;
    }
#endif

    store->beginPaint(toClean);

    // Must do this before sending any paint events because
    // the size may change in the paint event.
    updateStaticContentsSize();
    const QRegion dirtyCopy(dirty);
    dirty = QRegion();
    updateRequestSent = false;

    // Paint opaque non overlapped widgets.
    for (int i = 0; i < opaqueNonOverlappedWidgets.size(); ++i) {
        QWidget *w = opaqueNonOverlappedWidgets[i];
        QWidgetPrivate *wd = w->d_func();

        QWidgetPrivate::DrawWidgetFlags flags = QWidgetPrivate::DrawRecursive;
        // Scrolled and moved widgets must draw all children.
        if (!wd->isScrolled && !wd->isMoved)
            flags |= QWidgetPrivate::DontDrawOpaqueChildren;
        if (w == tlw)
            flags |= QWidgetPrivate::DrawAsRoot;

        QRegion toBePainted(wd->dirty);
        resetWidget(w);

        QPoint offset;
        if (w != tlw)
            offset += w->mapTo(tlw, QPoint());
        wd->drawWidget(store->paintDevice(), toBePainted, offset, flags, nullptr, this);
    }

    // Paint the rest with composition.
    if (repaintAllWidgets || !dirtyCopy.isEmpty()) {
        QWidgetPrivate::DrawWidgetFlags flags = QWidgetPrivate::DrawAsRoot | QWidgetPrivate::DrawRecursive
                | QWidgetPrivate::UseEffectRegionBounds;
        tlw->d_func()->drawWidget(store->paintDevice(), dirtyCopy, QPoint(), flags, nullptr, this);
    }

    store->endPaint();

    flush();
}

/*!
    Marks the \a region of the \a widget as needing a flush. The \a region will be copied from
    the backing store to the \a widget's native parent next time flush() is called.

    Paint on screen widgets are ignored.
*/
void QWidgetRepaintManager::markNeedsFlush(QWidget *widget, const QRegion &region, const QPoint &topLevelOffset)
{
    if (!widget || widget->d_func()->shouldPaintOnScreen() || region.isEmpty())
        return;

    if (widget == tlw) {
        // Top-level (native)
        qCInfo(lcWidgetPainting) << "Marking" << region << "of top level"
            << widget << "as needing flush";
        topLevelNeedsFlush += region;
    } else if (!hasPlatformWindow(widget) && !widget->isWindow()) {
        QWidget *nativeParent = widget->nativeParentWidget();
        qCInfo(lcWidgetPainting) << "Marking" << region << "of"
                << widget << "as needing flush in" << nativeParent
                << "at offset" << topLevelOffset;
        if (nativeParent == tlw) {
            // Alien widgets with the top-level as the native parent (common case)
            topLevelNeedsFlush += region.translated(topLevelOffset);
        } else {
            // Alien widgets with native parent != tlw
            const QPoint nativeParentOffset = widget->mapTo(nativeParent, QPoint());
            markNeedsFlush(nativeParent, region.translated(nativeParentOffset));
        }
    } else {
        // Native child widgets
        qCInfo(lcWidgetPainting) << "Marking" << region
            << "of native child" << widget << "as needing flush";
        markNeedsFlush(widget, region);
    }
}

void QWidgetRepaintManager::markNeedsFlush(QWidget *widget, const QRegion &region)
{
    if (!widget)
        return;

    auto *widgetPrivate = qt_widget_private(widget);
    if (!widgetPrivate->needsFlush)
        widgetPrivate->needsFlush = new QRegion;

    *widgetPrivate->needsFlush += region;

    if (!needsFlushWidgets.contains(widget))
        needsFlushWidgets.append(widget);
}

/*!
    Flushes the contents of the backing store into the top-level widget.
*/
void QWidgetRepaintManager::flush()
{
    qCInfo(lcWidgetPainting) << "Flushing top level"
        << topLevelNeedsFlush << "and children" << needsFlushWidgets;

    const bool hasNeedsFlushWidgets = !needsFlushWidgets.isEmpty();
    bool flushed = false;

    // Flush the top level widget
    if (!topLevelNeedsFlush.isEmpty()) {
        flush(tlw, topLevelNeedsFlush, widgetTexturesFor(tlw, tlw));
        topLevelNeedsFlush = QRegion();
        flushed = true;
    }

    // Render-to-texture widgets are not in topLevelNeedsFlush so flush if we have not done it above.
    if (!flushed && !hasNeedsFlushWidgets) {
        if (!tlw->d_func()->topData()->widgetTextures.empty()) {
            if (QPlatformTextureList *widgetTextures = widgetTexturesFor(tlw, tlw))
                flush(tlw, QRegion(), widgetTextures);
        }
    }

    if (!hasNeedsFlushWidgets)
        return;

    for (QWidget *w : std::exchange(needsFlushWidgets, {})) {
        QWidgetPrivate *wd = w->d_func();
        Q_ASSERT(wd->needsFlush);
        QPlatformTextureList *widgetTexturesForNative = wd->textureChildSeen ? widgetTexturesFor(tlw, w) : nullptr;
        flush(w, *wd->needsFlush, widgetTexturesForNative);
        *wd->needsFlush = QRegion();
    }
}

/*
    Flushes the contents of the backingstore into the screen area of \a widget.

    \a region is the region to be updated in \a widget coordinates.
 */
void QWidgetRepaintManager::flush(QWidget *widget, const QRegion &region, QPlatformTextureList *widgetTextures)
{
    Q_ASSERT(!region.isEmpty() || widgetTextures);
    Q_ASSERT(widget);
    Q_ASSERT(tlw);

    if (tlw->testAttribute(Qt::WA_DontShowOnScreen) || widget->testAttribute(Qt::WA_DontShowOnScreen))
        return;

    // Foreign Windows do not have backing store content and must not be flushed
    if (QWindow *widgetWindow = widget->windowHandle()) {
        if (widgetWindow->type() == Qt::ForeignWindow)
            return;
    }

    static bool fpsDebug = qEnvironmentVariableIntValue("QT_DEBUG_FPS");
    if (fpsDebug) {
        if (!perfFrames++)
            perfTime.start();
        if (perfTime.elapsed() > 5000) {
            double fps = double(perfFrames * 1000) / perfTime.restart();
            qDebug("FPS: %.1f\n", fps);
            perfFrames = 0;
        }
    }

    QPoint offset;
    if (widget != tlw)
        offset += widget->mapTo(tlw, QPoint());

    // Use a condition that tries to keep both QTBUG-108344 and QTBUG-113557
    // happy, i.e. support both (A) "native rhi-based child in a rhi-based
    // toplevel" and (B) "native raster child in a rhi-based toplevel".
    //
    // If the tlw and the backingstore are RHI-based, then there are two cases
    // to consider:
    //
    // (1) widget is not a native child, i.e. the QWindow for widget and tlw are
    // the same,
    //
    // (2) widget is a native child which we now attempt to flush with tlw's
    // backingstore to widget's native window. This is the interesting one.
    //
    // Using the condition tlw->usesRhiFlush on its own is insufficient since
    // it fails to capture the case of a raster-based native child widget
    // within tlw. (which must hit the non-rhi flush path)
    //
    // Extending the condition with tlw->windowHandle() == widget->windowHandle()
    // would be logical but wrong, when it comes to (A) since flushing a
    // RHI-based native child with a given 3D API using a RHI-based
    // tlw/backingstore with the same 3D API needs to be supported still. (this
    // happens when e.g. someone calls winId() on a QOpenGLWidget)
    //
    // Different 3D APIs do not need to be supported since we do not allow to
    // do things like having a QQuickWidget with Vulkan and a QOpenGLWidget in
    // the same toplevel, regardless of the widgets being native children or
    // not. Hence comparing the surfaceType() instead. This satisfies both (A)
    // and (B) given that an RHI-based toplevel cannot be RasterSurface.
    //
    if (tlw->d_func()->usesRhiFlush && tlw->windowHandle()->surfaceType() == widget->windowHandle()->surfaceType()) {
        QRhi *rhi = store->handle()->rhi();
        qCDebug(lcWidgetPainting) << "Flushing" << region << "of" << widget
                                  << "with QRhi" << rhi
                                  << "to window" << widget->windowHandle();
        if (!widgetTextures)
            widgetTextures = qt_dummy_platformTextureList;

        qt_window_private(tlw->windowHandle())->compositing = true;
        QWidgetPrivate *widgetWindowPrivate = widget->window()->d_func();
        widgetWindowPrivate->sendComposeStatus(widget->window(), false);
        // A window may have alpha even when the app did not request
        // WA_TranslucentBackground. Therefore the compositor needs to know whether the app intends
        // to rely on translucency, in order to decide if it should clear to transparent or opaque.
        const bool translucentBackground = widget->testAttribute(Qt::WA_TranslucentBackground);

        QPlatformBackingStore::FlushResult flushResult;
        flushResult = store->handle()->rhiFlush(widget->windowHandle(),
                                                widget->devicePixelRatio(),
                                                region,
                                                offset,
                                                widgetTextures,
                                                translucentBackground);
        widgetWindowPrivate->sendComposeStatus(widget->window(), true);
        if (flushResult == QPlatformBackingStore::FlushFailedDueToLostDevice) {
            qSendWindowChangeToTextureChildrenRecursively(widget->window(),
                                                          QEvent::WindowAboutToChangeInternal);
            store->handle()->graphicsDeviceReportedLost();
            qSendWindowChangeToTextureChildrenRecursively(widget->window(),
                                                          QEvent::WindowChangeInternal);
            widget->update();
        }
    } else {
        qCInfo(lcWidgetPainting) << "Flushing" << region << "of" << widget;
        store->flush(region, widget->windowHandle(), offset);
    }
}

// ---------------------------------------------------------------------------

void QWidgetRepaintManager::addStaticWidget(QWidget *widget)
{
    if (!widget)
        return;

    Q_ASSERT(widget->testAttribute(Qt::WA_StaticContents));
    if (!staticWidgets.contains(widget))
        staticWidgets.append(widget);
}

// Move the reparented widget and all its static children from this backing store
// to the new backing store if reparented into another top-level / backing store.
void QWidgetRepaintManager::moveStaticWidgets(QWidget *reparented)
{
    Q_ASSERT(reparented);
    QWidgetRepaintManager *newPaintManager = reparented->d_func()->maybeRepaintManager();
    if (newPaintManager == this)
        return;

    int i = 0;
    while (i < staticWidgets.size()) {
        QWidget *w = staticWidgets.at(i);
        if (reparented == w || reparented->isAncestorOf(w)) {
            staticWidgets.removeAt(i);
            if (newPaintManager)
                newPaintManager->addStaticWidget(w);
        } else {
            ++i;
        }
    }
}

void QWidgetRepaintManager::removeStaticWidget(QWidget *widget)
{
    staticWidgets.removeAll(widget);
}

bool QWidgetRepaintManager::hasStaticContents() const
{
#if defined(Q_OS_WIN)
    return !staticWidgets.isEmpty();
#else
    return !staticWidgets.isEmpty() && false;
#endif
}

/*!
    Returns the static content inside the \a parent if non-zero; otherwise the static content
    for the entire backing store is returned. The content will be clipped to \a withinClipRect
    if non-empty.
*/
QRegion QWidgetRepaintManager::staticContents(QWidget *parent, const QRect &withinClipRect) const
{
    if (!parent && tlw->testAttribute(Qt::WA_StaticContents)) {
        QRect backingstoreRect(QPoint(0, 0), store->size());
        if (!withinClipRect.isEmpty())
            backingstoreRect &= withinClipRect;
        return QRegion(backingstoreRect);
    }

    QRegion region;
    if (parent && parent->d_func()->children.isEmpty())
        return region;

    const bool clipToRect = !withinClipRect.isEmpty();
    const int count = staticWidgets.size();
    for (int i = 0; i < count; ++i) {
        QWidget *w = staticWidgets.at(i);
        QWidgetPrivate *wd = w->d_func();
        if (!wd->isOpaque || !wd->extra || wd->extra->staticContentsSize.isEmpty()
            || !w->isVisible() || (parent && !parent->isAncestorOf(w))) {
            continue;
        }

        QRect rect(0, 0, wd->extra->staticContentsSize.width(), wd->extra->staticContentsSize.height());
        const QPoint offset = w->mapTo(parent ? parent : tlw, QPoint());
        if (clipToRect)
            rect &= withinClipRect.translated(-offset);
        if (rect.isEmpty())
            continue;

        rect &= wd->clipRect();
        if (rect.isEmpty())
            continue;

        QRegion visible(rect);
        wd->clipToEffectiveMask(visible);
        if (visible.isEmpty())
            continue;
        wd->subtractOpaqueSiblings(visible, nullptr, /*alsoNonOpaque=*/true);

        visible.translate(offset);
        region += visible;
    }

    return region;
}

void QWidgetRepaintManager::updateStaticContentsSize()
{
    for (int i = 0; i < staticWidgets.size(); ++i) {
        QWidgetPrivate *wd = staticWidgets.at(i)->d_func();
        if (!wd->extra)
            wd->createExtra();
        wd->extra->staticContentsSize = wd->data.crect.size();
    }
}

// ---------------------------------------------------------------------------

bool QWidgetRepaintManager::isDirty() const
{
    return !(dirtyWidgets.isEmpty() && dirty.isEmpty() && dirtyRenderToTextureWidgets.isEmpty());
}

/*!
    Invalidates the backing store when the widget is resized.
    Static areas are never invalidated unless absolutely needed.
*/
void QWidgetPrivate::invalidateBackingStore_resizeHelper(const QPoint &oldPos, const QSize &oldSize)
{
    Q_Q(QWidget);
    Q_ASSERT(!q->isWindow());
    Q_ASSERT(q->parentWidget());

    const bool staticContents = q->testAttribute(Qt::WA_StaticContents);
    const bool sizeDecreased = (data.crect.width() < oldSize.width())
                               || (data.crect.height() < oldSize.height());

    const QPoint offset(data.crect.x() - oldPos.x(), data.crect.y() - oldPos.y());
    const bool parentAreaExposed = !offset.isNull() || sizeDecreased;
    const QRect newWidgetRect(q->rect());
    const QRect oldWidgetRect(0, 0, oldSize.width(), oldSize.height());

    if (!staticContents || graphicsEffect) {
        QRegion staticChildren;
        QWidgetRepaintManager *bs = nullptr;
        if (offset.isNull() && (bs = maybeRepaintManager()))
            staticChildren = bs->staticContents(q, oldWidgetRect);
        const bool hasStaticChildren = !staticChildren.isEmpty();

        if (hasStaticChildren) {
            QRegion dirty(newWidgetRect);
            dirty -= staticChildren;
            invalidateBackingStore(dirty);
        } else {
            // Entire widget needs repaint.
            invalidateBackingStore(newWidgetRect);
        }

        if (!parentAreaExposed)
            return;

        // Invalidate newly exposed area of the parent.
        if (!graphicsEffect && extra && extra->hasMask) {
            QRegion parentExpose(extra->mask.translated(oldPos));
            parentExpose &= QRect(oldPos, oldSize);
            if (hasStaticChildren)
                parentExpose -= data.crect; // Offset is unchanged, safe to do this.
            q->parentWidget()->d_func()->invalidateBackingStore(parentExpose);
        } else {
            if (hasStaticChildren && !graphicsEffect) {
                QRegion parentExpose(QRect(oldPos, oldSize));
                parentExpose -= data.crect; // Offset is unchanged, safe to do this.
                q->parentWidget()->d_func()->invalidateBackingStore(parentExpose);
            } else {
                q->parentWidget()->d_func()->invalidateBackingStore(effectiveRectFor(QRect(oldPos, oldSize)));
            }
        }
        return;
    }

    // Move static content to its new position.
    if (!offset.isNull()) {
        if (sizeDecreased) {
            const QSize minSize(qMin(oldSize.width(), data.crect.width()),
                                qMin(oldSize.height(), data.crect.height()));
            moveRect(QRect(oldPos, minSize), offset.x(), offset.y());
        } else {
            moveRect(QRect(oldPos, oldSize), offset.x(), offset.y());
        }
    }

    // Invalidate newly visible area of the widget.
    if (!sizeDecreased || !oldWidgetRect.contains(newWidgetRect)) {
        QRegion newVisible(newWidgetRect);
        newVisible -= oldWidgetRect;
        invalidateBackingStore(newVisible);
    }

    if (!parentAreaExposed)
        return;

    // Invalidate newly exposed area of the parent.
    const QRect oldRect(oldPos, oldSize);
    if (extra && extra->hasMask) {
        QRegion parentExpose(oldRect);
        parentExpose &= extra->mask.translated(oldPos);
        parentExpose -= (extra->mask.translated(data.crect.topLeft()) & data.crect);
        q->parentWidget()->d_func()->invalidateBackingStore(parentExpose);
    } else {
        QRegion parentExpose(oldRect);
        parentExpose -= data.crect;
        q->parentWidget()->d_func()->invalidateBackingStore(parentExpose);
    }
}

QRhi *QWidgetRepaintManager::rhi() const
{
    return store->handle()->rhi();
}

QT_END_NAMESPACE

#include "qwidgetrepaintmanager.moc"
#include "moc_qwidgetrepaintmanager_p.cpp"
