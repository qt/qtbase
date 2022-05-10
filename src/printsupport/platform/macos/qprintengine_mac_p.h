// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPRINTENGINE_MAC_P_H
#define QPRINTENGINE_MAC_P_H

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

#include <QtPrintSupport/qtprintsupportglobal.h>

#ifndef QT_NO_PRINTER

#include <QtPrintSupport/qprinter.h>
#include <QtPrintSupport/qprintengine.h>
#include <QtGui/private/qpainter_p.h>
#include <QtGui/qpagelayout.h>

#include "qcocoaprintdevice_p.h"

#include "qpaintengine_mac_p.h"

Q_FORWARD_DECLARE_OBJC_CLASS(NSPrintInfo);

QT_BEGIN_NAMESPACE

class QPrinterPrivate;
class QMacPrintEnginePrivate;
class QMacPrintEngine : public QPaintEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(QMacPrintEngine)
public:
    QMacPrintEngine(QPrinter::PrinterMode mode, const QString &deviceId);

    Qt::HANDLE handle() const;

    bool begin(QPaintDevice *dev);
    bool end();
    virtual QPaintEngine::Type type() const { return QPaintEngine::MacPrinter; }

    QPaintEngine *paintEngine() const;

    void setProperty(PrintEnginePropertyKey key, const QVariant &value);
    QVariant property(PrintEnginePropertyKey key) const;

    QPrinter::PrinterState printerState() const;

    bool newPage();
    bool abort();
    int metric(QPaintDevice::PaintDeviceMetric) const;

    NSPrintInfo *printInfo();

    //forwarded functions

    void updateState(const QPaintEngineState &state);

    virtual void drawLines(const QLineF *lines, int lineCount);
    virtual void drawRects(const QRectF *r, int num);
    virtual void drawPoints(const QPointF *p, int pointCount);
    virtual void drawEllipse(const QRectF &r);
    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr, Qt::ImageConversionFlags flags);
    virtual void drawTextItem(const QPointF &p, const QTextItem &ti);
    virtual void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s);
    virtual void drawPath(const QPainterPath &);

private:
    friend class QCocoaNativeInterface;
};

class QMacPrintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QMacPrintEngine)
public:
    QPrinter::PrinterMode mode;
    QPrinter::PrinterState state;
    QSharedPointer<QCocoaPrintDevice> m_printDevice;
    QPageLayout m_pageLayout;
    NSPrintInfo *printInfo;
    PMResolution resolution;
    QString outputFilename;
    QString m_creator;
    QPaintEngine *paintEngine;
    QHash<QMacPrintEngine::PrintEnginePropertyKey, QVariant> valueCache;
    uint embedFonts;

    QMacPrintEnginePrivate() : mode(QPrinter::ScreenResolution), state(QPrinter::Idle),
                               m_pageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(0, 0, 0, 0))),
                               printInfo(nullptr), paintEngine(nullptr), embedFonts(true) {}
    ~QMacPrintEnginePrivate();

    void initialize();
    void releaseSession();
    bool newPage_helper();
    void setPageSize(const QPageSize &pageSize);
    inline bool isPrintSessionInitialized() const
    {
        return printInfo != 0;
    }

    PMPageFormat format() const { return static_cast<PMPageFormat>([printInfo PMPageFormat]); }
    PMPrintSession session() const { return static_cast<PMPrintSession>([printInfo PMPrintSession]); }
    PMPrintSettings settings() const { return static_cast<PMPrintSettings>([printInfo PMPrintSettings]); }

    QPaintEngine *aggregateEngine() override { return paintEngine; }
    Qt::HANDLE nativeHandle() override { return q_func()->handle(); }
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTENGINE_WIN_P_H
