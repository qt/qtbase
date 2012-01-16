/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "qplatformdefs.h"

#include "qbitmap.h"
#include "qdrag.h"
#include "qpixmap.h"
#include "qevent.h"
#include "qfile.h"
#include "qtextcodec.h"
#include "qguiapplication.h"
#include "qpoint.h"
#include "qbuffer.h"
#include "qimage.h"
#include "qpainter.h"
#include "qregexp.h"
#include "qdir.h"
#include "qdnd_p.h"
#include "qimagereader.h"
#include "qimagewriter.h"
#include "qdebug.h"
#include <ctype.h>
#include <qplatformdrag_qpa.h>

#include <private/qguiapplication_p.h>

#ifndef QT_NO_DRAGANDDROP

QT_BEGIN_NAMESPACE

//#define QDND_DEBUG

#ifdef QDND_DEBUG
QString dragActionsToString(Qt::DropActions actions)
{
    QString str;
    if (actions == Qt::IgnoreAction) {
        if (!str.isEmpty())
            str += " | ";
        str += "IgnoreAction";
    }
    if (actions & Qt::LinkAction) {
        if (!str.isEmpty())
            str += " | ";
        str += "LinkAction";
    }
    if (actions & Qt::CopyAction) {
        if (!str.isEmpty())
            str += " | ";
        str += "CopyAction";
    }
    if (actions & Qt::MoveAction) {
        if (!str.isEmpty())
            str += " | ";
        str += "MoveAction";
    }
    if ((actions & Qt::TargetMoveAction) == Qt::TargetMoveAction ) {
        if (!str.isEmpty())
            str += " | ";
        str += "TargetMoveAction";
    }
    return str;
}

QString KeyboardModifiersToString(Qt::KeyboardModifiers moderfies)
{
    QString str;
    if (moderfies & Qt::ControlModifier) {
        if (!str.isEmpty())
            str += " | ";
        str += Qt::ControlModifier;
    }
    if (moderfies & Qt::AltModifier) {
        if (!str.isEmpty())
            str += " | ";
        str += Qt::AltModifier;
    }
    if (moderfies & Qt::ShiftModifier) {
        if (!str.isEmpty())
            str += " | ";
        str += Qt::ShiftModifier;
    }
    return str;
}
#endif

// the universe's only drag manager
QDragManager *QDragManager::instance = 0;


QDragManager::QDragManager()
    : QObject(qApp)
{
    Q_ASSERT(!instance);

    object = 0;
    beingCancelled = false;
    restoreCursor = false;
    willDrop = false;
    eventLoop = 0;
    currentDropTarget = 0;
    shapedPixmapWindow = 0;

    possible_actions = Qt::IgnoreAction;

    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    platformDrag = pi->drag();

    platformDropData = 0;
    if (platformDrag)
        platformDropData = platformDrag->platformDropData();
}


QDragManager::~QDragManager()
{
#ifndef QT_NO_CURSOR
    if (restoreCursor)
        QGuiApplication::restoreOverrideCursor();
#endif
    instance = 0;
}

QDragManager *QDragManager::self()
{
    if (!instance && !QGuiApplication::closingDown())
        instance = new QDragManager;
    return instance;
}

QPixmap QDragManager::dragCursor(Qt::DropAction action) const
{
    typedef QMap<Qt::DropAction, QPixmap>::const_iterator Iterator;

    if (const QDragPrivate *d = dragPrivate()) {
        const Iterator it = d->customCursors.constFind(action);
        if (it != d->customCursors.constEnd())
            return it.value();
    }

    Qt::CursorShape shape = Qt::ForbiddenCursor;
    switch (action) {
    case Qt::MoveAction:
        shape = Qt::DragMoveCursor;
        break;
    case Qt::CopyAction:
        shape = Qt::DragCopyCursor;
        break;
    case Qt::LinkAction:
        shape = Qt::DragLinkCursor;
        break;
    default:
        shape = Qt::ForbiddenCursor;
    }
    return QGuiApplicationPrivate::instance()->getPixmapCursor(shape);
}

