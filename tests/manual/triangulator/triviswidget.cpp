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

#include "triviswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QWheelEvent>
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtGui/private/qtriangulator_p.h>
#include <QtGui/private/qtriangulatingstroker_p.h>
#include <QDebug>

static const int W = 100;
static const int H = 100;
static const int MAX_ZOOM = 512;

class ScrollArea : public QScrollArea {
protected:
    void wheelEvent(QWheelEvent *event) override {
        if (!event->modifiers().testFlag(Qt::ControlModifier))
            QScrollArea::wheelEvent(event);
    }
};

TriangulationVisualizer::TriangulationVisualizer(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;

    QHBoxLayout *headerLayout = new QHBoxLayout;

    QGroupBox *cbBox = new QGroupBox(QLatin1String("Shape"));
    m_cbShape = new QComboBox;
    QVBoxLayout *cbBoxLayout = new QVBoxLayout;
    cbBoxLayout->addWidget(m_cbShape);
    cbBox->setLayout(cbBoxLayout);
    headerLayout->addWidget(cbBox);

    m_lbPreview = new QLabel;
    m_lbPreview->setFixedSize(W, H);
    headerLayout->addWidget(m_lbPreview);

    QGroupBox *typeBox = new QGroupBox(QLatin1String("Type"));
    m_rdStroke = new QRadioButton(QLatin1String("Stroke"));
    m_rdStroke->setChecked(true);
    m_rdFill = new QRadioButton(QLatin1String("Fill"));
    QVBoxLayout *typeBoxLayout = new QVBoxLayout;
    typeBoxLayout->addWidget(m_rdStroke);
    typeBoxLayout->addWidget(m_rdFill);
    typeBox->setLayout(typeBoxLayout);
    headerLayout->addWidget(typeBox);

    QGroupBox *paramBox = new QGroupBox(QLatin1String("Stroke params"));
    QVBoxLayout *paramBoxLayout = new QVBoxLayout;
    m_spStrokeWidth = new QSpinBox;
    m_spStrokeWidth->setPrefix(QLatin1String("Stroke width: "));
    m_spStrokeWidth->setMinimum(1);
    m_spStrokeWidth->setMaximum(32);
    m_spStrokeWidth->setValue(1);
    m_chDash = new QCheckBox(QLatin1String("Dash stroke"));
    paramBoxLayout->addWidget(m_spStrokeWidth);
    paramBoxLayout->addWidget(m_chDash);
    paramBox->setLayout(paramBoxLayout);
    headerLayout->addWidget(paramBox);

    m_lbInfo = new QLabel;
    headerLayout->addWidget(m_lbInfo);

    QGroupBox *animBox = new QGroupBox(QLatin1String("Step through"));
    QVBoxLayout *animBoxLayout = new QVBoxLayout;
    m_chStepEnable = new QCheckBox(QLatin1String("Enable"));
    m_spStepStroke = new QSpinBox;
    m_spStepStroke->setPrefix(QLatin1String("Stroke steps: "));
    m_spStepStroke->setMinimum(3);
    m_spStepStroke->setMaximum(INT_MAX);
    m_spStepStroke->setEnabled(false);
    m_spStepFill = new QSpinBox;
    m_spStepFill->setPrefix(QLatin1String("Fill steps: "));
    m_spStepFill->setMinimum(3);
    m_spStepFill->setMaximum(INT_MAX);
    m_spStepFill->setEnabled(false);
    animBoxLayout->addWidget(m_chStepEnable);
    animBoxLayout->addWidget(m_spStepStroke);
    animBoxLayout->addWidget(m_spStepFill);
    animBox->setLayout(animBoxLayout);
    headerLayout->addWidget(animBox);

    m_canvas = new TriVisCanvas;
    m_scrollArea = new ScrollArea;
    m_scrollArea->setWidget(m_canvas);
    m_scrollArea->setMinimumSize(W, H);

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_scrollArea);
    mainLayout->setStretchFactor(m_scrollArea, 9);

    setLayout(mainLayout);

    for (const QString &shapeName : m_canvas->shapes())
        m_cbShape->addItem(shapeName);

    connect(m_cbShape, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this]() {
        m_canvas->setIndex(m_cbShape->currentIndex());
        m_canvas->retriangulate();
    });
    connect(m_rdFill, &QRadioButton::toggled, [this]() {
        m_canvas->setType(TriVisCanvas::Fill);
        m_canvas->retriangulate();
    });
    connect(m_rdStroke, &QRadioButton::toggled, [this]() {
        m_canvas->setType(TriVisCanvas::Stroke);
        m_canvas->retriangulate();
    });

    connect(m_spStrokeWidth, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
        m_canvas->setStrokeWidth(m_spStrokeWidth->value());
        m_canvas->regeneratePreviews();
        m_canvas->retriangulate();
    });

    connect(m_chDash, &QCheckBox::toggled, [this]() {
        m_canvas->setDashStroke(m_chDash->isChecked());
        m_canvas->regeneratePreviews();
        m_canvas->retriangulate();
    });

    connect(m_chStepEnable, &QCheckBox::toggled, [this]() {
        bool enable = m_chStepEnable->isChecked();
        m_spStepStroke->setEnabled(enable);
        m_spStepFill->setEnabled(enable);
        if (enable)
            m_canvas->setStepLimits(m_spStepStroke->value(), m_spStepFill->value());
        else
            m_canvas->setStepLimits(0, 0);
    });

    connect(m_spStepStroke, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
        m_canvas->setStepLimits(m_spStepStroke->value(), m_spStepFill->value());
    });

    connect(m_spStepFill, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
        m_canvas->setStepLimits(m_spStepStroke->value(), m_spStepFill->value());
    });

    connect(m_canvas, &TriVisCanvas::retriangulated, [this]() {
        updateInfoLabel();
        updatePreviewLabel();
    });

    connect(m_canvas, &TriVisCanvas::zoomChanged, [this](float oldZoom, float newZoom) {
        QScrollBar *sb = m_scrollArea->horizontalScrollBar();
        float x = sb->value() / oldZoom;
        sb->setValue(x * newZoom);
        sb = m_scrollArea->verticalScrollBar();
        float y = sb->value() / oldZoom;
        sb->setValue(y * newZoom);
        updateInfoLabel();

    });

    m_canvas->retriangulate();
}

