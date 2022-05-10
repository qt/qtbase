// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "tabletcanvas.h"

#include <QActionGroup>
#include <QApplication>
#include <QColorDialog>
#include <QDir>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>

//! [0]
MainWindow::MainWindow(TabletCanvas *canvas)
    : m_canvas(canvas)
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
    m_canvas->setAlphaChannelValuator(qvariant_cast<TabletCanvas::Valuator>(action->data()));
}
//! [2]

//! [3]
void MainWindow::setLineWidthValuator(QAction *action)
{
    m_canvas->setLineWidthType(qvariant_cast<TabletCanvas::Valuator>(action->data()));
}
//! [3]

//! [4]
void MainWindow::setSaturationValuator(QAction *action)
{
    m_canvas->setColorSaturationValuator(qvariant_cast<TabletCanvas::Valuator>(action->data()));
}
//! [4]

void MainWindow::setEventCompression(bool compress)
{
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents, compress);
}

//! [5]
bool MainWindow::save()
{
    QString path = QDir::currentPath() + "/untitled.png";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Picture"),
                             path);
    bool success = m_canvas->saveImage(fileName);
    if (!success)
        QMessageBox::information(this, "Error Saving Picture",
                                 "Could not save the image");
    return success;
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

void MainWindow::clear()
{
    if (QMessageBox::question(this, tr("Save changes"), tr("Do you want to save your changes?"),
                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                              QMessageBox::Save) != QMessageBox::Save || save())
        m_canvas->clear();
}

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
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::load);
    fileMenu->addAction(tr("&Save As..."), QKeySequence::SaveAs, this, &MainWindow::save);
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::clear);
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, this, &MainWindow::close);

    QMenu *brushMenu = menuBar()->addMenu(tr("&Brush"));
    brushMenu->addAction(tr("&Brush Color..."), tr("Ctrl+B"), this, &MainWindow::setBrushColor);
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