Qt::DropAction QDragManager::defaultAction(Qt::DropActions possibleActions,
                                           Qt::KeyboardModifiers modifiers) const
{
#ifdef QDND_DEBUG
    qDebug("QDragManager::defaultAction(Qt::DropActions possibleActions)");
    qDebug("keyboard modifiers : %s", KeyboardModifiersToString(modifiers).latin1());
#endif

    QDragPrivate *d = dragPrivate();
    Qt::DropAction defaultAction = d ? d->defaultDropAction : Qt::IgnoreAction;

    if (defaultAction == Qt::IgnoreAction) {
        //This means that the drag was initiated by QDrag::start and we need to
        //preserve the old behavior
        defaultAction = Qt::CopyAction;
    }

    if (modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier)
        defaultAction = Qt::LinkAction;
    else if (modifiers & Qt::ControlModifier)
        defaultAction = Qt::CopyAction;
    else if (modifiers & Qt::ShiftModifier)
        defaultAction = Qt::MoveAction;
    else if (modifiers & Qt::AltModifier)
        defaultAction = Qt::LinkAction;

#ifdef QDND_DEBUG
    qDebug("possible actions : %s", dragActionsToString(possibleActions).latin1());
#endif

    // Check if the action determined is allowed
    if (!(possibleActions & defaultAction)) {
        if (possibleActions & Qt::CopyAction)
            defaultAction = Qt::CopyAction;
        else if (possibleActions & Qt::MoveAction)
            defaultAction = Qt::MoveAction;
        else if (possibleActions & Qt::LinkAction)
            defaultAction = Qt::LinkAction;
        else
            defaultAction = Qt::IgnoreAction;
    }

#ifdef QDND_DEBUG
    qDebug("default action : %s", dragActionsToString(defaultAction).latin1());
#endif

    return defaultAction;
}

void QDragManager::setCurrentTarget(QObject *target, bool dropped)
{
    if (currentDropTarget == target)
        return;

    currentDropTarget = target;
    if (!dropped && object) {
        object->d_func()->target = target;
        emit object->targetChanged(target);
    }

}

QObject *QDragManager::currentTarget()
{
    return currentDropTarget;
}


static const int default_pm_hotx = -2;
static const int default_pm_hoty = -16;
static const char *const default_pm[] = {
"13 9 3 1",
".      c None",
"       c #000000",
"X      c #FFFFFF",
"X X X X X X X",
" X X X X X X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X X X X X X ",
"X X X X X X X",
};


QShapedPixmapWindow::QShapedPixmapWindow()
    : QWindow()
{
    setSurfaceType(RasterSurface);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                   Qt::X11BypassWindowManagerHint | Qt::WindowTransparentForInput);
    create();
    backingStore = new QBackingStore(this);
}

void QShapedPixmapWindow::render()
{
    QRect rect(QPoint(), geometry().size());
    backingStore->resize(rect.size());

    backingStore->beginPaint(rect);

    QPaintDevice *device = backingStore->paintDevice();

    {
        QPainter p(device);
        p.drawPixmap(0, 0, pixmap);
    }

    backingStore->endPaint();
    backingStore->flush(rect);
}




static Qt::KeyboardModifiers oldstate;

void QDragManager::updatePixmap()
{
    if (shapedPixmapWindow) {
        shapedPixmapWindow->pixmap = QPixmap();
        shapedPixmapWindow->hotSpot = QPoint(default_pm_hotx,default_pm_hoty);
        if (object) {
            shapedPixmapWindow->pixmap = object->pixmap();
            if (!shapedPixmapWindow->pixmap.isNull())
                shapedPixmapWindow->hotSpot = object->hotSpot();
        }
        if (shapedPixmapWindow->pixmap.isNull())
            shapedPixmapWindow->pixmap = QPixmap(default_pm);
        shapedPixmapWindow->setGeometry(QRect(QCursor::pos() - shapedPixmapWindow->hotSpot, shapedPixmapWindow->pixmap.size()));
        shapedPixmapWindow->show();
        shapedPixmapWindow->render();
    }
}

