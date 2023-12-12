// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QOpenGLFunctions>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPicture>
#include <QRectF>
#include <QWidget>


namespace src_gui_painting_qpainter {
struct SimpleExampleWidget : public QPaintDevice {
    void paintEvent(QPaintEvent *);
    QRect rect();
};
struct MyWidget : public QWidget
{
    void paintEvent(QPaintEvent *);
};
QLine drawingCode;


//! [0]
void SimpleExampleWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Qt::blue);
    painter.setFont(QFont("Arial", 30));
    painter.drawText(rect(), Qt::AlignCenter, "Qt");
}
//! [0]


//! [1]
void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter p;
    p.begin(this);
    p.drawLine(drawingCode);        // drawing code
    p.end();
}
//! [1]

} // src_gui_painting_qpainter
namespace src_gui_painting_qpainter2 {
struct MyWidget : public QWidget
{
    void paintEvent(QPaintEvent *);
    int background() { return 0; }
    void wrapper1();
    void wrapper2();
    void wrapper3();
    void wrapper4();
    void wrapper5();
    void wrapper6();
    void wrapper7();
    void wrapper8();
    void wrapper9();
    void wrapper10();
    void wrapper11();
    void wrapper12();
    void wrapper13();
    void wrapper14();
    void wrapper15();
    void concentricCircles();
};
QLine drawingCode;

//! [2]
void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawLine(drawingCode);        // drawing code
}
//! [2]


void wrapper0() {
QPainter *painter = nullptr;
QPainter *painter2 = nullptr;
QPaintDevice *myWidget = nullptr;
//! [3]
painter->begin(0); // impossible - paint device cannot be 0

QPixmap image(0, 0);
painter->begin(&image); // impossible - image.isNull() == true;

painter->begin(myWidget);
painter2->begin(myWidget); // impossible - only one painter at a time
//! [3]

} // wrapper0

namespace QPainterWrapper {
struct QPainter {
    void rotate(qreal angle);
    void setWorldTransform(QTransform matrix, bool);
};

} // QPainterWrapper

void MyWidget::wrapper1() {
//! [5]
QPainterPath path;
path.moveTo(20, 80);
path.lineTo(20, 30);
path.cubicTo(80, 0, 50, 50, 80, 80);

QPainter painter(this);
painter.drawPath(path);
//! [5]


//! [6]
QLineF line(10.0, 80.0, 90.0, 20.0);

QPainter painter(this);
painter.drawLine(line);
//! [6]
} // MyWidget::wrapper1()


void MyWidget::wrapper2() {

//! [7]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);

QPainter painter(this);
painter.drawRect(rectangle);
//! [7]

} // MyWidget::wrapper2


void MyWidget::wrapper3() {

//! [8]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);

QPainter painter(this);
painter.drawRoundedRect(rectangle, 20.0, 15.0);
//! [8]

} // MyWidget::wrapper3


void MyWidget::wrapper4() {

//! [9]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);

QPainter painter(this);
painter.drawEllipse(rectangle);
//! [9]

} // MyWidget::wrapper4


void MyWidget::wrapper5() {

//! [10]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);
int startAngle = 30 * 16;
int spanAngle = 120 * 16;

QPainter painter(this);
painter.drawArc(rectangle, startAngle, spanAngle);
//! [10]

} // MyWidget::wrapper5


void MyWidget::wrapper6() {

//! [11]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);
int startAngle = 30 * 16;
int spanAngle = 120 * 16;

QPainter painter(this);
painter.drawPie(rectangle, startAngle, spanAngle);
//! [11]

} // MyWidget::wrapper6


void MyWidget::wrapper7() {
QRect rect;

//! [12]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);
int startAngle = 30 * 16;
int spanAngle = 120 * 16;

QPainter painter(this);
painter.drawChord(rect, startAngle, spanAngle);
//! [12]
Q_UNUSED(rectangle);
} // MyWidget::wrapper7


