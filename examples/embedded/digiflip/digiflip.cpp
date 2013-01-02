/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtWidgets>

class Digits: public QWidget
{
    Q_OBJECT

public:

    enum {
        Slide,
        Flip,
        Rotate
    };

    Digits(QWidget *parent)
        : QWidget(parent)
        , m_number(0)
        , m_transition(Slide)
    {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        connect(&m_animator, SIGNAL(frameChanged(int)), SLOT(update()));
        m_animator.setFrameRange(0, 100);
        m_animator.setDuration(600);
        m_animator.setCurveShape(QTimeLine::EaseInOutCurve);
    }

    void setTransition(int tr) {
        m_transition = tr;
    }

    int transition() const {
        return m_transition;
    }

    void setNumber(int n) {
        if (m_number != n) {
            m_number = qBound(0, n, 99);
            preparePixmap();
            update();
        }
    }

    void flipTo(int n) {
        if (m_number != n) {
            m_number = qBound(0, n, 99);
            m_lastPixmap = m_pixmap;
            preparePixmap();
            m_animator.stop();
            m_animator.start();
        }
    }

protected:

    void drawFrame(QPainter *p, const QRect &rect) {
        p->setPen(Qt::NoPen);
        QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
        gradient.setColorAt(0.00, QColor(245, 245, 245));
        gradient.setColorAt(0.49, QColor(192, 192, 192));
        gradient.setColorAt(0.51, QColor(245, 245, 245));
        gradient.setColorAt(1.00, QColor(192, 192, 192));
        p->setBrush(gradient);
        QRect r = rect;
        p->drawRoundedRect(r, 15, 15, Qt::RelativeSize);
        r.adjust(1, 4, -1, -4);
        p->setPen(QColor(181, 181, 181));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(r, 15, 15, Qt::RelativeSize);
        p->setPen(QColor(159, 159, 159));
        int y = rect.top() + rect.height() / 2 - 1;
        p->drawLine(rect.left(), y, rect.right(), y);
    }

    QPixmap drawDigits(int n, const QRect &rect) {

        int scaleFactor = 2;

        QString str = QString::number(n);
        if (str.length() == 1)
            str.prepend("0");

        QFont font;
        font.setFamily("Helvetica");
        int fontHeight = scaleFactor * 0.55 * rect.height();
        font.setPixelSize(fontHeight);
        font.setBold(true);

        QPixmap pixmap(rect.size() * scaleFactor);
        pixmap.fill(Qt::transparent);

        QLinearGradient gradient(QPoint(0, 0), QPoint(0, pixmap.height()));
        gradient.setColorAt(0.00, QColor(128, 128, 128));
        gradient.setColorAt(0.49, QColor(64, 64, 64));
        gradient.setColorAt(0.51, QColor(128, 128, 128));
        gradient.setColorAt(1.00, QColor(16, 16, 16));

        QPainter p;
        p.begin(&pixmap);
        p.setFont(font);
        QPen pen;
        pen.setBrush(QBrush(gradient));
        p.setPen(pen);
        p.drawText(pixmap.rect(), Qt::AlignCenter, str);
        p.end();

        return pixmap.scaledToWidth(width(), Qt::SmoothTransformation);
    }

    void preparePixmap() {
        m_pixmap = QPixmap(size());
        m_pixmap.fill(Qt::transparent);
        QPainter p;
        p.begin(&m_pixmap);
        p.drawPixmap(0, 0, drawDigits(m_number, rect()));
        p.end();
    }

    void resizeEvent(QResizeEvent*) {
        preparePixmap();
        update();
    }

    void paintStatic() {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);

