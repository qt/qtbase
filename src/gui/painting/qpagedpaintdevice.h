
#ifndef QPAGEDPAINTDEVICE_H
#define QPAGEDPAINTDEVICE_H

#include <QtGui/qpaintdevice.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPagedPaintDevicePrivate;

class Q_GUI_EXPORT QPagedPaintDevice : public QPaintDevice
{
public:
    QPagedPaintDevice();
    ~QPagedPaintDevice();

    virtual bool newPage() = 0;

    enum PageSize { A4, B5, Letter, Legal, Executive,
                    A0, A1, A2, A3, A5, A6, A7, A8, A9, B0, B1,
                    B10, B2, B3, B4, B6, B7, B8, B9, C5E, Comm10E,
                    DLE, Folio, Ledger, Tabloid, Custom, NPageSize = Custom };

    virtual void setPageSize(PageSize size);
    PageSize pageSize() const;

    virtual void setPageSizeMM(const QSizeF &size);
    QSizeF pageSizeMM() const;

    struct Margins {
        qreal left;
        qreal right;
        qreal top;
        qreal bottom;
    };

    virtual void setMargins(const Margins &margins);
    Margins margins() const;

protected:
    friend class QPagedPaintDevicePrivate;
    QPagedPaintDevicePrivate *d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
