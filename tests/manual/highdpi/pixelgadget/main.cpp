// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>
#include <QtWidgets>

class PixelGridViewWidget: public QWidget
{
public:
    PixelGridViewWidget()  {
        setMinimumSize(200, 200);
    }

    QImage sampleImage;
    qreal scale = 1;
    qreal deviceIndependentPixelSize = 40;
    bool drawDipGrid = true;
    bool drawDpGrid = false;
    QVector<QRectF> dpClipRects;
    QVector<QRectF> dipClipRects;

    void paintEvent(QPaintEvent *ev) override {
        QPainter p(this);

        const qreal devicePixelSize = deviceIndependentPixelSize / scale;
        QSize widgetSize = geometry().size();

        p.setClipRect(ev->rect());
        p.fillRect(ev->rect(), QColorConstants::Svg::gray);

        // draw device pixel grid and content
        for (qreal y = 0; y < widgetSize.height(); y += devicePixelSize) {
            for (qreal x = 0; x < widgetSize.width(); x += devicePixelSize) {
                QRectF pixelRect = QRect(x,y, qCeil(devicePixelSize), qCeil(devicePixelSize));

                QPen pen;
                pen.setWidth(1);
                pen.setColor(QColor(100, 100, 100, 100));

                // draw pixel outline
                if (drawDpGrid)
                    p.drawRect(pixelRect);

                // draw content (if in QImage range)
                QPoint imagePos(qRound(x / devicePixelSize), qRound(y / devicePixelSize));
                if (imagePos.x() < sampleImage.width() && imagePos.y() < sampleImage.height()) {
                    QColor pixel = sampleImage.pixelColor(imagePos);
                    p.fillRect(pixelRect, pixel);
                }
            }
        }

        // draw device-independent pixel grid
        if (drawDipGrid)
        for (qreal y = 0; y < widgetSize.height(); y += deviceIndependentPixelSize) {
            for (qreal x = 0; x < widgetSize.width(); x += deviceIndependentPixelSize) {

                QRectF pixelRect = QRect(x,y, deviceIndependentPixelSize, deviceIndependentPixelSize);
                QPen pen;
                pen.setWidth(1);
                pen.setColor(QColor(250, 100, 100, 255));
                p.setPen(pen);
                p.drawRect(pixelRect); // pixel outline
            }
        }

        // draw clip rects
        for (auto it = dpClipRects.begin(); it != dpClipRects.end(); ++it) {
            QRect clipRectRect(it->x() * devicePixelSize, it->y() * devicePixelSize,
                               it->width() * devicePixelSize, it->height() * devicePixelSize);
            QColor yellow(QColorConstants::Svg::yellow);
            p.fillRect(clipRectRect, yellow);
        }
        for (auto it = dipClipRects.begin(); it != dipClipRects.end(); ++it) {
            QRect clipRectRect(it->x() * deviceIndependentPixelSize, it->y() * deviceIndependentPixelSize,
                               it->width() * deviceIndependentPixelSize, it->height() * deviceIndependentPixelSize);
            QColor yellow(QColorConstants::Svg::yellow);
            p.fillRect(clipRectRect, yellow);
        }
    }
};

class PixelGadgetWidget : public QWidget
{
public:
    std::function<void ()> updateSampleImage;
    QVector<QRectF> dpClipRects;
    QVector<QRectF> dipClipRects;