void QDragManager::updateCursor()
{
    if (shapedPixmapWindow) {
        shapedPixmapWindow->render(); // ### Hack
        shapedPixmapWindow->move(QCursor::pos() - shapedPixmapWindow->hotSpot);
    }

    Qt::CursorShape cursorShape = Qt::ForbiddenCursor;
    if (willDrop) {
        if (global_accepted_action == Qt::CopyAction) {
            cursorShape = Qt::DragCopyCursor;
        } else if (global_accepted_action == Qt::LinkAction) {
            cursorShape = Qt::DragLinkCursor;
        } else {
            cursorShape = Qt::DragMoveCursor;
        }
    }
    QCursor *cursor = qApp->overrideCursor();
    if (cursor && cursorShape != cursor->shape())
        qApp->changeOverrideCursor(QCursor(cursorShape));
}


bool QDragManager::eventFilter(QObject *o, QEvent *e)
{
    if (beingCancelled) {
        if (e->type() == QEvent::KeyRelease && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            qApp->removeEventFilter(this);
            Q_ASSERT(object == 0);
            beingCancelled = false;
            eventLoop->exit();
            return true; // block the key release
        }
        return false;
    }

    Q_ASSERT(object != 0);

    if (!qobject_cast<QWindow *>(o))
        return false;

    switch(e->type()) {
        case QEvent::ShortcutOverride:
            // prevent accelerators from firing while dragging
            e->accept();
            return true;

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            QKeyEvent *ke = static_cast<QKeyEvent *>(e);
            if (ke->key() == Qt::Key_Escape && e->type() == QEvent::KeyPress) {
                cancel();
                qApp->removeEventFilter(this);
                beingCancelled = false;
                eventLoop->exit();
            } else {
                // ### x11 forces move!
                updateCursor();
            }
            return true; // Eat all key events
        }

        case QEvent::MouseMove:
            move(static_cast<QMouseEvent *>(e));
            return true; // Eat all mouse events

        case QEvent::MouseButtonRelease:
            qApp->removeEventFilter(this);
            if (willDrop)
                drop(static_cast<QMouseEvent *>(e));
            else
                cancel();
            beingCancelled = false;
            eventLoop->exit();
            return true; // Eat all mouse events

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
            return true;
        default:
             break;
    }
    return false;
}

Qt::DropAction QDragManager::drag(QDrag *o)
{
    if (!o || object == o)
         return Qt::IgnoreAction;

    if (!platformDrag || !o->source()) {
        o->deleteLater();
        return Qt::IgnoreAction;
    }

    if (object) {
        cancel();
        qApp->removeEventFilter(this);
        beingCancelled = false;
    }

    object = o;
    if (!shapedPixmapWindow)
        shapedPixmapWindow = new QShapedPixmapWindow();
    oldstate = Qt::NoModifier; // #### Should use state that caused the drag
//    drag_mode = mode;

    possible_actions = dragPrivate()->possible_actions;

    willDrop = false;
    object->d_func()->target = 0;
    qApp->installEventFilter(this);

    global_accepted_action = Qt::CopyAction;
#ifndef QT_NO_CURSOR
    qApp->setOverrideCursor(Qt::DragCopyCursor);
    restoreCursor = true;
    updateCursor();
#endif
    updatePixmap();

    platformDrag->startDrag();

    eventLoop = new QEventLoop;
    (void) eventLoop->exec();
    delete eventLoop;
    eventLoop = 0;

    delete shapedPixmapWindow;
    shapedPixmapWindow = 0;

    return global_accepted_action;
}

void QDragManager::move(const QMouseEvent *me)
{
    if (!platformDrag)
        return;

    platformDrag->move(me);
}

void QDragManager::drop(const QMouseEvent *me)
{
    if (!platformDrag)
        return;

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        QGuiApplication::restoreOverrideCursor();
        restoreCursor = false;
    }
#endif
    willDrop = false;

    platformDrag->drop(me);

    if (object)
        object->deleteLater();
    object = 0;
}

void QDragManager::cancel(bool deleteSource)
{
    if (!platformDrag)
        return;

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        QGuiApplication::restoreOverrideCursor();
        restoreCursor = false;
    }
