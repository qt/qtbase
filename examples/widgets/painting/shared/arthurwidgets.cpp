/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

extern QPixmap cached(const QString &img);

ArthurFrame::ArthurFrame(QWidget *parent)
    : QWidget(parent)
    , m_prefer_image(false)
{
#ifdef QT_OPENGL_SUPPORT
    glw = 0;
    m_use_opengl = false;
    QGLFormat f = QGLFormat::defaultFormat();
    f.setSampleBuffers(true);
    f.setStencil(true);
    f.setAlpha(true);
    f.setAlphaBufferSize(8);
    QGLFormat::setDefaultFormat(f);
#endif
    m_document = 0;
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


#ifdef QT_OPENGL_SUPPORT
void ArthurFrame::enableOpenGL(bool use_opengl)
{
    if (m_use_opengl == use_opengl)
        return;

    if (!glw && use_opengl) {
        glw = new GLWidget(this);
        glw->setAutoFillBackground(false);
        glw->disableAutoBufferSwap();
        QApplication::postEvent(this, new QResizeEvent(size(), size()));
    }

    m_use_opengl = use_opengl;
    if (use_opengl) {
        glw->show();
    } else {
        if (glw)
            glw->hide();
    }

    update();
}
#endif

void ArthurFrame::paintEvent(QPaintEvent *e)
{
    static QImage *static_image = 0;
    QPainter painter;
    if (preferImage()
#ifdef QT_OPENGL_SUPPORT
        && !m_use_opengl
#endif
        ) {
        if (!static_image || static_image->size() != size()) {
            delete static_image;
            static_image = new QImage(size(), QImage::Format_RGB32);
        }
        painter.begin(static_image);

        int o = 10;

        QBrush bg = palette().brush(QPalette::Background);
        painter.fillRect(0, 0, o, o, bg);
        painter.fillRect(width() - o, 0, o, o, bg);
        painter.fillRect(0, height() - o, o, o, bg);
        painter.fillRect(width() - o, height() - o, o, o, bg);
    } else {
#ifdef QT_OPENGL_SUPPORT
        if (m_use_opengl) {
            painter.begin(glw);
            painter.fillRect(QRectF(0, 0, glw->width(), glw->height()), palette().color(backgroundRole()));
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
#ifdef QT_OPENGL_SUPPORT
        && !m_use_opengl
#endif
        ) {
        painter.end();
        painter.begin(this);
        painter.drawImage(e->rect(), *static_image, e->rect());
    }

#ifdef QT_OPENGL_SUPPORT
    if (m_use_opengl && (inherits("PathDeformRenderer") || inherits("PathStrokeRenderer") || inherits("CompositionRenderer") || m_show_doc))
        glw->swapBuffers();
#endif
}

void ArthurFrame::resizeEvent(QResizeEvent *e)
{
#ifdef QT_OPENGL_SUPPORT
    if (glw)
        glw->setGeometry(0, 0, e->size().width()-1, e->size().height()-1);
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
        contents = QString("No source for widget: '%1'").arg(objectName());
    } else {
        QFile f(m_sourceFileName);
        if (!f.open(QFile::ReadOnly))
            contents = QString("Could not open file: '%1'").arg(m_sourceFileName);
        else
            contents = f.readAll();
    }

    contents.replace('&', "&amp;");
    contents.replace('<', "&lt;");
    contents.replace('>', "&gt;");

    QStringList keywords;
    keywords << "for " << "if " << "switch " << " int " << "#include " << "const"
             << "void " << "uint " << "case " << "double " << "#define " << "static"
             << "new" << "this";

    foreach (QString keyword, keywords)
        contents.replace(keyword, QLatin1String("<font color=olive>") + keyword + QLatin1String("</font>"));
    contents.replace("(int ", "(<font color=olive><b>int </b></font>");

    QStringList ppKeywords;
    ppKeywords << "#ifdef" << "#ifndef" << "#if" << "#endif" << "#else";

    foreach (QString keyword, ppKeywords)
        contents.replace(keyword, QLatin1String("<font color=navy>") + keyword + QLatin1String("</font>"));

    contents.replace(QRegExp("(\\d\\d?)"), QLatin1String("<font color=navy>\\1</font>"));

    QRegExp commentRe("(//.+)\\n");
    commentRe.setMinimal(true);
    contents.replace(commentRe, QLatin1String("<font color=red>\\1</font>\n"));

    QRegExp stringLiteralRe("(\".+\")");
    stringLiteralRe.setMinimal(true);
    contents.replace(stringLiteralRe, QLatin1String("<font color=green>\\1</font>"));

    QString html = contents;
    html.prepend("<html><pre>");
    html.append("</pre></html>");

    QTextBrowser *sourceViewer = new QTextBrowser(0);
    sourceViewer->setWindowTitle("Source: " + m_sourceFileName.mid(5));
    sourceViewer->setParent(this, Qt::Dialog);
    sourceViewer->setAttribute(Qt::WA_DeleteOnClose);
    sourceViewer->setLineWrapMode(QTextEdit::NoWrap);
    sourceViewer->setHtml(html);
    sourceViewer->resize(600, 600);
    sourceViewer->show();
}
