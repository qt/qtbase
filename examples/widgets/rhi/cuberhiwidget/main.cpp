// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QFontInfo>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include "examplewidget.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QVBoxLayout *layout = new QVBoxLayout;

    ExampleRhiWidget *rhiWidget = new ExampleRhiWidget;
    QLabel *overlayLabel = new QLabel(rhiWidget);
    overlayLabel->setText(QLatin1String("This is a\nsemi-transparent\n overlay widget\n"
                                        "placed on top of\nthe QRhiWidget."));
    overlayLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    overlayLabel->setAutoFillBackground(true);
    QPalette semiTransparent(QColor(255, 0, 0, 64));
    semiTransparent.setBrush(QPalette::Text, Qt::white);
    semiTransparent.setBrush(QPalette::WindowText, Qt::white);
    overlayLabel->setPalette(semiTransparent);
    QFont f = overlayLabel->font();
    f.setPixelSize(QFontInfo(f).pixelSize() * 2);
    f.setWeight(QFont::Bold);
    overlayLabel->setFont(f);
    overlayLabel->resize(320, 320);
    overlayLabel->hide();
    QObject::connect(rhiWidget, &ExampleRhiWidget::resized, rhiWidget, [rhiWidget, overlayLabel] {
        const int w = overlayLabel->width();
        const int h = overlayLabel->height();
        overlayLabel->setGeometry(rhiWidget->width() / 2 - w / 2, rhiWidget->height() / 2 - h / 2, w, h);
    });

    QTextEdit *edit = new QTextEdit(QLatin1String("QRhiWidget!<br><br>"
                                                  "The cube is textured with QPainter-generated content.<br><br>"
                                                  "Regular, non-native widgets on top work just fine."));
    QObject::connect(edit, &QTextEdit::textChanged, edit, [edit, rhiWidget] {
        rhiWidget->setCubeTextureText(edit->toPlainText());
    });
    edit->setMaximumHeight(100);
    layout->addWidget(edit);

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setMinimum(0);
    slider->setMaximum(360);
    QObject::connect(slider, &QSlider::valueChanged, slider, [slider, rhiWidget] {
        rhiWidget->setCubeRotation(slider->value());
    });

    QHBoxLayout *sliderLayout = new QHBoxLayout;
    sliderLayout->addWidget(new QLabel(QLatin1String("Cube rotation")));
    sliderLayout->addWidget(slider);
    layout->addLayout(sliderLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout;

    QLabel *apiLabel = new QLabel;
    btnLayout->addWidget(apiLabel);
    QObject::connect(rhiWidget, &ExampleRhiWidget::rhiChanged, rhiWidget, [apiLabel](const QString &apiName) {
        apiLabel->setText(QLatin1String("Using QRhi on ") + apiName);
    });

    QPushButton *btnMakeWindow = new QPushButton(QLatin1String("Make top-level window"));
    QObject::connect(btnMakeWindow, &QPushButton::clicked, btnMakeWindow, [rhiWidget, btnMakeWindow, layout] {
        if (rhiWidget->parentWidget()) {
            rhiWidget->setParent(nullptr);
            rhiWidget->setAttribute(Qt::WA_DeleteOnClose, true);
            rhiWidget->show();
            btnMakeWindow->setText(QLatin1String("Make child widget"));
        } else {
            rhiWidget->setAttribute(Qt::WA_DeleteOnClose, false);
            layout->addWidget(rhiWidget);
            btnMakeWindow->setText(QLatin1String("Make top-level window"));
        }
    });
    btnLayout->addWidget(btnMakeWindow);

    QPushButton *btn = new QPushButton(QLatin1String("Grab to image"));
    QObject::connect(btn, &QPushButton::clicked, btn, [rhiWidget] {
        QImage image = rhiWidget->grab();
        qDebug() << "Got image" << image;
        if (!image.isNull()) {
            QFileDialog fd(rhiWidget->parentWidget());
            fd.setAcceptMode(QFileDialog::AcceptSave);
            fd.setDefaultSuffix("png");
            fd.selectFile("test.png");
            if (fd.exec() == QDialog::Accepted)
                image.save(fd.selectedFiles().first());
        }
    });
    btnLayout->addWidget(btn);

    QCheckBox *cbMsaa = new QCheckBox(QLatin1String("Use 4x MSAA"));
    QObject::connect(cbMsaa, &QCheckBox::stateChanged, cbMsaa, [cbMsaa, rhiWidget] {
        if (cbMsaa->isChecked())
            rhiWidget->setSampleCount(4);
        else
            rhiWidget->setSampleCount(1);
    });
    btnLayout->addWidget(cbMsaa);

    QCheckBox *cbOvberlay = new QCheckBox(QLatin1String("Show overlay widget"));
    QObject::connect(cbOvberlay, &QCheckBox::stateChanged, cbOvberlay, [cbOvberlay, overlayLabel] {
        if (cbOvberlay->isChecked())
            overlayLabel->setVisible(true);
        else
            overlayLabel->setVisible(false);
    });
    btnLayout->addWidget(cbOvberlay);

    QCheckBox *cbFlip = new QCheckBox(QLatin1String("Flip"));
    QObject::connect(cbFlip, &QCheckBox::stateChanged, cbOvberlay, [cbFlip, rhiWidget] {
        rhiWidget->setMirrorVertically(cbFlip->isChecked());
    });
    btnLayout->addWidget(cbFlip);

    QCheckBox *cbExplicitSize = new QCheckBox(QLatin1String("Use explicit size"));
    btnLayout->addWidget(cbExplicitSize);
    QSlider *explicitSizeSlider = new QSlider(Qt::Horizontal);
    explicitSizeSlider->setMinimum(16);
    explicitSizeSlider->setMaximum(512);
    btnLayout->addWidget(explicitSizeSlider);

    QObject::connect(cbExplicitSize, &QCheckBox::stateChanged, cbExplicitSize, [cbExplicitSize, explicitSizeSlider, rhiWidget] {
        if (cbExplicitSize->isChecked())
            rhiWidget->setExplicitSize(QSize(explicitSizeSlider->value(), explicitSizeSlider->value()));
        else
            rhiWidget->setExplicitSize(QSize());
    });
    QObject::connect(explicitSizeSlider, &QSlider::valueChanged, explicitSizeSlider, [explicitSizeSlider, cbExplicitSize, rhiWidget] {
        if (cbExplicitSize->isChecked())
            rhiWidget->setExplicitSize(QSize(explicitSizeSlider->value(), explicitSizeSlider->value()));
    });

    // Exit when the detached window is closed; there is not much we can do
    // with the controls in the main window then.
    QObject::connect(rhiWidget, &QObject::destroyed, rhiWidget, [rhiWidget] {
        if (!rhiWidget->parentWidget())
            qGuiApp->quit();
    });

    layout->addLayout(btnLayout);
    layout->addWidget(rhiWidget);

    rhiWidget->setCubeTextureText(edit->toPlainText());

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();

    return app.exec();
}
