/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QDND_P_H
#define QDND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qobject.h"
#include "QtCore/qmap.h"
#include "QtCore/qmimedata.h"
#include "QtGui/qdrag.h"
#include "QtGui/qpixmap.h"
#include "QtGui/qcursor.h"
#include "QtGui/qwindow.h"
#include "QtCore/qpoint.h"
#include "private/qobject_p.h"
#include "QtGui/qbackingstore.h"
QT_BEGIN_NAMESPACE

class QEventLoop;
class QMouseEvent;
class QPlatformDrag;

#ifndef QT_NO_DRAGANDDROP

class Q_GUI_EXPORT QInternalMimeData : public QMimeData
{
    Q_OBJECT
public:
    QInternalMimeData();
    ~QInternalMimeData();

    bool hasFormat(const QString &mimeType) const;
    QStringList formats() const;
    static bool canReadData(const QString &mimeType);


    static QStringList formatsHelper(const QMimeData *data);
    static bool hasFormatHelper(const QString &mimeType, const QMimeData *data);
    static QByteArray renderDataHelper(const QString &mimeType, const QMimeData *data);

protected:
    QVariant retrieveData(const QString &mimeType, QVariant::Type type) const;

    virtual bool hasFormat_sys(const QString &mimeType) const = 0;
    virtual QStringList formats_sys() const = 0;
    virtual QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const = 0;
};

class QDragPrivate : public QObjectPrivate
{
public:
    QObject *source;
    QObject *target;
    QMimeData *data;
    QPixmap pixmap;
    QPoint hotspot;
    Qt::DropActions possible_actions;
    Qt::DropAction executed_action;
    QMap<Qt::DropAction, QPixmap> customCursors;
    Qt::DropAction defaultDropAction;
};

class QShapedPixmapWindow : public QWindow
{
public:
    QShapedPixmapWindow();

    void exposeEvent(QExposeEvent *)
    {
        render();
    }

    void render();

    QBackingStore *backingStore;
    QPixmap pixmap;
    QPoint hotSpot;
};


class Q_GUI_EXPORT QDragManager : public QObject {
    Q_OBJECT

    // only friend classes can use QDragManager.
    friend class QDrag;
    friend class QDragMoveEvent;
    friend class QDropEvent;
    friend class QApplication;

    bool eventFilter(QObject *, QEvent *);

public:
    QDragManager();
    ~QDragManager();
    static QDragManager *self();

    virtual Qt::DropAction drag(QDrag *);

    virtual void cancel(bool deleteSource = true);
    virtual void move(const QMouseEvent *me);
    virtual void drop(const QMouseEvent *me);

    void updatePixmap();
    void updateCursor();

    Qt::DropAction defaultAction(Qt::DropActions possibleActions,
                                 Qt::KeyboardModifiers modifiers) const;

    QPixmap dragCursor(Qt::DropAction action) const;

    QDragPrivate *dragPrivate() const { return object ? object->d_func() : 0; }

    inline QMimeData *dropData()
    { return object ? dragPrivate()->data : platformDropData; }

    void emitActionChanged(Qt::DropAction newAction) { if (object) emit object->actionChanged(newAction); }

    void setCurrentTarget(QObject *target, bool dropped = false);
    QObject *currentTarget();

    QDrag *object;

    bool beingCancelled;
    bool restoreCursor;
    bool willDrop;
    QEventLoop *eventLoop;

    Qt::DropActions possible_actions;
    // Shift/Ctrl handling, and final drop status
    Qt::DropAction global_accepted_action;

    QShapedPixmapWindow *shapedPixmapWindow;

    void unmanageEvents();
    void stopDrag();

private:
    QMimeData *platformDropData;

    Qt::DropAction currentActionForOverrideCursor;
    QObject *currentDropTarget;

    QPlatformDrag *platformDrag;

    static QDragManager *instance;
    Q_DISABLE_COPY(QDragManager)
};


#endif // !QT_NO_DRAGANDDROP


QT_END_NAMESPACE

#endif // QDND_P_H
