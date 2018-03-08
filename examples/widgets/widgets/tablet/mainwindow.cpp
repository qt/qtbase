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

#include "mainwindow.h"
#include "tabletcanvas.h"

//! [0]
MainWindow::MainWindow(TabletCanvas *canvas)
  : m_canvas(canvas), m_colorDialog(nullptr)
{
    createMenus();
    setWindowTitle(tr("Tablet Example"));
    setCentralWidget(m_canvas);
    QCoreApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
}
//! [0]

//! [1]
void MainWindow::setBrushColor()
{
    if (!m_colorDialog) {
        m_colorDialog = new QColorDialog(this);
        m_colorDialog->setModal(false);
        m_colorDialog->setCurrentColor(m_canvas->color());
        connect(m_colorDialog, &QColorDialog::colorSelected, m_canvas, &TabletCanvas::setColor);
    }
    m_colorDialog->setVisible(true);
}
//! [1]

//! [2]
void MainWindow::setAlphaValuator(QAction *action)
{
    m_canvas->setAlphaChannelValuator(action->data().value<TabletCanvas::Valuator>());
}
//! [2]

//! [3]
void MainWindow::setLineWidthValuator(QAction *action)
{
    m_canvas->setLineWidthType(action->data().value<TabletCanvas::Valuator>());
}
//! [3]

//! [4]
void MainWindow::setSaturationValuator(QAction *action)
{
    m_canvas->setColorSaturationValuator(action->data().value<TabletCanvas::Valuator>());
}
//! [4]

void MainWindow::setEventCompression(bool compress)
{
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents, compress);
}

//! [5]
void MainWindow::save()
{
    QString path = QDir::currentPath() + "/untitled.png";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Picture"),
                             path);

    if (!m_canvas->saveImage(fileName))
        QMessageBox::information(this, "Error Saving Picture",
                                 "Could not save the image");
}
//! [5]

//! [6]
void MainWindow::load()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Picture"),
                                                    QDir::currentPath());

    if (!m_canvas->loadImage(fileName))
        QMessageBox::information(this, "Error Opening Picture",
                                 "Could not open picture");
}
//! [6]

//! [7]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About Tablet Example"),
                       tr("This example shows how to use a graphics drawing tablet in Qt."));
}
//! [7]

//! [8]
void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Open..."), this, &MainWindow::load, QKeySequence::Open);
    fileMenu->addAction(tr("&Save As..."), this, &MainWindow::save, QKeySequence::SaveAs);
    fileMenu->addAction(tr("E&xit"), this, &MainWindow::close, QKeySequence::Quit);

    QMenu *brushMenu = menuBar()->addMenu(tr("&Brush"));
    brushMenu->addAction(tr("&Brush Color..."), this, &MainWindow::setBrushColor, tr("Ctrl+B"));
//! [8]

    QMenu *tabletMenu = menuBar()->addMenu(tr("&Tablet"));
    QMenu *lineWidthMenu = tabletMenu->addMenu(tr("&Line Width"));

    QAction *lineWidthPressureAction = lineWidthMenu->addAction(tr("&Pressure"));
    lineWidthPressureAction->setData(TabletCanvas::PressureValuator);
    lineWidthPressureAction->setCheckable(true);
    lineWidthPressureAction->setChecked(true);

    QAction *lineWidthTiltAction = lineWidthMenu->addAction(tr("&Tilt"));
    lineWidthTiltAction->setData(TabletCanvas::TiltValuator);
    lineWidthTiltAction->setCheckable(true);

    QAction *lineWidthFixedAction = lineWidthMenu->addAction(tr("&Fixed"));
    lineWidthFixedAction->setData(TabletCanvas::NoValuator);
    lineWidthFixedAction->setCheckable(true);

    QActionGroup *lineWidthGroup = new QActionGroup(this);
    lineWidthGroup->addAction(lineWidthPressureAction);
    lineWidthGroup->addAction(lineWidthTiltAction);
    lineWidthGroup->addAction(lineWidthFixedAction);
    connect(lineWidthGroup, &QActionGroup::triggered, this,
            &MainWindow::setLineWidthValuator);

//! [9]
    QMenu *alphaChannelMenu = tabletMenu->addMenu(tr("&Alpha Channel"));
    QAction *alphaChannelPressureAction = alphaChannelMenu->addAction(tr("&Pressure"));
    alphaChannelPressureAction->setData(TabletCanvas::PressureValuator);
    alphaChannelPressureAction->setCheckable(true);

    QAction *alphaChannelTangentialPressureAction = alphaChannelMenu->addAction(tr("T&angential Pressure"));
    alphaChannelTangentialPressureAction->setData(TabletCanvas::TangentialPressureValuator);
    alphaChannelTangentialPressureAction->setCheckable(true);
    alphaChannelTangentialPressureAction->setChecked(true);

    QAction *alphaChannelTiltAction = alphaChannelMenu->addAction(tr("&Tilt"));
    alphaChannelTiltAction->setData(TabletCanvas::TiltValuator);
    alphaChannelTiltAction->setCheckable(true);

    QAction *noAlphaChannelAction = alphaChannelMenu->addAction(tr("No Alpha Channel"));
    noAlphaChannelAction->setData(TabletCanvas::NoValuator);
    noAlphaChannelAction->setCheckable(true);

    QActionGroup *alphaChannelGroup = new QActionGroup(this);
    alphaChannelGroup->addAction(alphaChannelPressureAction);
    alphaChannelGroup->addAction(alphaChannelTangentialPressureAction);
    alphaChannelGroup->addAction(alphaChannelTiltAction);
    alphaChannelGroup->addAction(noAlphaChannelAction);
    connect(alphaChannelGroup, &QActionGroup::triggered,
            this, &MainWindow::setAlphaValuator);
//! [9]

    QMenu *colorSaturationMenu = tabletMenu->addMenu(tr("&Color Saturation"));

    QAction *colorSaturationVTiltAction = colorSaturationMenu->addAction(tr("&Vertical Tilt"));
    colorSaturationVTiltAction->setData(TabletCanvas::VTiltValuator);
    colorSaturationVTiltAction->setCheckable(true);

    QAction *colorSaturationHTiltAction = colorSaturationMenu->addAction(tr("&Horizontal Tilt"));
    colorSaturationHTiltAction->setData(TabletCanvas::HTiltValuator);
    colorSaturationHTiltAction->setCheckable(true);

    QAction *colorSaturationPressureAction = colorSaturationMenu->addAction(tr("&Pressure"));
    colorSaturationPressureAction->setData(TabletCanvas::PressureValuator);
    colorSaturationPressureAction->setCheckable(true);

    QAction *noColorSaturationAction = colorSaturationMenu->addAction(tr("&No Color Saturation"));
    noColorSaturationAction->setData(TabletCanvas::NoValuator);
    noColorSaturationAction->setCheckable(true);
    noColorSaturationAction->setChecked(true);

    QActionGroup *colorSaturationGroup = new QActionGroup(this);
    colorSaturationGroup->addAction(colorSaturationVTiltAction);
    colorSaturationGroup->addAction(colorSaturationHTiltAction);
    colorSaturationGroup->addAction(colorSaturationPressureAction);
    colorSaturationGroup->addAction(noColorSaturationAction);
    connect(colorSaturationGroup, &QActionGroup::triggered,
            this, &MainWindow::setSaturationValuator);

    QAction *compressAction = tabletMenu->addAction(tr("Co&mpress events"));
    compressAction->setCheckable(true);
    connect(compressAction, &QAction::toggled, this, &MainWindow::setEventCompression);

    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(tr("A&bout"), this, &MainWindow::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}
