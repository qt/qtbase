#include "qopenwfdbackingstore.h"

QOpenWFDBackingStore::QOpenWFDBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QPaintDevice * QOpenWFDBackingStore::paintDevice()
{
    return &mImage;
}

//we don't support flush yet :)
void QOpenWFDBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);
}

void QOpenWFDBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);
    mImage = QImage(size,QImage::Format_RGB32);
}
