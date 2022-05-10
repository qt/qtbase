// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qintegrityfbscreen.h"
#include <QtFbSupport/private/qfbcursor_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtCore/QRegularExpression>
#include <QtGui/QPainter>

#include <qimage.h>
#include <qdebug.h>

#include <INTEGRITY.h>
#include <memory_region.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QImage::Format determineFormat(const FBInfo *fbinfo)
{
    QImage::Format format = QImage::Format_Invalid;

    switch (fbinfo->BitsPerPixel) {
    case 32:
        if (fbinfo->Alpha.Bits)
            format = QImage::Format_ARGB32;
        else
            format = QImage::Format_RGB32;
        break;
    case 24:
        format = QImage::Format_RGB888;
        break;
    case 18:
        format = QImage::Format_RGB666;
        break;
    case 16:
        format = QImage::Format_RGB16;
        break;
    case 15:
        format = QImage::Format_RGB555;
        break;
    case 12:
        format = QImage::Format_RGB444;
        break;
    case 8:
        break;
    case 1:
        format = QImage::Format_Mono; //###: LSB???
        break;
    default:
        break;
    }

    return format;
}

QIntegrityFbScreen::QIntegrityFbScreen(const QStringList &args)
    : mArgs(args), mBlitter(0)
{
}

QIntegrityFbScreen::~QIntegrityFbScreen()
{
    if (mFbh) {
        MemoryRegion vmr;
        CheckSuccess(gh_FB_close_munmap(mFbh, &vmr));
        CheckSuccess(DeallocateMemoryRegionWithCookie(__ghs_VirtualMemoryRegionPool,
                                                      vmr, mVMRCookie));
    }

    delete mBlitter;
}

bool QIntegrityFbScreen::initialize()
{
    Error err;
    QRegularExpression fbRx("fb=(.*)"_L1);
    QRegularExpression sizeRx("size=(\\d+)x(\\d+)"_L1);
    QRegularExpression offsetRx("offset=(\\d+)x(\\d+)"_L1);

    QString fbDevice;
    QRect userGeometry;

    // Parse arguments
    foreach (const QString &arg, mArgs) {
        QRegularExpressionMatch match;
        if (arg.contains(sizeRx, &match))
            userGeometry.setSize(QSize(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(offsetRx, &match))
            userGeometry.setTopLeft(QPoint(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(fbRx, &match))
            fbDevice = match.captured(1);
    }

    if (fbDevice.isEmpty()) {
        /* no driver specified, try to get default one */
        err = gh_FB_get_driver_by_name(NULL, &mFbd);
        if (err != Success) {
            uintptr_t context = 0;
            /* no default driver, take the first available one */
            err = gh_FB_get_next_driver(&context, &mFbd);
        }
    } else {
        err = gh_FB_get_driver_by_name(qPrintable(fbDevice), &mFbd);
    }
    if (err != Success) {
        qErrnoWarning("Failed to open framebuffer %s: %d", qPrintable(fbDevice), err);
        return false;
    }

    memset(&mFbinfo, 0, sizeof(FBInfo));
    CheckSuccess(gh_FB_check_info(mFbd, &mFbinfo));
    if (userGeometry.width() && userGeometry.height()) {
        mFbinfo.Width = userGeometry.width();
        mFbinfo.Height = userGeometry.height();
        err = gh_FB_check_info(mFbd, &mFbinfo);
        if (err != Success) {
            qErrnoWarning("Unsupported resolution %dx%d for %s: %d",
                          userGeometry.width(), userGeometry.height(),
                          qPrintable(fbDevice), err);
            return false;
        }
    }

    if (mFbinfo.MMapSize) {
        err = AllocateAnyMemoryRegionWithCookie(__ghs_VirtualMemoryRegionPool,
                                            mFbinfo.MMapSize, &mVMR, &mVMRCookie);
        if (err != Success) {
            qErrnoWarning("Could not mmap: %d", err);
            return false;
        }

        err = gh_FB_open_mmap(mFbd, &mFbinfo, mVMR, &mFbh);
    } else {
        err = gh_FB_open(mFbd, &mFbinfo, &mFbh);
    }
    if (err != Success) {
        qErrnoWarning("Could not open framebuffer: %d", err);
        return false;
    }

    CheckSuccess(gh_FB_get_info(mFbh, &mFbinfo));

    mDepth = mFbinfo.BitsPerPixel;
    mGeometry = QRect(0, 0, mFbinfo.Width, mFbinfo.Height);
    mFormat = determineFormat(&mFbinfo);

    const int dpi = 100;
    int mmWidth = qRound((mFbinfo.Width * 25.4) / dpi);
    int mmHeight = qRound((mFbinfo.Height * 25.4) / dpi);
    mPhysicalSize = QSizeF(mmWidth, mmHeight);

    QFbScreen::initializeCompositor();
    mFbScreenImage = QImage((uchar *)mFbinfo.Start, mFbinfo.Width, mFbinfo.Height,
                            mFbinfo.BytesPerLine, mFormat);

    mCursor = new QFbCursor(this);

    return true;
}

QRegion QIntegrityFbScreen::doRedraw()
{
    QRegion touched = QFbScreen::doRedraw();

    if (touched.isEmpty())
        return touched;

    if (!mBlitter)
        mBlitter = new QPainter(&mFbScreenImage);

    for (QRect rect : touched) {
        FBRect fbrect = {
            (uint32_t)rect.left(),
            (uint32_t)rect.top(),
            (uint32_t)rect.width(),
            (uint32_t)rect.height()
        };
        mBlitter->drawImage(rect, mScreenImage, rect);
        gh_FB_expose(mFbh, &fbrect, NULL);
    }
    return touched;
}

// grabWindow() grabs "from the screen" not from the backingstores.
// In integrityfb's case it will also include the mouse cursor.
QPixmap QIntegrityFbScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    if (!wid) {
        if (width < 0)
            width = mFbScreenImage.width() - x;
        if (height < 0)
            height = mFbScreenImage.height() - y;
        return QPixmap::fromImage(mFbScreenImage).copy(x, y, width, height);
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
        return QPixmap::fromImage(mFbScreenImage).copy(rect);
    }

    return QPixmap();
}

QT_END_NAMESPACE

