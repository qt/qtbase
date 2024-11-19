// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>
#include <qpa/qplatformscreen.h>

QString debugDescription(const QColorSpace &colorSpace)
{
    QString str;
    QDebug dbg(&str);
    dbg << colorSpace;
    return str;
}

// The color space of our assets that we need to color match from.
// In real world apps this could be different for each QImage etc.
static const QColorSpace assetColorSpace = QColorSpace::AdobeRgb;

class TestWindow : public QRasterWindow
{
public:
    using QRasterWindow::QRasterWindow;
    TestWindow()
    {
        resize(900, 300);
        setFlag(Qt::NoDropShadowWindowHint);

        // Moving a window to a different screen may not result
        // in an automatic expose/paint event, as the window's
        // content might still be valid on the new screen. In
        // our case we know it's not, as we need to update the
        // screen information we reflect in the paintEvent below.
        QObject::connect(this, &QWindow::screenChanged,
                         this, qOverload<>(&QRasterWindow::update));
    }

    bool colorManaged = false;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);

        auto requestedColorSpace = requestedFormat().colorSpace();
        auto windowColorSpace = format().colorSpace();
        auto screenColorSpace = screen()->handle()->colorSpace();

        auto targetColorSpace = windowColorSpace;
        if (!targetColorSpace.isValid()) {
            qWarning("Window does not report color space! Using screen color space as target");
            targetColorSpace = screenColorSpace;
        }
        auto colorTransform = assetColorSpace.transformationToColorSpace(targetColorSpace);

        if (!colorManaged && requestedColorSpace.isValid() && !colorTransform.isIdentity()) {
            qWarning() << "Requested" << requestedColorSpace << "but got"
                << targetColorSpace << "and not prepared to do color matching";
        }

        QColor colors[] = {
            QColor(Qt::red),
            QColor(Qt::green),
            QColor(Qt::blue),
            QColor(Qt::cyan),
            QColor(Qt::magenta),
            QColor(Qt::yellow)
        };
        qreal colorWidth = width() / qreal(std::size(colors));
        for (size_t i = 0; i < std::size(colors); ++i) {
            QColor color = colors[i];
            if (colorManaged)
                color = colorTransform.map(color);

            painter.fillRect(QRectF(colorWidth * i, 0, colorWidth, height()), color);
        }

        QRect rect(0, 0, width(), height());
        painter.fillRect(rect.adjusted(20, 100, -20, -100), Qt::white);
        painter.drawText(rect, Qt::AlignCenter,
            QString("Assets: %1\nRequested: %2\nWindow: %3\nScreen: %4").arg(
                debugDescription(assetColorSpace)
            ).arg(
                debugDescription(windowColorSpace)
            ).arg(
                debugDescription(windowColorSpace)
            ).arg(
                debugDescription(screenColorSpace)
            ));
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    TestWindow defaultColorSpaceWindow;
    defaultColorSpaceWindow.setTitle(QString(
        "Color-space unaware (pass-through, assuming surface/screen is %1)").arg(
        assetColorSpace.description()));
    defaultColorSpaceWindow.show();

    TestWindow colorSpaceAwareWindow;
    colorSpaceAwareWindow.setTitle(QString(
        "Color-space aware (match %1 to surface)").arg(assetColorSpace.description()));
    colorSpaceAwareWindow.colorManaged = true;
    colorSpaceAwareWindow.show();

    TestWindow explicitColorSpaceWindow;
    explicitColorSpaceWindow.setTitle(QString(
        "Explicit %1 surface").arg(assetColorSpace.description()));
    auto format = explicitColorSpaceWindow.format();
    format.setColorSpace(assetColorSpace);
    explicitColorSpaceWindow.setFormat(format);
    explicitColorSpaceWindow.show();

    return app.exec();
}
