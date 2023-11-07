// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPRINTENGINE_WIN_P_H
#define QPRINTENGINE_WIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of other Qt classes. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#ifndef QT_NO_PRINTER

#include <QtGui/qpaintengine.h>
#include <QtGui/qpagelayout.h>
#include <QtPrintSupport/QPrintEngine>
#include <QtPrintSupport/QPrinter>
#include <private/qpaintengine_alpha_p.h>
#include <private/qprintdevice_p.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class QWin32PrintEnginePrivate;
class QPrinterPrivate;
class QPainterState;

class Q_PRINTSUPPORT_EXPORT QWin32PrintEngine : public QAlphaPaintEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(QWin32PrintEngine)
public:
    QWin32PrintEngine(QPrinter::PrinterMode mode, const QString &deviceId);

    // override QWin32PaintEngine
    bool begin(QPaintDevice *dev) override;
    bool end() override;

    void updateState(const QPaintEngineState &state) override;

    void updateMatrix(const QTransform &matrix);
    void updateClipPath(const QPainterPath &clip, Qt::ClipOperation op);

    void drawPath(const QPainterPath &path) override;
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    void drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &p) override;
    void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
    QVariant property(PrintEnginePropertyKey key) const override;

    bool newPage() override;
    bool abort() override;
    int metric(QPaintDevice::PaintDeviceMetric) const override;

    QPrinter::PrinterState printerState() const override;

    QPaintEngine::Type type() const override { return Windows; }

    HDC getDC() const;
    void releaseDC(HDC) const;

    /* Used by print/page setup dialogs */
    void setGlobalDevMode(HGLOBAL globalDevNames, HGLOBAL globalDevMode);
    HGLOBAL *createGlobalDevNames();
    HGLOBAL globalDevMode();

private:
    friend class QPrintDialog;
    friend class QPageSetupDialog;
};

class QWin32PrintEnginePrivate : public QAlphaPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QWin32PrintEngine)
public:
    QWin32PrintEnginePrivate() :
        printToFile(false), reinit(false),
        complex_xform(false), has_pen(false), has_brush(false), has_custom_paper_size(false),
        embed_fonts(true)
    {
    }

    ~QWin32PrintEnginePrivate();


    /* Initializes the printer data based on the current printer name. This
       function creates a DEVMODE struct, HDC and a printer handle. If these
       structures are already in use, they are freed using release
    */
    void initialize();

    /* Initializes data in the print engine whenever the HDC has been renewed
    */
    void initHDC();

    /* Releases all the handles the printer currently holds, HDC, DEVMODE,
       etc and resets the corresponding members to 0. */
    void release();

    /* Resets the DC with changes in devmode. If the printer is active
       this function only sets the reinit variable to true so it
       is handled in the next begin or newpage. */
    void doReinit();

    static void initializeDevMode(DEVMODE *);

    bool resetDC();

    void strokePath(const QPainterPath &path, const QColor &color);
    void fillPath(const QPainterPath &path, const QColor &color);

    void composeGdiPath(const QPainterPath &path);
    void fillPath_dev(const QPainterPath &path, const QColor &color);
    void strokePath_dev(const QPainterPath &path, const QColor &color, qreal width);

    void setPageSize(const QPageSize &pageSize);
    void updatePageLayout();
    void updateMetrics();
    void debugMetrics() const;

    // Windows GDI printer references.
    HANDLE hPrinter = nullptr;

    HGLOBAL globalDevMode = nullptr;
    DEVMODE *devMode = nullptr;
    PRINTER_INFO_2 *pInfo = nullptr;
    HGLOBAL hMem = nullptr;

    HDC hdc = nullptr;

    // True if devMode was allocated separately from pInfo.
    bool ownsDevMode = false;

    QPrinter::PrinterMode mode = QPrinter::ScreenResolution;

    // Print Device
    QPrintDevice m_printDevice;

    // Document info
    QString docName;
    QString m_creator;
    QString fileName;

    QPrinter::PrinterState state = QPrinter::Idle;
    int resolution = 0;

    // Page Layout
    QPageLayout m_pageLayout{QPageSize(QPageSize::A4),
                             QPageLayout::Portrait, QMarginsF{0, 0, 0, 0}};
    // Page metrics cache
    QRect m_paintRectPixels;
    QSize m_paintSizeMM;

    // Windows painting
    qreal stretch_x = 1;
    qreal stretch_y = 1;
    int origin_x = 0;
    int origin_y = 0;

    int dpi_x = 96;
    int dpi_y = 96;
    int dpi_display = 96;
    int num_copies = 1;

    uint printToFile : 1;
    uint reinit : 1;

    uint complex_xform : 1;
    uint has_pen : 1;
    uint has_brush : 1;
    uint has_custom_paper_size : 1;
    uint embed_fonts : 1;

    uint txop = 0; // QTransform::TxNone

    QColor brush_color;
    QPen pen;
    QColor pen_color;
    QSizeF paper_size;  // In points

    QTransform painterMatrix;
    QTransform matrix;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTENGINE_WIN_P_H