void MyWidget::wrapper8() {

//! [13]
static const QPointF points[3] = {
    QPointF(10.0, 80.0),
    QPointF(20.0, 10.0),
    QPointF(80.0, 30.0),
};

QPainter painter(this);
painter.drawPolyline(points, 3);
//! [13]

} // MyWidget::wrapper8


void MyWidget::wrapper9() {
//! [14]
static const QPointF points[4] = {
    QPointF(10.0, 80.0),
    QPointF(20.0, 10.0),
    QPointF(80.0, 30.0),
    QPointF(90.0, 70.0)
};

QPainter painter(this);
painter.drawPolygon(points, 4);
//! [14]

} // MyWidget::wrapper9


void MyWidget::wrapper10() {

//! [15]
static const QPointF points[4] = {
    QPointF(10.0, 80.0),
    QPointF(20.0, 10.0),
    QPointF(80.0, 30.0),
    QPointF(90.0, 70.0)
};

QPainter painter(this);
painter.drawConvexPolygon(points, 4);
//! [15]


//! [16]
QRectF target(10.0, 20.0, 80.0, 60.0);
QRectF source(0.0, 0.0, 70.0, 40.0);
QPixmap pixmap(":myPixmap.png");

QPainter painter(this);
painter.drawPixmap(target, pixmap, source);
//! [16]

} // MyWidget::wrapper10


void MyWidget::wrapper11() {
QRect rect;

//! [17]
QPainter painter(this);
painter.drawText(rect, Qt::AlignCenter, tr("Qt\nProject"));
//! [17]

} // MyWidget::wrapper11


QRectF fillRect(QRect rect, int background) {
    Q_UNUSED(rect);
    Q_UNUSED(background);
    return QRectF();
};
void MyWidget::wrapper12() {
QRect rectangle;

//! [18]
QPicture picture;
QPointF point(10.0, 20.0);
picture.load("drawing.pic");

QPainter painter(this);
painter.drawPicture(0, 0, picture);
//! [18]

Q_UNUSED(point);


//! [19]
fillRect(rectangle, background());
//! [19]

} // MyWidget::wrapper12


void MyWidget::wrapper13() {

//! [20]
QRectF target(10.0, 20.0, 80.0, 60.0);
QRectF source(0.0, 0.0, 70.0, 40.0);
QImage image(":/images/myImage.png");

QPainter painter(this);
painter.drawImage(target, image, source);
//! [20]

} // MyWidget::wrapper13


void MyWidget::wrapper14() {

//! [21]
QPainter painter(this);
painter.fillRect(0, 0, 128, 128, Qt::green);
painter.beginNativePainting();

glEnable(GL_SCISSOR_TEST);
glScissor(0, 0, 64, 64);

glClearColor(1, 0, 0, 1);
glClear(GL_COLOR_BUFFER_BIT);

glDisable(GL_SCISSOR_TEST);

painter.endNativePainting();
//! [21]

} // MyWidget::wrapper14


void MyWidget::wrapper15() {

//! [drawText]
QPainter painter(this);
QFont font = painter.font();
font.setPixelSize(48);
painter.setFont(font);

const QRect rectangle = QRect(0, 0, 100, 50);
QRect boundingRect;
painter.drawText(rectangle, 0, tr("Hello"), &boundingRect);

QPen pen = painter.pen();
pen.setStyle(Qt::DotLine);
painter.setPen(pen);
painter.drawRect(boundingRect.adjusted(0, 0, -pen.width(), -pen.width()));

pen.setStyle(Qt::DashLine);
painter.setPen(pen);
painter.drawRect(rectangle.adjusted(0, 0, -pen.width(), -pen.width()));
//! [drawText]


} // MyWidget::wrapper15

void MyWidget::concentricCircles()
{
//! [renderHint]
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
//! [renderHint]
    int diameter = 50;
//! [floatBased]
    painter.drawEllipse(QRectF(-diameter / 2.0, -diameter / 2.0, diameter, diameter));
//! [floatBased]
//! [intBased]
    painter.drawEllipse(QRect(-diameter / 2, -diameter / 2, diameter, diameter));
//! [intBased]

} // MyWidget::concentricCircles

} // src_gui_painting_qpainter2