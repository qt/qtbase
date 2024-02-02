// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include "examplewidget.h"

static const bool TEST_OFFSCREEN_GRAB = false;

int main(int argc, char **argv)
{
    qputenv("QSG_INFO", "1");
    QApplication app(argc, argv);

    QVBoxLayout *layout = new QVBoxLayout;

    QLineEdit *edit = new QLineEdit(QLatin1String("Text on cube"));
    QSlider *slider = new QSlider(Qt::Horizontal);
    ExampleRhiWidget *rw = new ExampleRhiWidget;

    QObject::connect(edit, &QLineEdit::textChanged, edit, [edit, rw] {
        rw->setCubeTextureText(edit->text());
    });

    slider->setMinimum(0);
    slider->setMaximum(360);
    QObject::connect(slider, &QSlider::valueChanged, slider, [slider, rw] {
        rw->setCubeRotation(slider->value());
    });

    QPushButton *btn = new QPushButton(QLatin1String("Grab to image"));
    QObject::connect(btn, &QPushButton::clicked, btn, [rw] {
        QImage image = rw->grabTexture();
        qDebug() << image;
        if (!image.isNull()) {
            QFileDialog fd(rw->parentWidget());
            fd.setAcceptMode(QFileDialog::AcceptSave);
            fd.setDefaultSuffix("png");
            fd.selectFile("test.png");
            if (fd.exec() == QDialog::Accepted)
                image.save(fd.selectedFiles().first());
        }
    });
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(btn);
    QCheckBox *cbExplicitSize = new QCheckBox(QLatin1String("Use explicit size"));
    QObject::connect(cbExplicitSize, &QCheckBox::stateChanged, cbExplicitSize, [cbExplicitSize, rw] {
        if (cbExplicitSize->isChecked())
            rw->setExplicitSize(QSize(128, 128));
        else
            rw->setExplicitSize(QSize());
    });
    btnLayout->addWidget(cbExplicitSize);
    QPushButton *btnMakeWindow = new QPushButton(QLatin1String("Make top-level window"));
    QObject::connect(btnMakeWindow, &QPushButton::clicked, btnMakeWindow, [rw, btnMakeWindow, layout] {
        if (rw->parentWidget()) {
            rw->setParent(nullptr);
            rw->setAttribute(Qt::WA_DeleteOnClose, true);
            rw->show();
            btnMakeWindow->setText(QLatin1String("Make child widget"));
        } else {
            rw->setAttribute(Qt::WA_DeleteOnClose, false);
            layout->addWidget(rw);
            btnMakeWindow->setText(QLatin1String("Make top-level window"));
        }
    });
    btnLayout->addWidget(btnMakeWindow);

    layout->addWidget(edit);
    QHBoxLayout *sliderLayout = new QHBoxLayout;
    sliderLayout->addWidget(new QLabel(QLatin1String("Cube rotation")));
    sliderLayout->addWidget(slider);
    layout->addLayout(sliderLayout);
    layout->addLayout(btnLayout);
    layout->addWidget(rw);

    rw->setCubeTextureText(edit->text());

    if (TEST_OFFSCREEN_GRAB) {
        rw->resize(320, 200);
        rw->grabTexture().save("offscreen_grab.png");
    }

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();

    return app.exec();
}
