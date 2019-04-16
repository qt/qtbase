/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QtWidgets>

#ifdef Q_OS_DARWIN
#include <private/qcoregraphics_p.h>
#include <private/qcore_mac_p.h>
#include <Foundation/Foundation.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#endif

static int s_mode;
static QString s_text = QString::fromUtf8("The quick brown \xF0\x9F\xA6\x8A jumps over the lazy \xF0\x9F\x90\xB6");

class TextRenderer : public QWidget
{
    Q_OBJECT
public:
    enum RenderingMode { QtRendering, NativeRendering };
    Q_ENUM(RenderingMode);

    TextRenderer(qreal pointSize, const QString &text, const QColor &textColor = QColor(), const QColor &bgColor = QColor())
        : m_text(text)
    {
        if (pointSize) {
            QFont f = font();
            f.setPointSize(pointSize);
            setFont(f);
        }

        if (textColor.isValid()) {
            QPalette p = palette();
            p.setColor(QPalette::Text, textColor);
            setPalette(p);
        }

        if (bgColor.isValid()) {
            QPalette p = palette();
            p.setColor(QPalette::Window, bgColor);
            setPalette(p);
        }
    }

    QString text() const
    {
        return !m_text.isNull() ? m_text : s_text;
    }

    QSize sizeHint() const override
    {
        QFontMetrics fm = fontMetrics();
        return QSize(fm.boundingRect(text()).width(), fm.height());
    }