#endif

    beingCancelled = true;

    platformDrag->cancel();

    if (object && deleteSource)
        object->deleteLater();
    object = 0;

    global_accepted_action = Qt::IgnoreAction;
}

/*!
  Called from startDrag() in QPlatformDrag implementations that do not need
  the desktop-oriented stuff provided by the event filter (e.g. because their
  drag is not based on mouse events). Instead, they will manage everything on
  their own, will not rely on move/drop/cancel, and will call stopDrag() to stop
  the event loop when the drag is over.
 */
void QDragManager::unmanageEvents()
{
    qApp->removeEventFilter(this);
}

void QDragManager::stopDrag()
{
    if (eventLoop)
        eventLoop->exit();
}

#endif // QT_NO_DRAGANDDROP

#if !(defined(QT_NO_DRAGANDDROP) && defined(QT_NO_CLIPBOARD))

static QStringList imageReadMimeFormats()
{
    QStringList formats;
    QList<QByteArray> imageFormats = QImageReader::supportedImageFormats();
    for (int i = 0; i < imageFormats.size(); ++i) {
        QString format = QLatin1String("image/");
        format += QString::fromLatin1(imageFormats.at(i).toLower());
        formats.append(format);
    }

    //put png at the front because it is best
    int pngIndex = formats.indexOf(QLatin1String("image/png"));
    if (pngIndex != -1 && pngIndex != 0)
        formats.move(pngIndex, 0);

    return formats;
}


static QStringList imageWriteMimeFormats()
{
    QStringList formats;
    QList<QByteArray> imageFormats = QImageWriter::supportedImageFormats();
    for (int i = 0; i < imageFormats.size(); ++i) {
        QString format = QLatin1String("image/");
        format += QString::fromLatin1(imageFormats.at(i).toLower());
        formats.append(format);
    }

    //put png at the front because it is best
    int pngIndex = formats.indexOf(QLatin1String("image/png"));
    if (pngIndex != -1 && pngIndex != 0)
        formats.move(pngIndex, 0);

    return formats;
}

QInternalMimeData::QInternalMimeData()
    : QMimeData()
{
}

QInternalMimeData::~QInternalMimeData()
{
}

bool QInternalMimeData::hasFormat(const QString &mimeType) const
{
    bool foundFormat = hasFormat_sys(mimeType);
    if (!foundFormat && mimeType == QLatin1String("application/x-qt-image")) {
        QStringList imageFormats = imageReadMimeFormats();
        for (int i = 0; i < imageFormats.size(); ++i) {
            if ((foundFormat = hasFormat_sys(imageFormats.at(i))))
                break;
        }
    }
    return foundFormat;
}

QStringList QInternalMimeData::formats() const
{
    QStringList realFormats = formats_sys();
    if (!realFormats.contains(QLatin1String("application/x-qt-image"))) {
        QStringList imageFormats = imageReadMimeFormats();
        for (int i = 0; i < imageFormats.size(); ++i) {
            if (realFormats.contains(imageFormats.at(i))) {
                realFormats += QLatin1String("application/x-qt-image");
                break;
            }
        }
    }
    return realFormats;
}

QVariant QInternalMimeData::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    QVariant data = retrieveData_sys(mimeType, type);
    if (mimeType == QLatin1String("application/x-qt-image")) {
        if (data.isNull() || (data.type() == QVariant::ByteArray && data.toByteArray().isEmpty())) {
            // try to find an image
            QStringList imageFormats = imageReadMimeFormats();
            for (int i = 0; i < imageFormats.size(); ++i) {
                data = retrieveData_sys(imageFormats.at(i), type);
                if (data.isNull() || (data.type() == QVariant::ByteArray && data.toByteArray().isEmpty()))
                    continue;
                break;
            }
        }
        // we wanted some image type, but all we got was a byte array. Convert it to an image.
        if (data.type() == QVariant::ByteArray
            && (type == QVariant::Image || type == QVariant::Pixmap || type == QVariant::Bitmap))
            data = QImage::fromData(data.toByteArray());

    } else if (mimeType == QLatin1String("application/x-color") && data.type() == QVariant::ByteArray) {
        QColor c;
        QByteArray ba = data.toByteArray();
        if (ba.size() == 8) {
            ushort * colBuf = (ushort *)ba.data();
            c.setRgbF(qreal(colBuf[0]) / qreal(0xFFFF),
                      qreal(colBuf[1]) / qreal(0xFFFF),
                      qreal(colBuf[2]) / qreal(0xFFFF),
                      qreal(colBuf[3]) / qreal(0xFFFF));
            data = c;
        } else {
            qWarning("Qt: Invalid color format");
        }
    } else if (data.type() != type && data.type() == QVariant::ByteArray) {
        // try to use mime data's internal conversion stuf.
        QInternalMimeData *that = const_cast<QInternalMimeData *>(this);
        that->setData(mimeType, data.toByteArray());
        data = QMimeData::retrieveData(mimeType, type);
        that->clear();
    }
    return data;
}

