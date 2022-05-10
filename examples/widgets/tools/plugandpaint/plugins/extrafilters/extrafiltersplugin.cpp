// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "extrafiltersplugin.h"

#include <QInputDialog>

QStringList ExtraFiltersPlugin::filters() const
{
    return {tr("Flip Horizontally"), tr("Flip Vertically"),
            tr("Smudge..."), tr("Threshold...")};
}

QImage ExtraFiltersPlugin::filterImage(const QString &filter,
                                       const QImage &image, QWidget *parent)
{
    QImage original = image.convertToFormat(QImage::Format_RGB32);
    QImage result = original;

    if (filter == tr("Flip Horizontally")) {
        for (int y = 0; y < original.height(); ++y) {
            for (int x = 0; x < original.width(); ++x) {
                QRgb pixel = original.pixel(original.width() - x - 1, y);
                result.setPixel(x, y, pixel);
            }
        }
    } else if (filter == tr("Flip Vertically")) {
        for (int y = 0; y < original.height(); ++y) {
            for (int x = 0; x < original.width(); ++x) {
                QRgb pixel = original.pixel(x, original.height() - y - 1);
                result.setPixel(x, y, pixel);
            }
        }
    } else if (filter == tr("Smudge...")) {
        bool ok;
        int numIters = QInputDialog::getInt(parent, tr("Smudge Filter"),
                                    tr("Enter number of iterations:"),
                                    5, 1, 20, 1, &ok);
        if (ok) {
            for (int i = 0; i < numIters; ++i) {
                for (int y = 1; y < original.height() - 1; ++y) {
                    for (int x = 1; x < original.width() - 1; ++x) {
                        QRgb p1 = original.pixel(x, y);
                        QRgb p2 = original.pixel(x, y + 1);
                        QRgb p3 = original.pixel(x, y - 1);
                        QRgb p4 = original.pixel(x + 1, y);
                        QRgb p5 = original.pixel(x - 1, y);

                        int red = (qRed(p1) + qRed(p2) + qRed(p3) + qRed(p4)
                                   + qRed(p5)) / 5;
                        int green = (qGreen(p1) + qGreen(p2) + qGreen(p3)
                                     + qGreen(p4) + qGreen(p5)) / 5;
                        int blue = (qBlue(p1) + qBlue(p2) + qBlue(p3)
                                    + qBlue(p4) + qBlue(p5)) / 5;
                        int alpha = (qAlpha(p1) + qAlpha(p2) + qAlpha(p3)
                                     + qAlpha(p4) + qAlpha(p5)) / 5;

                        result.setPixel(x, y, qRgba(red, green, blue, alpha));
                    }
                }
            }
        }
    } else if (filter == tr("Threshold...")) {
        bool ok;
        int threshold = QInputDialog::getInt(parent, tr("Threshold Filter"),
                                                 tr("Enter threshold:"),
                                                 10, 1, 256, 1, &ok);
        if (ok) {
            int factor = 256 / threshold;
            for (int y = 0; y < original.height(); ++y) {
                for (int x = 0; x < original.width(); ++x) {
                    QRgb pixel = original.pixel(x, y);
                    result.setPixel(x, y, qRgba(qRed(pixel) / factor * factor,
                                                qGreen(pixel) / factor * factor,
                                                qBlue(pixel) / factor * factor,
                                                qAlpha(pixel)));
                }
            }
        }
    }
    return result;
}