    bool event(QEvent * event) override
    {
        if (event->type() == QEvent::ToolTip) {
            QString toolTip;
            QDebug debug(&toolTip);
            debug << "textColor =" << palette().color(QPalette::Text) << "bgColor =" << palette().color(QPalette::Window);
            setToolTip(toolTip);
        }

        return QWidget::event(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QImage image(size() * devicePixelRatio(), QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(devicePixelRatio());

        QPainter p(&image);
        p.fillRect(QRect(0, 0, image.width(), image.height()), palette().window().color());

        const int ascent = fontMetrics().ascent();

        QPen metricsPen(QColor(112, 216, 255), 1.0);
        metricsPen.setCosmetic(true);
        p.setPen(metricsPen);
        p.drawLine(QPoint(0, ascent), QPoint(width(), ascent));
        p.end();

        if (s_mode == QtRendering)
            renderQtText(image);
        else
            renderNativeText(image);

        QPainter wp(this);
        wp.drawImage(QPoint(0, 0), image);
    }

    void renderQtText(QImage &image)
    {
        QPainter p(&image);

        const int ascent = fontMetrics().ascent();

        p.setPen(palette().text().color());

        QFont f = font();
        f.resolve(-1);
        p.setFont(f);

        p.drawText(QPoint(0, ascent), text());
    }

    void renderNativeText(QImage &image)
    {
#ifdef Q_OS_DARWIN
        QMacAutoReleasePool pool;
        QMacCGContext ctx(&image);

        const auto *fontEngine = QFontPrivate::get(font())->engineForScript(QChar::Script_Common);
        Q_ASSERT(fontEngine);
        if (fontEngine->type() == QFontEngine::Multi) {
            fontEngine = static_cast<const QFontEngineMulti *>(fontEngine)->engine(0);
            Q_ASSERT(fontEngine);
        }
        Q_ASSERT(fontEngine->type() == QFontEngine::Mac);

        QColor textColor = palette().text().color();
        auto nsColor = [NSColor colorWithSRGBRed:textColor.redF()
            green:textColor.greenF()
            blue:textColor.blueF()
            alpha:textColor.alphaF()];

        if (font().styleStrategy() & QFont::NoAntialias)
            CGContextSetShouldAntialias(ctx, false);

        // Flip to what CT expects
        CGContextScaleCTM(ctx, 1, -1);
        CGContextTranslateCTM(ctx, 0, -height());

        // Set up baseline
        CGContextSetTextPosition(ctx, 0, height() - fontMetrics().ascent());

        auto *attributedString = [[NSAttributedString alloc] initWithString:text().toNSString()
            attributes:@{
                NSFontAttributeName : (NSFont *)fontEngine->handle(),
                NSForegroundColorAttributeName : nsColor
            }
        ];

        QCFType<CTLineRef> line = CTLineCreateWithAttributedString(CFAttributedStringRef([attributedString autorelease]));
        CTLineDraw(line, ctx);
#endif
    }

public:

    RenderingMode m_mode = QtRendering;
    QString m_text;
};

class TestWidget : public QWidget
{
    Q_OBJECT
public:
    TestWidget()
    {
        auto *mainLayout = new QVBoxLayout;

        m_previews = new QWidget;
        m_previews->setLayout(new QHBoxLayout);

        for (int i = 0; i < 6; ++i) {
            auto *layout = new QVBoxLayout;
            QString text;
            if (i > 0)
                text = "ABC";

            QPair<QColor, QColor> color = [i] {
                switch (i) {
                case 0: return qMakePair(QColor(), QColor());
                case 1: return qMakePair(QColor(Qt::black), QColor(Qt::white));
                case 2: return qMakePair(QColor(Qt::white), QColor(Qt::black));
                case 3: return qMakePair(QColor(Qt::magenta), QColor(Qt::green));
                case 4: return qMakePair(QColor(0, 0, 0, 128), QColor(Qt::white));
                case 5: return qMakePair(QColor(255, 255, 255, 128), QColor(Qt::black));
                default: return qMakePair(QColor(), QColor());
                }
            }();

            for (int pointSize : {8, 12, 24, 36, 48})
                layout->addWidget(new TextRenderer(pointSize, text, color.first, color.second));

            static_cast<QHBoxLayout*>(m_previews->layout())->addLayout(layout);
        }

        mainLayout->addWidget(m_previews);

        auto *controls = new QHBoxLayout;
        auto *lineEdit = new QLineEdit(s_text);
        connect(lineEdit, &QLineEdit::textChanged, [&](const QString &text) {
            s_text = text;
            for (TextRenderer *renderer : m_previews->findChildren<TextRenderer *>())
                renderer->updateGeometry();
        });
        controls->addWidget(lineEdit);

        auto *colorButton = new QPushButton("Color...");
        connect(colorButton, &QPushButton::clicked, [&] {
            auto *colorDialog = new QColorDialog(this);
            colorDialog->setOptions(QColorDialog::NoButtons | QColorDialog::ShowAlphaChannel);
            colorDialog->setModal(false);
            connect(colorDialog, &QColorDialog::currentColorChanged, [&](const QColor &color) {
                QPalette p = palette();
                p.setColor(QPalette::Text, color);
                setPalette(p);
            });
            colorDialog->setCurrentColor(palette().text().color());
            colorDialog->setVisible(true);
        });
        controls->addWidget(colorButton);
        auto *fontButton = new QPushButton("Font...");
        connect(fontButton, &QPushButton::clicked, [&] {
            auto *fontDialog = new QFontDialog(this);
            fontDialog->setOptions(QFontDialog::NoButtons);
            fontDialog->setModal(false);
            fontDialog->setCurrentFont(m_previews->font());
            connect(fontDialog, &QFontDialog::currentFontChanged, [&](const QFont &font) {
                m_previews->setFont(font);
            });
            fontDialog->setVisible(true);
        });
        controls->addWidget(fontButton);

        auto *aaButton = new QCheckBox("NoAntialias");
        connect(aaButton, &QCheckBox::stateChanged, [&] {
            for (TextRenderer *renderer : m_previews->findChildren<TextRenderer *>()) {
                QFont font = renderer->font();
                font.setStyleStrategy(QFont::StyleStrategy(font.styleStrategy() ^ QFont::NoAntialias));
                renderer->setFont(font);
            }
        });
        controls->addWidget(aaButton);

        auto *subpixelAAButton = new QCheckBox("NoSubpixelAntialias");
        connect(subpixelAAButton, &QCheckBox::stateChanged, [&] {
            for (TextRenderer *renderer : m_previews->findChildren<TextRenderer *>()) {
                QFont font = renderer->font();
                font.setStyleStrategy(QFont::StyleStrategy(font.styleStrategy() ^ QFont::NoSubpixelAntialias));
                renderer->setFont(font);
            }
        });
        controls->addWidget(subpixelAAButton);
        controls->addStretch();

        mainLayout->addLayout(controls);

        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
        setLayout(mainLayout);

        setMode(TextRenderer::QtRendering);
        setFocusPolicy(Qt::StrongFocus);
        setFocus();
    }

    void setMode(TextRenderer::RenderingMode mode)
    {
        s_mode = mode;
        setWindowTitle(s_mode == TextRenderer::QtRendering ? "Qt" : "Native");

        for (TextRenderer *renderer : m_previews->findChildren<TextRenderer *>())
            renderer->update();
    }

    void mousePressEvent(QMouseEvent *) override
    {
        setMode(TextRenderer::RenderingMode(!s_mode));
    }

    void keyPressEvent(QKeyEvent *e) override
    {
        if (e->key() == Qt::Key_Space)
            setMode(TextRenderer::RenderingMode(!s_mode));
    }

    QWidget *m_previews;
};

int main(int argc, char **argv)
{
    qputenv("QT_MAX_CACHED_GLYPH_SIZE", "97");
    QApplication app(argc, argv);

    TestWidget widget;
    widget.show();
    return app.exec();
}

#include "main.moc"

