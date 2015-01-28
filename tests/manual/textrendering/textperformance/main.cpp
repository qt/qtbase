/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QDialog>
#include <QFontDatabase>
#include <QPainter>
#include <QTime>
#include <QTimer>

static const int lastMeasurementsCount = 50;

class FontBlaster: public QWidget
{
    Q_OBJECT

public:
    FontBlaster(QWidget *parent = 0)
        : QWidget(parent)
        , m_currentMode(0)
    {
        setFocusPolicy(Qt::StrongFocus);
    }

    void paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event);
        QPainter p(this);

        if (!m_timer.isNull())
            m_lastMeasurements.append(m_timer.elapsed());
        m_timer.start();

        p.save();
        m_modes[m_currentMode].function(p, size());
        p.restore();

        const QFontMetrics fm = p.fontMetrics();
        p.setOpacity(0.7);
        p.fillRect(0, 0, width(), fm.height(), Qt::gray);
        p.fillRect(0, height() - fm.height(), width(), height(), Qt::gray);
        p.setOpacity(1);
        p.setPen(palette().color(QPalette::Text));
        p.drawText(2, fm.ascent(), m_modes[m_currentMode].name);

        if (m_lastMeasurements.count() == lastMeasurementsCount) {
            m_lastMeasurements.removeFirst();
            int lastMsecsSum = 0;
            foreach(const int measurement, m_lastMeasurements)
                lastMsecsSum += measurement;

            p.drawText(2, height() - fm.descent(),
                QLatin1String("Fps: ") +
                QString::number(1000 / ((qreal)lastMsecsSum / lastMeasurementsCount), 'f', 1)
            );
        }

        QTimer::singleShot(0, this, SLOT(repaint()));
    }

    /*
      Creating all kinds of size/weight/italic combinations, stress testing
      the glyph cache.
      Also: painting with different opacities, stress testing blitting.
    */
    static void paintDifferentFontStyles(QPainter &p, const QSize &size)
    {
        static const QString text = QLatin1String("Qt rocks!!!");
        static const int textsPerPaint = 30;
        for (int i = 0; i < textsPerPaint; i++) {
            const int fontSize = 4 + (qrand() % 5);
            const int fontWeight = (qrand() % 2) == 1 ? QFont::Normal : QFont::Bold;
            const bool fontItalic = (qrand() % 2) == 1;
            const QFont font("Default", fontSize, fontWeight, fontItalic);
            p.setFont(font);
            p.setPen(QColor::fromHsv(qrand() % 359, 155 + qrand() % 100,
                                     155 + qrand() % 100, 100 + qrand() % 155));
            const QSize textSize(p.fontMetrics().boundingRect(text).size());
            const QPoint position(
                -textSize.width() / 2 + (qrand() % size.width()),
                textSize.height() / 2 + (qrand() % size.height()));
            p.drawText(position, text);
        }
    }

    /*
      Drawing a multiline latin text, stress testing the text layout system.
    */
    static void paintLongLatinText(QPainter &p, const QSize &size)
    {
        static const char* const pieces[] = {
            "lorem ipsum",
            "dolor sit amet",
            "consectetuer",
            "sed diam nonumy",
            "eos et accusam",
            "sea takimata sanctus"
        };
        static const int piecesCount = (int)(sizeof pieces / sizeof pieces[0]);
        static const int piecesPerPaint = 30;

        QString text;
        for (int i = 0; i < piecesPerPaint; ++i) {
            QString piece = QLatin1String(pieces[qrand() % piecesCount]);
            if (i == 0 || qrand() % 2) {
                // Make this piece the beginning of a new sentence.
                piece[0] = piece[0].toUpper();
                if (i > 0)
                    piece.prepend(QLatin1String(". "));
            } else {
                piece.prepend(QLatin1String(", "));
            }
            text.append(piece);
        }
        text.append(QLatin1Char('.'));

        p.drawText(QRectF(QPointF(0, 0), QSizeF(size)),
                   Qt::AlignTop | Qt::AlignAbsolute | Qt::TextWordWrap, text);
    }

    /*
      Drawing one text with several snippets of different writingSystems, stress
      testing the font merging in the font database.
    */
    static void paintInternationalText(QPainter &p, const QSize &size)
    {
        static QStringList samples;
        if (samples.isEmpty()) {
            foreach (const QFontDatabase::WritingSystem system, QFontDatabase().writingSystems())
                if (system != QFontDatabase::Ogham && system != QFontDatabase::Runic)
                    samples.append(QFontDatabase::writingSystemSample(system));
        }
        static const int systemsPerPaint = 65;
        QString text;
        for (int i = 0; i < systemsPerPaint; i++) {
            if (i > 0)
                text.append(QLatin1Char(' '));
            text.append(samples.at(qrand() % samples.count()));
        }
        p.drawText(QRectF(QPointF(0, 0), QSizeF(size)),
                   Qt::AlignTop | Qt::AlignAbsolute | Qt::TextWordWrap, text);
    }

protected:
    void nextMode()
    {
        m_currentMode = (m_currentMode + 1) % m_modesCount;
        m_lastMeasurements.clear();
    }

    void keyPressEvent(QKeyEvent *event)
    {
        Q_UNUSED(event);
        nextMode();
    }

    void mousePressEvent(QMouseEvent *event)
    {
        Q_UNUSED(event);
        nextMode();
    }

private:
    static const struct mode {
        QString name;
        void (*function)(QPainter &, const QSize&);
    } m_modes[];
    static const int m_modesCount;

    int m_currentMode;
    QList<int> m_lastMeasurements;
    QTime m_timer;
};

const struct FontBlaster::mode FontBlaster::m_modes[] = {
    { QLatin1String("Qt rocks!!!"), FontBlaster::paintDifferentFontStyles },
    { QLatin1String("Latin"), FontBlaster::paintLongLatinText },
    { QLatin1String("International"), FontBlaster::paintInternationalText }
};

const int FontBlaster::m_modesCount =
    (int)(sizeof m_modes / sizeof m_modes[0]);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FontBlaster dlg;
    dlg.show();

    return a.exec();
}

#include "main.moc"