void TriangulationVisualizer::updateInfoLabel()
{
    m_lbInfo->setText(QString(QStringLiteral("Type: %1\n%2 vertices (x, y)\n%3 indices\nzoom: %4\nUSE CTRL+WHEEL TO ZOOM"))
                      .arg(m_canvas->geomType() == TriVisCanvas::Triangles ? QLatin1String("Triangles") : QLatin1String("Triangle strips"))
                      .arg(m_canvas->vertexCount())
                      .arg(m_canvas->indexCount())
                      .arg(m_canvas->zoomLevel()));
}

void TriangulationVisualizer::updatePreviewLabel()
{
    m_lbPreview->setPixmap(QPixmap::fromImage(m_canvas->preview()).scaled(m_lbPreview->size()));
}

const int TLX = 10;
const int TLY = 10;

TriVisCanvas::TriVisCanvas(QWidget *parent)
    : QWidget(parent)
{
    resize(W * m_zoom, H * m_zoom);

    QPainterPath linePath;
    linePath.moveTo(TLX, TLY);
    linePath.lineTo(TLX + 30, TLY + 30);
    m_paths << linePath;

    QPainterPath rectPath;
    rectPath.moveTo(TLX, TLY);
    rectPath.lineTo(TLX + 30, TLY);
    rectPath.lineTo(TLX + 30, TLY + 30);
    rectPath.lineTo(TLX, TLY + 30);
    rectPath.lineTo(TLX, TLY);
    m_paths << rectPath;

    QPainterPath roundRectPath;
    roundRectPath.addRoundedRect(TLX, TLY, TLX + 29, TLY + 29, 5, 5);
    m_paths << roundRectPath;

    QPainterPath ellipsePath;
    ellipsePath.addEllipse(TLX, TLY, 40, 20);
    m_paths << ellipsePath;

    QPainterPath cubicPath;
    cubicPath.moveTo(TLX, TLY + 30);
    cubicPath.cubicTo(15, 2, 40, 40, 30, 10);
    m_paths << cubicPath;

    QPainterPath cubicPath2;
    cubicPath2.moveTo(TLX, TLY + 20);
    cubicPath2.cubicTo(15, 2, 30, 30, 30, 35);
    m_paths << cubicPath2;

    regeneratePreviews();
}

QStringList TriVisCanvas::shapes() const
{
    return QStringList()
            << "line"
            << "rect"
            << "roundedrect"
            << "ellipse"
            << "cubic curve 1"
            << "cubic curve 2";
}

void TriVisCanvas::regeneratePreviews()
{
    m_strokePreviews.clear();
    m_fillPreviews.clear();
    for (int i = 0; i < m_paths.count(); ++i)
        addPreview(i);
}