    PixelGadgetWidget() {

        QHBoxLayout *layout = new QHBoxLayout();

        PixelGridViewWidget *pixelGridView = new PixelGridViewWidget();
        layout->addWidget(pixelGridView, 10);

        QVBoxLayout *controlLayout = new QVBoxLayout();
        layout->addLayout(controlLayout);

        controlLayout->addWidget(new QLabel("<b>Content</b>"));

        QComboBox *contentSelect = new QComboBox();
        contentSelect->addItem("<empty>");
        contentSelect->addItem("lines");
        contentSelect->addItem("CE_ShapedFrame (fusion)");
        contentSelect->addItem("CC_ScrollBar (fusion)");
        connect(contentSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(contentSelect);

        QCheckBox *clipping = new QCheckBox("Clipping");
        connect(clipping, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(clipping);

        controlLayout->addWidget(new QLabel("Start Point"));
        QSpinBox *startX = new QSpinBox();
        startX->setValue(1);
        startX->setMinimum(-1000);
        startX->setMaximum(1000);
        connect(startX, QOverload<int>::of(&QSpinBox::valueChanged), [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(startX);

        QSpinBox *startY = new QSpinBox();
        startY->setValue(1);
        startX->setMinimum(-1000);
        startX->setMaximum(1000);
        connect(startY, QOverload<int>::of(&QSpinBox::valueChanged), [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(startY);

        controlLayout->addWidget(new QLabel("<b>Paint Settings</b>"));

        controlLayout->addWidget(new QLabel("Scale"));

        QCheckBox *quarterIncrement = new QCheckBox("25% Scale Increments");
        quarterIncrement->setChecked(true);
        controlLayout->addWidget(quarterIncrement);

        QSpinBox *scale = new QSpinBox();
        scale->setSuffix("%");
        scale->setSingleStep(25);
        connect(quarterIncrement, &QCheckBox::stateChanged, [scale](int val){
            scale->setSingleStep(val > 0 ? 25 : 1);
        });
        scale->setMinimum(100);
        scale->setMaximum(200);
        scale->setValue(100); // 1x
        connect(scale, QOverload<int>::of(&QSpinBox::valueChanged), [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(scale);

        QCheckBox *antialising = new QCheckBox("Antialiasing");
        connect(antialising, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(antialising);

        QCheckBox *offset = new QCheckBox("+0.5 Offset");
        connect(offset, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(offset);

        QCheckBox *dpAllign = new QCheckBox("Device Pixel Aligned Painting");
        connect(dpAllign, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(dpAllign);

        controlLayout->addWidget(new QLabel("<b>Visualization Settings</b>"));

        QSpinBox *pixelSize = new QSpinBox();
        pixelSize->setValue(40);
        pixelSize->setMinimum(10);
        connect(pixelSize, QOverload<int>::of(&QSpinBox::valueChanged), [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(pixelSize);

        QCheckBox *dipGrid = new QCheckBox("Device Independent Pixel Grid");
        dipGrid->setChecked(true);
        connect(dipGrid, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(dipGrid);

        QCheckBox *dpGrid = new QCheckBox("Device Pixel Grid");
        dpGrid->setChecked(false);
        connect(dpGrid, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(dpGrid);

        QCheckBox *dipClipRects = new QCheckBox("Device Independent Clip Rects");
        dipClipRects->setChecked(false);
        connect(dipClipRects, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(dipClipRects);

        QCheckBox *dpClipRects = new QCheckBox("Device Clip Rects");
        dpClipRects->setChecked(false);
        connect(dpClipRects, &QCheckBox::stateChanged, [this](int){ this->updateSampleImage(); });
        controlLayout->addWidget(dpClipRects);

        controlLayout->addStretch(10);

        setLayout(layout);

        auto setClip = [this](QPainter *p, QRect clipRect, bool enableClip, qreal currentScale, QPoint currentStart) {
            if (enableClip) {
                // Set the clip rect without the sub-pixel offset in order to
                // get the same device clip rect as the no-offset case. (This
                // simulates the case where painting code does not control clip
                // rects which have already been set.)
                p->setClipRect(clipRect);

                // Print/record clip debug info - device and device independent rects.
                p->save();
                    p->translate(-currentStart.x(), -currentStart.y());
                    QRegion dipClip = p->clipRegion();
                    p->scale(qreal(1) / currentScale, qreal(1) / currentScale);
                    QRegion dpClip = p->clipRegion();
                    this->dipClipRects.append(dipClip.boundingRect());
                    this->dpClipRects.append(dpClip.boundingRect());
                p->restore();
            } else {
//              p->setClipEnabled(false);
            }
        };

        auto initPainter_1 = [](QPainter *p, QPoint start, qreal scale) {
            p->scale(scale, scale);
            p->translate(start);
        };

        auto initPainter_2 = [](QPainter *p, bool antialias, QPointF offset) {
            p->setRenderHint(QPainter::Antialiasing, antialias);
            p->translate(offset); // sub-(device-independent) pixel offset
        };

        auto drawLines = [=](QPainter *p, QPoint start, qreal scale, bool antialias, bool clip, QPointF offset) {
            QPen pen;
            pen.setColor(QColor(50, 50, 250));

            // 1-width line

            p->save(); {
                QPoint drawPoint = start + QPoint(0, 0);
                pen.setWidth(1);
                p->setPen(pen);
                initPainter_1(p, drawPoint, scale);
                setClip(p, QRect(0, 0, 1, 7), clip, scale, drawPoint);
                initPainter_2(p, antialias, offset);
                p->drawLine(0, 0, 0, 6);
            } p->restore();


            // 2-width line
            p->save(); {
                QPoint drawPoint = start + QPoint(4, 0);
                pen.setWidth(2);
                p->setPen(pen);
                initPainter_1(p, drawPoint, scale);
                setClip(p, QRect(-1, -1, 2, 9), clip, scale, drawPoint);
                initPainter_2(p, antialias, offset);
                p->drawLine(0, 0, 0, 7);
            } p->restore();

            // cosmetic line
            p->save(); {
                QPoint drawPoint = start + QPoint(8, 0);
                pen.setWidth(0);
                p->setPen(pen);
                initPainter_1(p, drawPoint, scale);
                setClip(p, QRect(0, 0, 1, 7), clip, scale, drawPoint);
                initPainter_2(p, antialias, offset);
                p->drawLine(0, 0, 0, 7);
            } p->restore();
        };

        auto drawCE_ShapedFrame = [=](QPainter *p, QPoint start, qreal scale, bool antialias, bool clip, QPointF offset) {

            QRect frameRect(0, 0, 8, 8);

            QStyleOptionFrame opt;
            opt.initFrom(this);
            opt.rect = frameRect;
            opt.frameShape = QFrame::StyledPanel;

            initPainter_1(p, start, scale);
            setClip(p, frameRect, clip, scale, start);
            initPainter_2(p, antialias, offset);

            QStyle *style = QStyleFactory::create("fusion");
            style->drawControl(QStyle::CE_ShapedFrame, &opt, p, nullptr);
        };

        auto drawCC_ScrollBar = [=](QPainter *p, QPoint start, qreal scale, bool antialias, bool clip, QPointF offset) {

            QRect scrollBarRect(0, 0, 100, 18);

            QStyleOptionSlider opt;
            opt.initFrom(this);
//            opt.palette = QPalette(QColor(200, 200, 200)); // force light mode
            opt.rect = scrollBarRect;
            opt.subControls = QStyle::SC_All;
            opt.orientation = Qt::Horizontal;
            opt.minimum = 0;
            opt.maximum = 10;
            opt.sliderPosition = 0;
            opt.sliderValue = 0;
            opt.singleStep = 1;
            opt.pageStep = 5;
            opt.upsideDown = false;
            opt.state |= QStyle::State_Horizontal;
            //opt.state |= QStyle::State_On;

            initPainter_1(p, start, scale);
            setClip(p, scrollBarRect.adjusted(0, 0, 0, 2), clip, scale, start);
            initPainter_2(p, antialias, offset);

            QStyle *style = QStyleFactory::create("fusion");
            style->drawComplexControl(QStyle::CC_ScrollBar, &opt, p, nullptr);
        };

        updateSampleImage = [=]() {

            bool clip = clipping->isChecked();
            QPoint start(startX->value(), startY->value());
            qreal _scale = qreal(scale->value()) / 100.0;
            bool antialias = antialising->isChecked();

            // Set up sub-pixel offset
            QPointF _offset(0, 0);
            if (offset->isChecked()) {
                _offset += QPointF(0.5, 0.5);
            }
            if (dpAllign->isChecked()) {

                // Align to the closest device pixel, in accordance
                QPointF dpStart = QPointF(start) * _scale;
                QPointF dpStartRounded(qCeil(dpStart.x()), qCeil(dpStart.y()));     // down/right
//                QPointF dpStartRounded(qRound(dpStart.x()), qRound(dpStart.y())); // nearest

                QPointF offset = dpStartRounded - dpStart;
/*
                qDebug() << "";
                qDebug() << "start" << start;
                qDebug() << "dpStart" << dpStart;
                qDebug() << "dpStartRounded" << dpStartRounded;
                qDebug() << "offset" << offset;
*/
                _offset += offset;
            }

//            qDebug() << "offset" << _offset;

            QImage img(200, 200, QImage::Format_ARGB32_Premultiplied);
            img.fill(QColorConstants::Svg::gray);
            QPainter painter(&img);

            // Prepare for recording clip rects during paint
            this->dipClipRects.clear();
            this->dpClipRects.clear();

            // paint currently selected content
            switch (contentSelect->currentIndex()) {
                case 0:
                break;
                case 1:
                    drawLines(&painter, start, _scale, antialias, clip, _offset);
                break;
                case 2:
                    drawCE_ShapedFrame(&painter, start, _scale, antialias, clip, _offset);
                break;
                case 3:
                    drawCC_ScrollBar(&painter, start, _scale, antialias, clip, _offset);
                break;
            };

            img.save("sampleimage.png");

            // Update the pixel grid view
            pixelGridView->sampleImage = img;
            pixelGridView->scale = _scale;
            pixelGridView->deviceIndependentPixelSize = pixelSize->value();
            pixelGridView->drawDipGrid = dipGrid->isChecked();
            pixelGridView->drawDpGrid = dpGrid->isChecked();
            pixelGridView->dipClipRects.clear();
            if (dipClipRects->isChecked())
                pixelGridView->dipClipRects = this->dipClipRects;
            pixelGridView->dpClipRects.clear();
            if (dpClipRects->isChecked())
                pixelGridView->dpClipRects = this->dpClipRects;
            pixelGridView->update();
        };

        updateSampleImage();
    }
};

int main(int argc, char **argv) {

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    PixelGadgetWidget pixelGadget;
    pixelGadget.resize(400, 300);
    pixelGadget.show();

    return app.exec();
}