bool QInternalMimeData::canReadData(const QString &mimeType)
{
    return imageReadMimeFormats().contains(mimeType);
}

// helper functions for rendering mimedata to the system, this is needed because QMimeData is in core.
QStringList QInternalMimeData::formatsHelper(const QMimeData *data)
{
    QStringList realFormats = data->formats();
    if (realFormats.contains(QLatin1String("application/x-qt-image"))) {
        // add all supported image formats
        QStringList imageFormats = imageWriteMimeFormats();
        for (int i = 0; i < imageFormats.size(); ++i) {
            if (!realFormats.contains(imageFormats.at(i)))
                realFormats.append(imageFormats.at(i));
        }
    }
    return realFormats;
}

bool QInternalMimeData::hasFormatHelper(const QString &mimeType, const QMimeData *data)
{

    bool foundFormat = data->hasFormat(mimeType);
    if (!foundFormat) {
        if (mimeType == QLatin1String("application/x-qt-image")) {
            // check all supported image formats
            QStringList imageFormats = imageWriteMimeFormats();
            for (int i = 0; i < imageFormats.size(); ++i) {
                if ((foundFormat = data->hasFormat(imageFormats.at(i))))
                    break;
            }
        } else if (mimeType.startsWith(QLatin1String("image/"))) {
            return data->hasImage() && imageWriteMimeFormats().contains(mimeType);
        }
    }
    return foundFormat;
}

QByteArray QInternalMimeData::renderDataHelper(const QString &mimeType, const QMimeData *data)
{
    QByteArray ba;
    if (mimeType == QLatin1String("application/x-color")) {
        /* QMimeData can only provide colors as QColor or the name
           of a color as a QByteArray or a QString. So we need to do
           the conversion to application/x-color here.
           The application/x-color format is :
           type: application/x-color
           format: 16
           data[0]: red
           data[1]: green
           data[2]: blue
           data[3]: opacity
        */
        ba.resize(8);
        ushort * colBuf = (ushort *)ba.data();
        QColor c = qvariant_cast<QColor>(data->colorData());
        colBuf[0] = ushort(c.redF() * 0xFFFF);
        colBuf[1] = ushort(c.greenF() * 0xFFFF);
        colBuf[2] = ushort(c.blueF() * 0xFFFF);
        colBuf[3] = ushort(c.alphaF() * 0xFFFF);
    } else {
        ba = data->data(mimeType);
        if (ba.isEmpty()) {
            if (mimeType == QLatin1String("application/x-qt-image") && data->hasImage()) {
                QImage image = qvariant_cast<QImage>(data->imageData());
                QBuffer buf(&ba);
                buf.open(QBuffer::WriteOnly);
                // would there not be PNG ??
                image.save(&buf, "PNG");
            } else if (mimeType.startsWith(QLatin1String("image/")) && data->hasImage()) {
                QImage image = qvariant_cast<QImage>(data->imageData());
                QBuffer buf(&ba);
                buf.open(QBuffer::WriteOnly);
                image.save(&buf, mimeType.mid(mimeType.indexOf(QLatin1Char('/')) + 1).toLatin1().toUpper());
            }
        }
    }
    return ba;
}

#endif // QT_NO_DRAGANDDROP && QT_NO_CLIPBOARD

QT_END_NAMESPACE