void TriVisCanvas::addPreview(int idx)
{
    QPen pen(Qt::black);
    pen.setWidthF(m_strokeWidth);
    if (m_dashStroke)
        pen.setStyle(Qt::DashLine);

    QImage img(W, H, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.translate(-TLX, -TLY);
    p.scale(2, 2);
    p.strokePath(m_paths[idx], pen);
    p.end();
    m_strokePreviews.append(img);

    img = QImage(W, H, QImage::Format_RGB32);
    img.fill(Qt::white);
    p.begin(&img);
    p.translate(-TLX, -TLY);
    p.scale(2, 2);
    p.fillPath(m_paths[idx], QBrush(Qt::gray));
    p.end();
    m_fillPreviews.append(img);
}

QImage TriVisCanvas::preview() const
{
    if (m_type == Stroke)
        return m_strokePreviews[m_idx];
    else
        return m_fillPreviews[m_idx];
}

static const qreal SCALE = 100;

void TriVisCanvas::retriangulate()
{
    const QPainterPath &path(m_paths[m_idx]);

    if (m_type == Stroke) {
        const QVectorPath &vp = qtVectorPathForPath(path);
        const QSize clipSize(W, H);
        const QRectF clip(QPointF(0, 0), clipSize);
        const qreal inverseScale = 1.0 / SCALE;

        QTriangulatingStroker stroker;
        stroker.setInvScale(inverseScale);

        QPen pen;
        pen.setWidthF(m_strokeWidth);
        if (m_dashStroke)
            pen.setStyle(Qt::DashLine);

        if (pen.style() == Qt::SolidLine) {
            stroker.process(vp, pen, clip, 0);
        } else {
            QDashedStrokeProcessor dashStroker;
            dashStroker.setInvScale(inverseScale);
            dashStroker.process(vp, pen, clip, 0);
            QVectorPath dashStroke(dashStroker.points(), dashStroker.elementCount(),
                                   dashStroker.elementTypes(), 0);
            stroker.process(dashStroke, pen, clip, 0);
        }

        m_strokeVertices.resize(stroker.vertexCount() / 2);
        if (!m_strokeVertices.isEmpty()) {
            const float *vsrc = stroker.vertices();
            for (int i = 0; i < m_strokeVertices.count(); ++i)
                m_strokeVertices[i].set(vsrc[i * 2], vsrc[i * 2 + 1]);
        }
    } else {
        const QVectorPath &vp = qtVectorPathForPath(path);
        QTriangleSet ts = qTriangulate(vp, QTransform::fromScale(SCALE, SCALE), 1, true);
        const int vertexCount = ts.vertices.count() / 2;
        m_fillVertices.resize(vertexCount);
        Vertex *vdst = reinterpret_cast<Vertex *>(m_fillVertices.data());
        const qreal *vsrc = ts.vertices.constData();
        for (int i = 0; i < vertexCount; ++i)
            vdst[i].set(vsrc[i * 2] / SCALE, vsrc[i * 2 + 1] / SCALE);

        m_fillIndices.resize(ts.indices.size());
        if (ts.indices.type() == QVertexIndexVector::UnsignedShort) {
            const quint16 *shortD = static_cast<const quint16 *>(ts.indices.data());
            for (int i = 0; i < m_fillIndices.count(); ++i)
                m_fillIndices[i] = shortD[i];
        } else {
            memcpy(m_fillIndices.data(), ts.indices.data(), ts.indices.size() * sizeof(quint32));
        }
    }

    emit retriangulated();
    update();
}

void TriVisCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    if (m_type == Stroke) {
        QPointF prevPt[3];
        int cnt = 0;
        for (int i = 0; i < m_strokeVertices.count() && (!m_strokeStepLimit || i < m_strokeStepLimit); ++i) {
            auto &v = m_strokeVertices[i];
            QPointF pt(v.x, v.y);
            pt *= m_zoom;
            if (cnt == 1 || cnt == 2)
                p.drawLine(prevPt[cnt - 1], pt);
            prevPt[cnt] = pt;
            cnt = (cnt + 1) % 3;
            if (!cnt) {
                p.drawLine(pt, prevPt[cnt]);
                i -= 2;
            }
        }
    } else {
        QPointF prevPt[3];
        int cnt = 0;
        for (int i = 0; i < m_fillIndices.count() && (!m_fillStepLimit || i < m_fillStepLimit); ++i) {
            auto &v = m_fillVertices[m_fillIndices[i]];
            QPointF pt(v.x, v.y);
            pt *= m_zoom;
            if (cnt == 1 || cnt == 2)
                p.drawLine(prevPt[cnt - 1], pt);
            prevPt[cnt] = pt;
            cnt = (cnt + 1) % 3;
            if (!cnt)
                p.drawLine(pt, prevPt[cnt]);
        }
    }
}

void TriVisCanvas::wheelEvent(QWheelEvent *event)
{
    int change = 0;

    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->delta() > 0 && m_zoom < MAX_ZOOM) {
            m_zoom += 1;
            change = 1;
        } else if (event->delta() < 0 && m_zoom > 1) {
            m_zoom -= 1;
            change = -1;
        }
    }

    resize(W * m_zoom, H * m_zoom);
    emit zoomChanged(m_zoom - change, m_zoom);
    update();
}
