/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include "arthurwidgets.h"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QtEvents>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QFile>
#include <QTextBrowser>
#include <QBoxLayout>
#include <QRegularExpression>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QOpenGLWindow>

extern QPixmap cached(const QString &img);

ArthurFrame::ArthurFrame(QWidget *parent)
    : QWidget(parent)
    , m_prefer_image(false)
{
#if QT_CONFIG(opengl)
    m_glWindow = nullptr;
    m_glWidget = nullptr;
    m_use_opengl = false;
#endif
    m_document = nullptr;
    m_show_doc = false;

    m_tile = QPixmap(128, 128);
    m_tile.fill(Qt::white);
    QPainter pt(&m_tile);
    QColor color(230, 230, 230);
    pt.fillRect(0, 0, 64, 64, color);
    pt.fillRect(64, 64, 64, 64, color);
    pt.end();

//     QPalette pal = palette();
//     pal.setBrush(backgroundRole(), m_tile);
//     setPalette(pal);
}


#if QT_CONFIG(opengl)
void ArthurFrame::enableOpenGL(bool use_opengl)
{
    if (m_use_opengl == use_opengl)
        return;

    m_use_opengl = use_opengl;

    if (!m_glWindow && use_opengl) {
        createGlWindow();
        QApplication::postEvent(this, new QResizeEvent(size(), size()));
    }

    if (use_opengl) {
        m_glWidget->show();
    } else {
        if (m_glWidget)
            m_glWidget->hide();
    }

    update();
}

void ArthurFrame::createGlWindow()
{
    Q_ASSERT(m_use_opengl);

    m_glWindow = new QOpenGLWindow();
    QSurfaceFormat f = QSurfaceFormat::defaultFormat();
    f.setSamples(4);
    f.setAlphaBufferSize(8);
    f.setStencilBufferSize(8);
    m_glWindow->setFormat(f);
    m_glWindow->setFlags(Qt::WindowTransparentForInput);
    m_glWindow->resize(width(), height());
    m_glWidget = QWidget::createWindowContainer(m_glWindow, this);
    // create() must be called after createWindowContainer() otherwise
    // an incorrect offsetting of the position will occur.
    m_glWindow->create();
}
#endif


void ArthurFrame::paintEvent(QPaintEvent *e)
{
    static QImage *static_image = nullptr;

    QPainter painter;

    if (preferImage()
#if QT_CONFIG(opengl)
        && !m_use_opengl
#endif
        ) {
        if (!static_image || static_image->size() != size()) {
            delete static_image;
            static_image = new QImage(size(), QImage::Format_RGB32);
        }
        painter.begin(static_image);

        int o = 10;

        QBrush bg = palette().brush(QPalette::Window);
        painter.fillRect(0, 0, o, o, bg);
        painter.fillRect(width() - o, 0, o, o, bg);
        painter.fillRect(0, height() - o, o, o, bg);
        painter.fillRect(width() - o, height() - o, o, o, bg);
    } else {
#if QT_CONFIG(opengl)
        if (m_use_opengl && m_glWindow->isValid()) {
            m_glWindow->makeCurrent();

            painter.begin(m_glWindow);
            painter.fillRect(QRectF(0, 0, m_glWindow->width(), m_glWindow->height()), palette().color(backgroundRole()));
        } else {
            painter.begin(this);
        }
#else
        painter.begin(this);
#endif
    }

    painter.setClipRect(e->rect());

    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath clipPath;

    QRect r = rect();
    qreal left = r.x() + 1;
    qreal top = r.y() + 1;
    qreal right = r.right();
    qreal bottom = r.bottom();
    qreal radius2 = 8 * 2;

    clipPath.moveTo(right - radius2, top);
    clipPath.arcTo(right - radius2, top, radius2, radius2, 90, -90);
    clipPath.arcTo(right - radius2, bottom - radius2, radius2, radius2, 0, -90);
    clipPath.arcTo(left, bottom - radius2, radius2, radius2, 270, -90);
    clipPath.arcTo(left, top, radius2, radius2, 180, -90);
    clipPath.closeSubpath();

    painter.save();
    painter.setClipPath(clipPath, Qt::IntersectClip);

    painter.drawTiledPixmap(rect(), m_tile);

    // client painting

    paint(&painter);

    painter.restore();

    painter.save();
    if (m_show_doc)
        paintDescription(&painter);
    painter.restore();

    int level = 180;
    painter.setPen(QPen(QColor(level, level, level), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(clipPath);

    if (preferImage()
#if QT_CONFIG(opengl)
        && !m_use_opengl
#endif
        ) {
        painter.end();
        painter.begin(this);
        painter.drawImage(e->rect(), *static_image, e->rect());
    }
#if QT_CONFIG(opengl)
    if (m_use_opengl)
        m_glWindow->update();
#endif
}

void ArthurFrame::resizeEvent(QResizeEvent *e)
{
#if QT_CONFIG(opengl)
    if (m_glWidget)
        m_glWidget->setGeometry(0, 0, e->size().width(), e->size().height());
#endif
    QWidget::resizeEvent(e);
}

void ArthurFrame::setDescriptionEnabled(bool enabled)
{
    if (m_show_doc != enabled) {
        m_show_doc = enabled;
        emit descriptionEnabledChanged(m_show_doc);
        update();
    }
}

void ArthurFrame::loadDescription(const QString &fileName)
{
    QFile textFile(fileName);
    QString text;
    if (!textFile.open(QFile::ReadOnly))
        text = QString("Unable to load resource file: '%1'").arg(fileName);
    else
        text = textFile.readAll();
    setDescription(text);
}


void ArthurFrame::setDescription(const QString &text)
{
    m_document = new QTextDocument(this);
    m_document->setHtml(text);
}

void ArthurFrame::paintDescription(QPainter *painter)
{
    if (!m_document)
        return;

    int pageWidth = qMax(width() - 100, 100);
    int pageHeight = qMax(height() - 100, 100);
    if (pageWidth != m_document->pageSize().width()) {
        m_document->setPageSize(QSize(pageWidth, pageHeight));
    }

    QRect textRect(width() / 2 - pageWidth / 2,
                   height() / 2 - pageHeight / 2,
                   pageWidth,
                   pageHeight);
    int pad = 10;
    QRect clearRect = textRect.adjusted(-pad, -pad, pad, pad);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 63));
    int shade = 10;
    painter->drawRect(clearRect.x() + clearRect.width() + 1,
                      clearRect.y() + shade,
                      shade,
                      clearRect.height() + 1);
    painter->drawRect(clearRect.x() + shade,
                      clearRect.y() + clearRect.height() + 1,
                      clearRect.width() - shade + 1,
                      shade);

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setBrush(QColor(255, 255, 255, 220));
    painter->setPen(Qt::black);
    painter->drawRect(clearRect);

    painter->setClipRegion(textRect, Qt::IntersectClip);
    painter->translate(textRect.topLeft());

    QAbstractTextDocumentLayout::PaintContext ctx;

    QLinearGradient g(0, 0, 0, textRect.height());
    g.setColorAt(0, Qt::black);
    g.setColorAt(0.9, Qt::black);
    g.setColorAt(1, Qt::transparent);

    QPalette pal = palette();
    pal.setBrush(QPalette::Text, g);

    ctx.palette = pal;
    ctx.clip = QRect(0, 0, textRect.width(), textRect.height());
    m_document->documentLayout()->draw(painter, ctx);
}