        int pad = width() / 10;
        drawFrame(&p, rect().adjusted(pad, pad, -pad, -pad));
        p.drawPixmap(0, 0, m_pixmap);
    }

    void paintSlide() {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);

        int pad = width() / 10;
        QRect fr = rect().adjusted(pad, pad, -pad, -pad);
        drawFrame(&p, fr);
        p.setClipRect(fr);

        int y = height() * m_animator.currentFrame() / 100;
        p.drawPixmap(0, y, m_lastPixmap);
        p.drawPixmap(0, y - height(), m_pixmap);
    }

    void paintFlip() {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), Qt::black);

        int hw = width() / 2;
        int hh = height() / 2;

        // behind is the new pixmap
        int pad = width() / 10;
        QRect fr = rect().adjusted(pad, pad, -pad, -pad);
        drawFrame(&p, fr);
        p.drawPixmap(0, 0, m_pixmap);

        int index = m_animator.currentFrame();

        if (index <= 50) {

            // the top part of the old pixmap is flipping
            int angle = -180 * index / 100;
            QTransform transform;
            transform.translate(hw, hh);
            transform.rotate(angle, Qt::XAxis);
            p.setTransform(transform);
            drawFrame(&p, fr.adjusted(-hw, -hh, -hw, -hh));
            p.drawPixmap(-hw, -hh, m_lastPixmap);

            // the bottom part is still the old pixmap
            p.resetTransform();
            p.setClipRect(0, hh, width(), hh);
            drawFrame(&p, fr);
            p.drawPixmap(0, 0, m_lastPixmap);
        } else {

            p.setClipRect(0, hh, width(), hh);

            // the bottom part is still the old pixmap
            drawFrame(&p, fr);
            p.drawPixmap(0, 0, m_lastPixmap);

            // the bottom part of the new pixmap is flipping
            int angle = 180 - 180 * m_animator.currentFrame() / 100;
            QTransform transform;
            transform.translate(hw, hh);
            transform.rotate(angle, Qt::XAxis);
            p.setTransform(transform);
            drawFrame(&p, fr.adjusted(-hw, -hh, -hw, -hh));
            p.drawPixmap(-hw, -hh, m_pixmap);

        }

    }

    void paintRotate() {
        QPainter p(this);

        int pad = width() / 10;
        QRect fr = rect().adjusted(pad, pad, -pad, -pad);
        drawFrame(&p, fr);
        p.setClipRect(fr);

        int angle1 = -180 * m_animator.currentFrame() / 100;
        int angle2 = 180 - 180 * m_animator.currentFrame() / 100;
        int angle = (m_animator.currentFrame() <= 50) ? angle1 : angle2;
        QPixmap pix = (m_animator.currentFrame() <= 50) ? m_lastPixmap : m_pixmap;

        QTransform transform;
        transform.translate(width() / 2, height() / 2);
        transform.rotate(angle, Qt::XAxis);

        p.setTransform(transform);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(-width() / 2, -height() / 2, pix);
    }

    void paintEvent(QPaintEvent *event) {
        Q_UNUSED(event);
        if (m_animator.state() == QTimeLine::Running) {
            if (m_transition == Slide)
                paintSlide();
            if (m_transition == Flip)
                paintFlip();
            if (m_transition == Rotate)
                paintRotate();
        } else {
            paintStatic();
        }
    }

private:
    int m_number;
    int m_transition;
    QPixmap m_pixmap;
    QPixmap m_lastPixmap;
    QTimeLine m_animator;
};

class DigiFlip : public QMainWindow
{
    Q_OBJECT

public:
    DigiFlip(QWidget *parent = 0)
        : QMainWindow(parent)
    {
        m_hour = new Digits(this);
        m_hour->show();
        m_minute = new Digits(this);
        m_minute->show();

        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::black);
        setPalette(pal);

        m_ticker.start(1000, this);
        QTime t = QTime::currentTime();
        m_hour->setNumber(t.hour());
        m_minute->setNumber(t.minute());
        updateTime();

        QAction *slideAction = new QAction("&Slide", this);
        QAction *flipAction = new QAction("&Flip", this);
        QAction *rotateAction = new QAction("&Rotate", this);
        connect(slideAction, SIGNAL(triggered()), SLOT(chooseSlide()));
        connect(flipAction, SIGNAL(triggered()), SLOT(chooseFlip()));
        connect(rotateAction, SIGNAL(triggered()), SLOT(chooseRotate()));
        addAction(slideAction);
        addAction(flipAction);
        addAction(rotateAction);
        setContextMenuPolicy(Qt::ActionsContextMenu);
    }

    void updateTime() {
        QTime t = QTime::currentTime();
        m_hour->flipTo(t.hour());
        m_minute->flipTo(t.minute());
        QString str = t.toString("hh:mm:ss");
        str.prepend(": ");
        if (m_hour->transition() == Digits::Slide)
            str.prepend("Slide");
        if (m_hour->transition() == Digits::Flip)
            str.prepend("Flip");
        if (m_hour->transition() == Digits::Rotate)
            str.prepend("Rotate");
        setWindowTitle(str);
    }

    void switchTransition(int delta) {
        int i = (m_hour->transition() + delta + 3) % 3;
        m_hour->setTransition(i);
        m_minute->setTransition(i);
        updateTime();
    }

protected:
    void resizeEvent(QResizeEvent*) {
        int digitsWidth = width() / 2;
        int digitsHeight = digitsWidth * 1.2;

        int y = (height() - digitsHeight) / 3;

        m_hour->resize(digitsWidth, digitsHeight);
        m_hour->move(0, y);

        m_minute->resize(digitsWidth, digitsHeight);
        m_minute->move(width() / 2, y);
    }

    void timerEvent(QTimerEvent*) {
        updateTime();
    }

    void keyPressEvent(QKeyEvent *event) {
        if (event->key() == Qt::Key_Right) {
            switchTransition(1);
            event->accept();
        }
        if (event->key() == Qt::Key_Left) {
            switchTransition(-1);
            event->accept();
        }
    }

private slots:
    void chooseSlide() {
        m_hour->setTransition(0);
        m_minute->setTransition(0);
        updateTime();
    }

    void chooseFlip() {
        m_hour->setTransition(1);
        m_minute->setTransition(1);
        updateTime();
    }

    void chooseRotate() {
        m_hour->setTransition(2);
        m_minute->setTransition(2);
        updateTime();
    }

private:
    QBasicTimer m_ticker;
    Digits *m_hour;
    Digits *m_minute;
};

#include "digiflip.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DigiFlip time;
    time.resize(320, 240);
    time.show();

    return app.exec();
}
