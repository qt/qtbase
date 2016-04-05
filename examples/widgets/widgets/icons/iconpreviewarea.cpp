/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtWidgets>

#include "iconpreviewarea.h"

//! [0]
IconPreviewArea::IconPreviewArea(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *mainLayout = new QGridLayout(this);

    for (int row = 0; row < NumStates; ++row) {
        stateLabels[row] = createHeaderLabel(IconPreviewArea::iconStateNames().at(row));
        mainLayout->addWidget(stateLabels[row], row + 1, 0);
    }
    Q_ASSERT(NumStates == 2);

    for (int column = 0; column < NumModes; ++column) {
        modeLabels[column] = createHeaderLabel(IconPreviewArea::iconModeNames().at(column));
        mainLayout->addWidget(modeLabels[column], 0, column + 1);
    }
    Q_ASSERT(NumModes == 4);

    for (int column = 0; column < NumModes; ++column) {
        for (int row = 0; row < NumStates; ++row) {
            pixmapLabels[column][row] = createPixmapLabel();
            mainLayout->addWidget(pixmapLabels[column][row], row + 1, column + 1);
        }
    }
}
//! [0]

#ifdef Q_COMPILER_INITIALIZER_LISTS

//! [42]
QVector<QIcon::Mode> IconPreviewArea::iconModes()
{
    static const QVector<QIcon::Mode> result = {QIcon::Normal, QIcon::Active, QIcon::Disabled, QIcon::Selected};
    return result;
}

QVector<QIcon::State> IconPreviewArea::iconStates()
{
    static const QVector<QIcon::State> result = {QIcon::Off, QIcon::On};
    return result;
}

QStringList IconPreviewArea::iconModeNames()
{
    static const QStringList result = {tr("Normal"), tr("Active"), tr("Disabled"), tr("Selected")};
    return result;
}

QStringList IconPreviewArea::iconStateNames()
{
    static const QStringList result = {tr("Off"), tr("On")};
    return result;
}
//! [42]

#else // Q_COMPILER_INITIALIZER_LISTS

//! [43]
QVector<QIcon::Mode> IconPreviewArea::iconModes()
{
    static QVector<QIcon::Mode> result;
    if (result.isEmpty())
        result << QIcon::Normal << QIcon::Active << QIcon::Disabled << QIcon::Selected;
    return result;
}
//! [43]

QVector<QIcon::State> IconPreviewArea::iconStates()
{
    static QVector<QIcon::State> result;
    if (result.isEmpty())
        result << QIcon::Off << QIcon::On;
    return result;
}

QStringList IconPreviewArea::iconModeNames()
{
    static QStringList result;
    if (result.isEmpty())
        result << tr("Normal") << tr("Active") << tr("Disabled") << tr("Selected");
    return result;
}

QStringList IconPreviewArea::iconStateNames()
{
    static QStringList result;
    if (result.isEmpty())
        result << tr("Off") << tr("On");
    return result;
}

#endif // !Q_COMPILER_INITIALIZER_LISTS

//! [1]
void IconPreviewArea::setIcon(const QIcon &icon)
{
    this->icon = icon;
    updatePixmapLabels();
}
//! [1]

//! [2]
void IconPreviewArea::setSize(const QSize &size)
{
    if (size != this->size) {
        this->size = size;
        updatePixmapLabels();
    }
}
//! [2]

//! [3]
QLabel *IconPreviewArea::createHeaderLabel(const QString &text)
{
    QLabel *label = new QLabel(tr("<b>%1</b>").arg(text));
    label->setAlignment(Qt::AlignCenter);
    return label;
}
//! [3]

//! [4]
QLabel *IconPreviewArea::createPixmapLabel()
{
    QLabel *label = new QLabel;
    label->setEnabled(false);
    label->setAlignment(Qt::AlignCenter);
    label->setFrameShape(QFrame::Box);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label->setBackgroundRole(QPalette::Base);
    label->setAutoFillBackground(true);
    label->setMinimumSize(132, 132);
    return label;
}
//! [4]

//! [5]
void IconPreviewArea::updatePixmapLabels()
{
    QWindow *window = Q_NULLPTR;
    if (const QWidget *nativeParent = nativeParentWidget())
        window = nativeParent->windowHandle();
    for (int column = 0; column < NumModes; ++column) {
        for (int row = 0; row < NumStates; ++row) {
            const QPixmap pixmap =
                icon.pixmap(window, size, IconPreviewArea::iconModes().at(column),
                            IconPreviewArea::iconStates().at(row));
            QLabel *pixmapLabel = pixmapLabels[column][row];
            pixmapLabel->setPixmap(pixmap);
            pixmapLabel->setEnabled(!pixmap.isNull());
            QString toolTip;
            if (!pixmap.isNull()) {
                const QSize actualSize = icon.actualSize(size);
                toolTip =
                    tr("Size: %1x%2\nActual size: %3x%4\nDevice pixel ratio: %5")
                        .arg(size.width()).arg(size.height())
                        .arg(actualSize.width()).arg(actualSize.height())
                        .arg(pixmap.devicePixelRatioF());
            }
            pixmapLabel->setToolTip(toolTip);
        }
    }
}
//! [5]
