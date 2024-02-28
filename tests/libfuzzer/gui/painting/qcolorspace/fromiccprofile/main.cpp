// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <cstdlib>

#include <QGuiApplication>
#include <QColor>
#include <QColorSpace>
#include <QImage>

static QImage::Format toFormat(QColorSpace::ColorModel model)
{
    switch (model) {
    case QColorSpace::ColorModel::Rgb:
        return QImage::Format_RGB32;
    case QColorSpace::ColorModel::Gray:
        return QImage::Format_Grayscale16;
    case QColorSpace::ColorModel::Cmyk:
        return QImage::Format_CMYK8888;
    case QColorSpace::ColorModel::Undefined:
        break;
    }
    return QImage::Format_Invalid;
}

extern "C" int LLVMFuzzerTestOneInput(const char *data, size_t size) {
    // to reduce noise and increase speed
    static char quiet[] = "QT_LOGGING_RULES=qt.gui.icc=false";
    static int pe = putenv(quiet);
    Q_UNUSED(pe);
    static int argc = 3;
    static char arg1[] = "fuzzer";
    static char arg2[] = "-platform";
    static char arg3[] = "minimal";
    static char *argv[] = {arg1, arg2, arg3, nullptr};
    static QGuiApplication qga(argc, argv);
    QColorSpace cs = QColorSpace::fromIccProfile(QByteArray::fromRawData(data, size));
    if (cs.isValid()) {
        cs.description();
        QColorTransform trans1 = cs.transformationToColorSpace(QColorSpace::SRgb);
        trans1.isIdentity();
        QColorSpace cs2 = cs;
        cs2.setDescription("Hello");
        Q_ASSERT(cs == cs2);
        QByteArray profileData = cs2.iccProfile();
        QColorSpace cs3 = QColorSpace::fromIccProfile(profileData);
        Q_ASSERT(cs == cs3);
        QColor color(0xfaf8fa00);
        color = trans1.map(color);
        QImage img(16, 2, toFormat(cs.colorModel()));
        img.setColorSpace(cs);
        QImage img2 = img.convertedToColorSpace(QColorSpace::SRgb);
        if (cs.isValidTarget()) {
            QImage img3 = img2.convertedToColorSpace(cs);

            QColorTransform trans2 = QColorSpace(QColorSpace::SRgb).transformationToColorSpace(cs);
            bool a = (trans1 == trans2);
            Q_UNUSED(a);
            color = trans2.map(color);
        }
    }
    return 0;
}
