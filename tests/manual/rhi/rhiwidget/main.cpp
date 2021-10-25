/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
