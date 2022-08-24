// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "singleglyphrun.h"
#include "ui_singleglyphrun.h"

SingleGlyphRun::SingleGlyphRun(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SingleGlyphRun)
{
    ui->setupUi(this);
}

SingleGlyphRun::~SingleGlyphRun()
{
    delete ui;
}

void SingleGlyphRun::updateGlyphRun(const QGlyphRun &glyphRun)
{
    m_bounds = QRegion();

    QList<quint32> glyphIndexes = glyphRun.glyphIndexes();
    QList<qsizetype> stringIndexes = glyphRun.stringIndexes();
    QList<QPointF> glyphPositions = glyphRun.positions();

    ui->twGlyphRun->clearContents();
    ui->twGlyphRun->setRowCount(glyphIndexes.size());

    for (int i = 0; i < glyphIndexes.size(); ++i) {
        {
            QTableWidgetItem *glyphIndex = new QTableWidgetItem(QString::number(glyphIndexes.at(i)));
            ui->twGlyphRun->setItem(i, 0, glyphIndex);
        }

        {
            QPointF position = glyphPositions.at(i);
            QTableWidgetItem *glyphPosition = new QTableWidgetItem(QStringLiteral("(%1, %2)")
                                                                   .arg(position.x())
                                                                   .arg(position.y()));
            ui->twGlyphRun->setItem(i, 1, glyphPosition);
        }

        {
            QTableWidgetItem *stringIndex = new QTableWidgetItem(QString::number(stringIndexes.at(i)));
            ui->twGlyphRun->setItem(i, 2, stringIndex);
        }

        QChar c = glyphRun.sourceString().at(stringIndexes.at(i));

        {
            QTableWidgetItem *unicode = new QTableWidgetItem(QString::number(c.unicode(), 16));
            ui->twGlyphRun->setItem(i, 3, unicode);
        }

        {
            QTableWidgetItem *character = new QTableWidgetItem(c);
            ui->twGlyphRun->setItem(i, 4, character);
        }

        {
            QImage image = glyphRun.rawFont().alphaMapForGlyph(glyphIndexes.at(i));

            QTableWidgetItem *glyphImage = new QTableWidgetItem(QIcon(QPixmap::fromImage(image)), QString{});
            ui->twGlyphRun->setItem(i, 5, glyphImage);
        }

        QRectF brect = glyphRun.rawFont().boundingRect(glyphIndexes.at(i));
        brect.adjust(glyphPositions.at(i).x(), glyphPositions.at(i).y(),
                     glyphPositions.at(i).x(), glyphPositions.at(i).y());
        m_bounds += brect.toAlignedRect();
    }
}
