/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <qapplication.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qbitmap.h>

static QColor baseColor( int k, int intensity )
{
    int r = ( k & 1 ) * intensity;
    int g = ( (k>>1) & 1 ) * intensity;
    int b = ( (k>>2) & 1 ) * intensity;
    return QColor( r, g, b );
}

static QPixmap createDestPixmap()
{
    const int colorbands = 3;
    const int intensities = 4;
    QPixmap pm( 32, colorbands*intensities*4 );
    QPainter painter;
    painter.begin( &pm );
    for ( int i=0; i<colorbands; i++ ) {
        for (int j = 0; j < intensities; j++) {
            int intensity = 255 * (j+1) / intensities; // 25%, 50%, 75% and 100%
            for (int k = 0; k < 8; k++) {
                QColor col = baseColor(k, intensity);
                painter.setPen(QPen(col, 1));
                painter.setBrush(col);
                painter.drawRect(k*4, j*4 + i*intensities*4, 4, 4);
            }
        }
    }
    painter.end();
    return pm;
}

static QBitmap createDestBitmap()
{
    // create a bitmap that looks like:
    // (0 is color0 and 1 is color1)
    //  00001111
    //  00001111
    //  00001111
    //  00001111
    //  00001111
    //  00001111
    //  00001111
    //  00001111
    QBitmap bm( 8, 8 );
    QPainter painter;
    painter.begin( &bm );
    painter.setPen( QPen( Qt::color0, 4 ) );
    painter.drawLine( 2, 0, 2, 8 );
    painter.setPen( QPen( Qt::color1, 4 ) );
    painter.drawLine( 6, 0, 6, 8 );
    painter.end();
    return bm;
}

static QBitmap createSrcBitmap( int size, int border )
{
    // create the source bitmap that looks like
    // (for size=4 and border=2):
    //
    //
    //    1111
    //    1111
    //    0000
    //    0000
    //
    //
    // If \a border is 0, the bitmap does not have a mask, otherwise the inner
    // part is masked.
    // \a size specifies the size of the inner (i.e. masked) part. It should be
    // a multiple of 2.
    int size2 = size/2;
    int totalSize = 2 * ( size2 + border );
    QBitmap bm( totalSize, totalSize );
    QPainter painter;
    painter.begin( &bm );
    painter.setPen( QPen( Qt::color0, 1 ) );
    painter.setBrush( Qt::color0 );
    painter.drawRect( border, size2+border, size, size2 );
    painter.setPen( QPen( Qt::color1, 1 ) );
    painter.setBrush( Qt::color1 );
    painter.drawRect( border, border, size, size2 );
    painter.end();
    if ( border > 0 ) {
        QBitmap mask(totalSize, totalSize, true);
        QPainter painter;
        painter.begin(&mask);
        painter.setPen(QPen(Qt::color1, 1));
        painter.setBrush(Qt::color1);
        painter.drawRect(border, border, size, size);
        painter.end();
        bm.setMask(mask);
    }
    return bm;
}


int main( int argc, char **argv )
{
    QApplication a( argc, argv );

    // input for tst_QPainter::drawLine_rop_bitmap()
    {
        QBitmap dst = createDestBitmap();
        dst.save("../../drawLine_rop_bitmap/dst.xbm", "XBM");
    }

    // input for tst_QPainter::drawPixmap_rop_bitmap()
    {
        QBitmap dst = createDestBitmap();
        QBitmap src1 = createSrcBitmap(4, 2);
        QBitmap src2 = createSrcBitmap(4, 0);
        dst.save("../../drawPixmap_rop_bitmap/dst.xbm", "XBM");
        src1.save("../../drawPixmap_rop_bitmap/src1.xbm", "XBM");
        src1.mask()->save("../../drawPixmap_rop_bitmap/src1-mask.xbm", "XBM");
        src2.save("../../drawPixmap_rop_bitmap/src2.xbm", "XBM");
    }

    // input for tst_QPainter::drawPixmap_rop()
    {
        QPixmap dst1 = createDestPixmap();
        QPixmap dst2 = createDestPixmap();
        dst2.resize(32, 32);
        QBitmap src1 = createSrcBitmap(32, 0);

        QBitmap src_tmp = createSrcBitmap(32, 0).xForm(QWMatrix(1, 0, 0, -1, 0, 0));
        src_tmp.resize(32, 48);
        QBitmap src2 = src_tmp.xForm(QWMatrix(1, 0, 0, -1, 0, 0));
        QBitmap mask(32, 48, true);
        {
            QPainter painter;
            painter.begin(&mask);
            painter.setPen(QPen(Qt::color1, 1));
            painter.setBrush(Qt::color1);
            painter.drawRect(0, 16, 32, 32);
            painter.end();
        }
        src2.setMask(mask);

        QBitmap src3 = createSrcBitmap(32, 0).xForm(QWMatrix(1, 0, 0, -1, 0, 0));

        dst1.save("../../drawPixmap_rop/dst1.png", "PNG");
        dst2.save("../../drawPixmap_rop/dst2.png", "PNG");
        src1.save("../../drawPixmap_rop/src1.xbm", "XBM");
        src2.save("../../drawPixmap_rop/src2.xbm", "XBM");
        src2.mask()->save("../../drawPixmap_rop/src2-mask.xbm", "XBM");
        src3.save("../../drawPixmap_rop/src3.xbm", "XBM");
    }
}
