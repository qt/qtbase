/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QWIDGETREPAINTMANAGER_P_H
#define QWIDGETREPAINTMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QDebug>
#include <QtWidgets/qwidget.h>
#include <private/qwidget_p.h>
#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

class QPlatformTextureList;
class QPlatformTextureListWatcher;
class QWidgetRepaintManager;

#ifndef QT_NO_OPENGL
class QPlatformTextureListWatcher : public QObject
{
    Q_OBJECT

public:
    QPlatformTextureListWatcher(QWidgetRepaintManager *repaintManager);
    void watch(QPlatformTextureList *textureList);
    bool isLocked() const;

private slots:
     void onLockStatusChanged(bool locked);

private:
     QHash<QPlatformTextureList *, bool> m_locked;
     QWidgetRepaintManager *m_repaintManager;
};
#endif

class Q_AUTOTEST_EXPORT QWidgetRepaintManager
{
public:
    enum UpdateTime {
        UpdateNow,
        UpdateLater
    };

    enum BufferState{
        BufferValid,
        BufferInvalid
    };

    QWidgetRepaintManager(QWidget *t);
    ~QWidgetRepaintManager();

    void sync(QWidget *exposedWidget, const QRegion &exposedRegion);
    void sync();

    QBackingStore *backingStore() const { return store; }
    void setBackingStore(QBackingStore *backingStore) { store = backingStore; }

    template <class T>
    void markDirty(const T &r, QWidget *widget, UpdateTime updateTime = UpdateLater,
                   BufferState bufferState = BufferValid);

    void removeDirtyWidget(QWidget *w);

    inline void addStaticWidget(QWidget *widget)
    {
        if (!widget)
            return;

        Q_ASSERT(widget->testAttribute(Qt::WA_StaticContents));
        if (!staticWidgets.contains(widget))
            staticWidgets.append(widget);
    }

    // Move the reparented widget and all its static children from this backing store
    // to the new backing store if reparented into another top-level / backing store.
    inline void moveStaticWidgets(QWidget *reparented)
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

    inline void removeStaticWidget(QWidget *widget)
    {
        staticWidgets.removeAll(widget);
    }

    QRegion staticContents(QWidget *widget = nullptr, const QRect &withinClipRect = QRect()) const;

    void markDirtyOnScreen(const QRegion &dirtyOnScreen, QWidget *widget, const QPoint &topLevelOffset);
    bool bltRect(const QRect &rect, int dx, int dy, QWidget *widget);

private:
    QWidget *tlw;
    QRegion dirtyOnScreen; // needsFlush
    QRegion dirty; // needsRepaint
    QRegion dirtyFromPreviousSync;
    QVector<QWidget *> dirtyWidgets;
    QVector<QWidget *> dirtyRenderToTextureWidgets;
    QVector<QWidget *> dirtyOnScreenWidgets;
    QList<QWidget *> staticWidgets;
    QBackingStore *store;
    uint updateRequestSent : 1;

    QPlatformTextureListWatcher *textureListWatcher;
    QElapsedTimer perfTime;
    int perfFrames;

    void sendUpdateRequest(QWidget *widget, UpdateTime updateTime);

    inline bool isDirty() const
    {
        return !(dirtyWidgets.isEmpty() && dirty.isEmpty() && dirtyRenderToTextureWidgets.isEmpty());
    }

    void paintAndFlush();

    void flush(QWidget *widget = nullptr);
    void flush(QWidget *widget, const QRegion &region, QPlatformTextureList *widgetTextures);

    QRegion dirtyRegion(QWidget *widget = nullptr) const;

    void updateLists(QWidget *widget);

    bool syncAllowed();

    inline void addDirtyWidget(QWidget *widget, const QRegion &rgn)
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

    inline void addDirtyRenderToTextureWidget(QWidget *widget)
    {
        if (widget && !widget->d_func()->inDirtyList && !widget->data->in_destructor) {
            QWidgetPrivate *widgetPrivate = widget->d_func();
            Q_ASSERT(widgetPrivate->renderToTexture);
            dirtyRenderToTextureWidgets.append(widget);
            widgetPrivate->inDirtyList = true;
        }
    }

    inline QRect topLevelRect() const
    {
        return tlw->data->crect;
    }

    inline void appendDirtyOnScreenWidget(QWidget *widget)
    {
        if (!widget)
            return;

        if (!dirtyOnScreenWidgets.contains(widget))
            dirtyOnScreenWidgets.append(widget);
    }

    inline void resetWidget(QWidget *widget)
    {
        if (widget) {
            widget->d_func()->inDirtyList = false;
            widget->d_func()->isScrolled = false;
            widget->d_func()->isMoved = false;
            widget->d_func()->dirty = QRegion();
        }
    }

    inline void updateStaticContentsSize()
    {
        for (int i = 0; i < staticWidgets.size(); ++i) {
            QWidgetPrivate *wd = staticWidgets.at(i)->d_func();
            if (!wd->extra)
                wd->createExtra();
            wd->extra->staticContentsSize = wd->data.crect.size();
        }
    }

    inline bool hasStaticContents() const
    {
#if defined(Q_OS_WIN)
        return !staticWidgets.isEmpty();
#else
        return !staticWidgets.isEmpty() && false;
#endif
    }

    Q_DISABLE_COPY_MOVE(QWidgetRepaintManager)
};

QT_END_NAMESPACE

#endif // QWIDGETREPAINTMANAGER_P_H
