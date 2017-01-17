/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TRIVISWIDGET_H
#define TRIVISWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QScrollArea>

class TriVisCanvas : public QWidget
{
    Q_OBJECT

public:
    TriVisCanvas(QWidget *parent = nullptr);

    QStringList shapes() const;

    enum Type {
        Stroke,
        Fill
    };

    void setType(Type t) { m_type = t; }
    void setIndex(int idx) { m_idx = idx; }

    void setStrokeWidth(float w) { m_strokeWidth = w; }
    void setDashStroke(bool d) { m_dashStroke = d; }

    void setStepLimits(int strokeLimit, int fillLimit) {
        m_strokeStepLimit = strokeLimit;
        m_fillStepLimit = fillLimit;
        update();
    }

    enum GeomType {
        Triangles,
        TriangleStrips
    };

    QImage preview() const;
    GeomType geomType() const { return m_type == Stroke ? TriangleStrips : Triangles; }
    int vertexCount() const { return m_type == Stroke ? m_strokeVertices.count() : m_fillVertices.count(); }
    int indexCount() const { return m_type == Stroke ? 0 : m_fillIndices.count(); }
    float zoomLevel() const { return m_zoom; }

    void retriangulate();
    void regeneratePreviews();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void retriangulated();
    void zoomChanged(float oldZoom, float newZoom);

private:
    void addPreview(int idx);

    Type m_type = Stroke;
    int m_idx = 0;
    float m_strokeWidth = 1;
    bool m_dashStroke = false;

    QVector<QPainterPath> m_paths;
    QVector<QImage> m_strokePreviews;
    QVector<QImage> m_fillPreviews;

    struct Vertex {
        float x, y;
        void set(float vx, float vy) { x = vx; y = vy; }
    };
    QVector<Vertex> m_fillVertices;
    QVector<quint32> m_fillIndices;
    QVector<Vertex> m_strokeVertices;

    float m_zoom = 1;

    int m_fillStepLimit = 0;
    int m_strokeStepLimit = 0;
};

class TriangulationVisualizer : public QWidget
{
    Q_OBJECT

public:
    TriangulationVisualizer(QWidget *parent = nullptr);

private:
    void updateInfoLabel();
    void updatePreviewLabel();

    QComboBox *m_cbShape;
    QLabel *m_lbPreview;
    QRadioButton *m_rdStroke;
    QRadioButton *m_rdFill;
    QScrollArea *m_scrollArea;
    TriVisCanvas *m_canvas;
    QLabel *m_lbInfo;
    QSpinBox *m_spStrokeWidth;
    QCheckBox *m_chDash;
    QCheckBox *m_chStepEnable;
    QSpinBox *m_spStepStroke;
    QSpinBox *m_spStepFill;
};

#endif
