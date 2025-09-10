// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvncscreen.h"
#include "qvnc_p.h"
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtFbSupport/private/qfbcursor_p.h>

#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtCore/QRegularExpression>


QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;


QVncScreen::QVncScreen(const QStringList &args)
    : mArgs(args)
{
    initialize();
}

QVncScreen::~QVncScreen()
{
#if QT_CONFIG(cursor)
    if (clientCursor)
        delete clientCursor;
#endif
}

bool QVncScreen::initialize()
{
    QRegularExpression sizeRx("size=(\\d+)x(\\d+)"_L1);
    QRegularExpression mmSizeRx("mmsize=(?<width>(\\d*\\.)?\\d+)x(?<height>(\\d*\\.)?\\d+)"_L1);
    QRegularExpression depthRx("depth=(\\d+)"_L1);

    mGeometry = QRect(0, 0, 1024, 768);
    mFormat = QImage::Format_ARGB32_Premultiplied;
    mDepth = 32;
    mPhysicalSize = QSizeF(mGeometry.width()/96.*25.4, mGeometry.height()/96.*25.4);

    for (const QString &arg : std::as_const(mArgs)) {
        QRegularExpressionMatch match;
        if (arg.contains(mmSizeRx, &match)) {
            mPhysicalSize = QSizeF(match.captured("width").toDouble(), match.captured("height").toDouble());
        } else if (arg.contains(sizeRx, &match)) {
            mGeometry.setSize(QSize(match.captured(1).toInt(), match.captured(2).toInt()));
        } else if (arg.contains(depthRx, &match)) {
            mDepth = match.captured(1).toInt();
        }
    }

    switch (depth()) {
    case 32:
        dirty = new QVncDirtyMapOptimized<quint32>(this);
        break;
    case 16:
        dirty = new QVncDirtyMapOptimized<quint16>(this);
        mFormat = QImage::Format_RGB16;
        break;
    case 8:
        dirty = new QVncDirtyMapOptimized<quint8>(this);
        break;
    default:
        qWarning("QVNCScreen::initDevice: No support for screen depth %d",
                 depth());
        dirty = nullptr;
        return false;
    }

    QFbScreen::initializeCompositor();

    setPowerState(PowerStateOff);

    return true;
}

QRegion QVncScreen::doRedraw()
{
    QRegion touched = QFbScreen::doRedraw();

    if (touched.isEmpty())
        return touched;
    dirtyRegion += touched;

    vncServer->setDirty();
    return touched;
}


void QVncScreen::enableClientCursor(QVncClient *client)
{
#if QT_CONFIG(cursor)
    delete mCursor;
    mCursor = nullptr;
    if (!clientCursor)
        clientCursor = new QVncClientCursor();
    clientCursor->addClient(client);
#else
    Q_UNUSED(client);
#endif
}

void QVncScreen::disableClientCursor(QVncClient *client)
{
#if QT_CONFIG(cursor)
    if (!clientCursor)
        return;

    uint clientCount = clientCursor->removeClient(client);
    if (clientCount == 0) {
        delete clientCursor;
        clientCursor = nullptr;

        if (mCursor == nullptr)
            mCursor = new QFbCursor(this);
    }
#else
    Q_UNUSED(client);
#endif
}

QPlatformCursor *QVncScreen::cursor() const
{
#if QT_CONFIG(cursor)
    return mCursor ? static_cast<QPlatformCursor *>(mCursor) : static_cast<QPlatformCursor *>(clientCursor);
#else
    return nullptr;
#endif
}

// grabWindow() grabs "from the screen" not from the backingstores.
// In linuxfb's case it will also include the mouse cursor.
QPixmap QVncScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    if (!wid) {
        if (width < 0)
            width = mScreenImage.width() - x;
        if (height < 0)
            height = mScreenImage.height() - y;
        return QPixmap::fromImage(mScreenImage).copy(x, y, width, height);
    }

    QFbWindow *window = windowForId(wid);
    if (window) {
        const QRect geom = window->geometry();
        if (width < 0)
            width = geom.width() - x;
        if (height < 0)
            height = geom.height() - y;
        QRect rect(geom.topLeft() + QPoint(x, y), QSize(width, height));
        rect &= window->geometry();
        return QPixmap::fromImage(mScreenImage).copy(rect);
    }

    return QPixmap();
}

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
bool QVncScreen::swapBytes() const
{
    return false;

    /* TODO
    if (depth() != 16)
        return false;

    if (screen())
        return screen()->frameBufferLittleEndian();
    return frameBufferLittleEndian();
    */
}
#endif

QFbScreen::Flags QVncScreen::flags() const
{
    return QFbScreen::DontForceFirstWindowToFullScreen;
}

QT_END_NAMESPACE

#include "moc_qvncscreen.cpp"