void ArthurFrame::loadSourceFile(const QString &sourceFile)
{
    m_sourceFileName = sourceFile;
}

void ArthurFrame::showSource()
{
    // Check for existing source
    if (findChild<QTextBrowser *>())
        return;

    QString contents;
    if (m_sourceFileName.isEmpty()) {
        contents = tr("No source for widget: '%1'").arg(objectName());
    } else {
        QFile f(m_sourceFileName);
        if (!f.open(QFile::ReadOnly))
            contents = tr("Could not open file: '%1'").arg(m_sourceFileName);
        else
            contents = f.readAll();
    }

    contents.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    contents.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    contents.replace(QLatin1Char('>'), QStringLiteral("&gt;"));

    static const QString keywords[] = {
        QStringLiteral("for "),      QStringLiteral("if "),
        QStringLiteral("switch "),   QStringLiteral(" int "),
        QStringLiteral("#include "), QStringLiteral("const"),
        QStringLiteral("void "),     QStringLiteral("uint "),
        QStringLiteral("case "),     QStringLiteral("double "),
        QStringLiteral("#define "),  QStringLiteral("static"),
        QStringLiteral("new"),       QStringLiteral("this")
    };

    for (const QString &keyword : keywords)
        contents.replace(keyword, QLatin1String("<font color=olive>") + keyword + QLatin1String("</font>"));
    contents.replace(QStringLiteral("(int "), QStringLiteral("(<font color=olive><b>int </b></font>"));

    static const QString ppKeywords[] = {
        QStringLiteral("#ifdef"), QStringLiteral("#ifndef"),
        QStringLiteral("#if"),    QStringLiteral("#endif"),
        QStringLiteral("#else")
    };

    for (const QString &keyword : ppKeywords)
        contents.replace(keyword, QLatin1String("<font color=navy>") + keyword + QLatin1String("</font>"));

    contents.replace(QRegularExpression("(\\d\\d?)"), QLatin1String("<font color=navy>\\1</font>"));

    QRegularExpression commentRe("(//.+?)\\n");
    contents.replace(commentRe, QLatin1String("<font color=red>\\1</font>\n"));

    QRegularExpression stringLiteralRe("(\".+?\")");
    contents.replace(stringLiteralRe, QLatin1String("<font color=green>\\1</font>"));

    const QString html = QStringLiteral("<html><pre>") + contents + QStringLiteral("</pre></html>");

    QTextBrowser *sourceViewer = new QTextBrowser;
    sourceViewer->setWindowTitle(tr("Source: %1").arg(m_sourceFileName.midRef(5)));
    sourceViewer->setParent(this, Qt::Dialog);
    sourceViewer->setAttribute(Qt::WA_DeleteOnClose);
    sourceViewer->setLineWrapMode(QTextEdit::NoWrap);
    sourceViewer->setHtml(html);
    sourceViewer->resize(600, 600);
    sourceViewer->show();
}
