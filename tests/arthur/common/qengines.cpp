/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qengines.h"
#include "paintcommands.h"

#include <QApplication>
#include <QProcess>
#include <QPainter>
#include <QSvgRenderer>
#include <QStringList>
#include <QDir>
#include <QDebug>
#include <QPrintEngine>
#include <QWidget>

// For QApplicationPrivate::graphics_system_name
#include <private/qapplication_p.h>

QEngine::~QEngine()
{
}

Q_GLOBAL_STATIC(QtEngines, qtengines_global)
QtEngines * QtEngines::self()
{
    return qtengines_global();
}


QList<QEngine*> QtEngines::engines() const
{
    return m_engines;
}


QList<QEngine*> QtEngines::foreignEngines() const
{
    return m_foreignEngines;
}


QEngine * QtEngines::defaultEngine() const
{
    return m_defaultEngine;
}


QtEngines::QtEngines()
{
    init();
}


void QtEngines::init()
{
    m_defaultEngine = new RasterEngine;
    m_engines << m_defaultEngine
              << new NativeEngine
              << new WidgetEngine;

#if defined(BUILD_OPENGL)
    if (QGLFormat::hasOpenGL())
        m_engines << new GLEngine;
#endif

#ifndef QT_NO_PRINTER
    m_engines << new PDFEngine
#ifdef Q_WS_X11
              << new PSEngine
#endif
#ifdef Q_WS_WIN
			  << new WinPrintEngine
#endif
        ;
#endif //QT_NO_PRINTER
    
    m_foreignEngines << new RSVGEngine;
}

RasterEngine::RasterEngine()
{

}

QString RasterEngine::name() const
{
    return QLatin1String("Raster");
}


void RasterEngine::prepare(const QSize &size, const QColor &fillColor)
{
    image = QImage(size, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&image);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(image.rect(), fillColor);
}


void RasterEngine::render(QSvgRenderer *r, const QString &)
{
    QPainter p(&image);
    r->render(&p);
    p.end();
}


void RasterEngine::render(const QStringList &qpsScript,
                          const QString &absFilePath)
{
    QPainter pt(&image);
    PaintCommands pcmd(qpsScript, 800, 800);
    pcmd.setPainter(&pt);
    pcmd.setFilePath(absFilePath);
    pcmd.runCommands();
    pt.end();
}

bool RasterEngine::drawOnPainter(QPainter *p)
{
    p->drawImage(0, 0, image);
    return true;
}

void RasterEngine::save(const QString &file)
{
    image.save(file, "PNG");
}


NativeEngine::NativeEngine()
{

}


QString NativeEngine::name() const
{
#ifdef Q_WS_X11
#ifndef QT_NO_XRENDER
    return QLatin1String("NativeXRender");
#else
    return QLatin1String("NativeXLib");
#endif
#elif (defined Q_WS_WIN32)
    return QLatin1String("NativeWin32");
#elif (defined Q_WS_MAC)
    return QLatin1String("NativeMac");
#elif defined(Q_WS_QWS)
    return QLatin1String("NativeEmbedded");
#endif
}


void NativeEngine::prepare(const QSize &size, const QColor &fillColor)
{
    pixmap = QPixmap(size);
    pixmap.fill(fillColor);
}


void NativeEngine::render(QSvgRenderer *r, const QString &)
{
    QPainter p(&pixmap);
    r->render(&p);
    p.end();
}


void NativeEngine::render(const QStringList &qpsScript,
                          const QString &absFilePath)
{
    QPainter pt(&pixmap);
    PaintCommands pcmd(qpsScript, 800, 800);
    pcmd.setPainter(&pt);
    pcmd.setFilePath(absFilePath);
    pcmd.runCommands();
    pt.end();
}

bool NativeEngine::drawOnPainter(QPainter *p)
{
    p->drawPixmap(0, 0, pixmap);
    return true;
}

void NativeEngine::save(const QString &file)
{
    pixmap.save(file, "PNG");
}


#if defined(BUILD_OPENGL)
GLEngine::GLEngine()
    : pbuffer(0), widget(0)
{
    usePixelBuffers = QGLPixelBuffer::hasOpenGLPbuffers();
}


QString GLEngine::name() const
{
    return QLatin1String("OpenGL");
}

void GLEngine::prepare(const QSize &_size, const QColor &color)
{
    size = _size;
    fillColor = color;
    if (usePixelBuffers) {
        pbuffer = new QGLPixelBuffer(size, QGLFormat(QGL::SampleBuffers));
    } else {
        widget = new QGLWidget(QGLFormat(QGL::SampleBuffers));
        widget->setAutoFillBackground(false);
        widget->resize(size);
        widget->show();
        QApplication::flush();
        QApplication::syncX();
    }
}

void GLEngine::render(QSvgRenderer *r, const QString &)
{
    QPainter *p;
    if (usePixelBuffers)
        p = new QPainter(pbuffer);
    else
        p = new QPainter(widget);
    p->fillRect(0, 0, size.width(), size.height(), fillColor);
    r->render(p);
    p->end();
    delete p;
}

void GLEngine::render(const QStringList &qpsScript,
                      const QString &absFilePath)
{
    QPainter *p;
    if (usePixelBuffers)
        p = new QPainter(pbuffer);
    else
        p = new QPainter(widget);

    PaintCommands pcmd(qpsScript, 800, 800);
    pcmd.setPainter(p);
    pcmd.setFilePath(absFilePath);
    pcmd.runCommands();
    p->end();
    delete p;
}

bool GLEngine::drawOnPainter(QPainter *p)
{
    if (usePixelBuffers) {
        QImage img = pbuffer->toImage();
        p->drawImage(0, 0, img);
    } else {
        QImage img = widget->grabFrameBuffer();
        p->drawImage(0, 0, img);
    }
    return true;
}


void GLEngine::save(const QString &file)
{
    if (usePixelBuffers) {
        QImage img = pbuffer->toImage();
        img.save(file, "PNG");
    } else {
        QImage img = widget->grabFrameBuffer();
        img.save(file, "PNG");
    }
}

void GLEngine::cleanup()
{
    delete pbuffer;
    delete widget;
}

#endif

class WidgetEngineWidget : public QWidget
{
public:
    WidgetEngineWidget(QWidget* =0);

    void paintEvent(QPaintEvent*);
    void render(QSvgRenderer*);
    void render(QStringList const&,QString const&);

    QSize           m_size;
    QColor          m_fillColor;

private:
    QSvgRenderer*   m_svgr;
    QStringList     m_qpsScript;
    QString         m_absFilePath;
};

WidgetEngineWidget::WidgetEngineWidget(QWidget* parent)
    : QWidget(parent)
    , m_size()
    , m_fillColor()
    , m_svgr(0)
    , m_qpsScript()
    , m_absFilePath()
{}

void WidgetEngineWidget::render(QSvgRenderer* r)
{
    m_svgr = r;
    repaint();
    m_svgr = 0;
}

void WidgetEngineWidget::render(QStringList const& qpsScript, QString const& absFilePath)
{
    m_qpsScript = qpsScript;
    m_absFilePath = absFilePath;
    repaint();
    m_qpsScript = QStringList();
    m_absFilePath = QString();
}

void WidgetEngineWidget::paintEvent(QPaintEvent*)
{
    if (m_svgr) {
        QPainter p(this);
        p.fillRect(0, 0, m_size.width(), m_size.height(), m_fillColor);
        m_svgr->render(&p);
        p.end();
    }
    else {
        QPainter p(this);

        PaintCommands pcmd(m_qpsScript, 800, 800);
        pcmd.setPainter(&p);
        pcmd.setFilePath(m_absFilePath);
        pcmd.runCommands();
        p.end();
    }
}

WidgetEngine::WidgetEngine()
    : m_widget(0)
{
}


QString WidgetEngine::name() const
{
    QString gs = QApplicationPrivate::graphics_system_name;
    if (!gs.isEmpty()) gs[0] = gs[0].toUpper();
    return QString::fromLatin1("Widget") + gs;
}

void WidgetEngine::prepare(const QSize &size, const QColor &color)
{
    m_widget = new WidgetEngineWidget;
    m_widget->m_size = size;
    m_widget->m_fillColor = color;
    m_widget->setAutoFillBackground(false);
    m_widget->resize(size);
    m_widget->show();
    QApplication::flush();
    QApplication::syncX();
}

void WidgetEngine::render(QSvgRenderer *r, const QString &)
{
    m_widget->render(r);
}

void WidgetEngine::render(const QStringList &qpsScript,
                      const QString &absFilePath)
{
    m_widget->render(qpsScript, absFilePath);
}

bool WidgetEngine::drawOnPainter(QPainter *p)
{
    p->drawPixmap(0, 0, QPixmap::grabWindow(m_widget->winId()));
    return true;
}


void WidgetEngine::save(const QString &file)
{
    QImage img = QPixmap::grabWindow(m_widget->winId()).toImage();
    img.save(file, "PNG");
}

void WidgetEngine::cleanup()
{
    delete m_widget;
}

#ifndef QT_NO_PRINTER
PDFEngine::PDFEngine()
{
}


QString PDFEngine::name() const
{
    return QLatin1String("PDF");
}

void PDFEngine::prepare(const QSize &size, const QColor &fillColor)
{
    Q_UNUSED(fillColor);

    static int i = 1;

    m_size = size;
    printer = new QPrinter(QPrinter::ScreenResolution);
    printer->setOutputFormat(QPrinter::PdfFormat);
    printer->setFullPage(true);
    //printer->setOrientation(QPrinter::Landscape);
    m_tempFile = QDir::tempPath() + QString("temp%1.pdf").arg(i++);
    printer->setOutputFileName(m_tempFile);
}

void PDFEngine::render(QSvgRenderer *r, const QString &)
{
    QPainter p(printer);
    r->render(&p);
    p.end();
}


void PDFEngine::render(const QStringList &qpsScript,
                       const QString &absFilePath)
{
    QPainter pt(printer);
    PaintCommands pcmd(qpsScript, 800, 800);
    pcmd.setPainter(&pt);
    pcmd.setFilePath(absFilePath);
    pcmd.runCommands();
    pt.end();
}

bool PDFEngine::drawOnPainter(QPainter *)
{
    return false;
}

void PDFEngine::save(const QString &file)
{
#ifdef USE_ACROBAT
    QString psFile = m_tempFile;
    psFile.replace(".pdf", ".ps");
    QProcess toPs;
    QStringList args1;
    args1 << "-toPostScript"
          << "-level3"
          << "-transQuality"
          << "1";
    args1 << m_tempFile;
    toPs.start("acroread", args1);
    toPs.waitForFinished();

    QProcess convert;
    QStringList args;
    args << psFile;
    args << QString("-resize")
         << QString("%1x%2")
        .arg(m_size.width())
        .arg(m_size.height());
    args << file;

    convert.start("convert", args);
    convert.waitForFinished();
    QFile::remove(psFile);
#else
    QProcess toPng;
    QStringList args1;
    args1 << "-sDEVICE=png16m"
          << QString("-sOutputFile=") + file
          << "-r97x69"
          << "-dBATCH"
          << "-dNOPAUSE";
    args1 << m_tempFile;
    toPng.start("gs", args1);
    toPng.waitForFinished();
#endif

     QString pfile = file;
     pfile.replace(".png", ".pdf");
     QFile::rename(m_tempFile, pfile);
//    QFile::remove(m_tempFile);
}

void PDFEngine::cleanup()
{
    delete printer; printer = 0;
}

#ifdef Q_WS_X11
PSEngine::PSEngine()
{
}


QString PSEngine::name() const
{
    return QLatin1String("PS");
}

void PSEngine::prepare(const QSize &size, const QColor &fillColor)
{
    Q_UNUSED(fillColor);

    static int i = 1;

    m_size = size;
    printer = new QPrinter(QPrinter::ScreenResolution);
    printer->setOutputFormat(QPrinter::PostScriptFormat);
    printer->setFullPage(true);
    m_tempFile = QDir::tempPath() + QString("temp%1.ps").arg(i++);
    printer->setOutputFileName(m_tempFile);
}

void PSEngine::render(QSvgRenderer *r, const QString &)
{
    QPainter p(printer);
    r->render(&p);
    p.end();
}


void PSEngine::render(const QStringList &qpsScript,
                      const QString &absFilePath)
{
    QPainter pt(printer);
    PaintCommands pcmd(qpsScript, 800, 800);
    pcmd.setPainter(&pt);
    pcmd.setFilePath(absFilePath);
    pcmd.runCommands();
    pt.end();
}

bool PSEngine::drawOnPainter(QPainter *)
{
    return false;
}

void PSEngine::save(const QString &file)
{
    QProcess toPng;
    QStringList args1;
    args1 << "-sDEVICE=png16m"
          << QString("-sOutputFile=") + file
          << "-r97x69"
          << "-dBATCH"
          << "-dNOPAUSE";
    args1 << m_tempFile;
    toPng.start("gs", args1);
    toPng.waitForFinished();

    QString pfile = file;
    pfile.replace(".png", ".ps");
    QFile::rename(m_tempFile, pfile);
}

void PSEngine::cleanup()
{
    delete printer; printer = 0;
}
#endif
#endif //QT_NO_PRINTER

RSVGEngine::RSVGEngine()
{

}

QString RSVGEngine::name() const
{
    return QLatin1String("RSVG");
}

void RSVGEngine::prepare(const QSize &size, const QColor &fillColor)
{
    Q_UNUSED(fillColor);

    m_size = size;
}

void RSVGEngine::render(QSvgRenderer *, const QString &fileName)
{
    m_fileName = fileName;
}

void RSVGEngine::render(const QStringList &, const QString &)
{
}

bool RSVGEngine::drawOnPainter(QPainter *)
{
    return false;
}


void RSVGEngine::save(const QString &file)
{
    QProcess rsvg;
    QStringList args;
    args << QString("-w %1").arg(m_size.width());
    args << QString("-h %1").arg(m_size.height());
    args << m_fileName;
    args << file;
    rsvg.start("rsvg", args);
    rsvg.waitForFinished();
}

void QEngine::cleanup()
{
}

#ifdef Q_WS_WIN
WinPrintEngine::WinPrintEngine()
{
}


QString WinPrintEngine::name() const
{
    return QLatin1String("WinPrint");
}

void WinPrintEngine::prepare(const QSize &size, const QColor &fillColor)
{
    Q_UNUSED(fillColor);

    static int i = 1;

    m_size = size;
    printer = new QPrinter(QPrinter::ScreenResolution);
    printer->setFullPage(true);
	printer->setPrinterName("HP 2500C Series PS3");
    m_tempFile = QDir::tempPath() + QString("temp%1.ps").arg(i++);
    printer->setOutputFileName(m_tempFile);
}

void WinPrintEngine::render(QSvgRenderer *r, const QString &)
{
    QPainter p(printer);
    r->render(&p);
    p.end();
}


void WinPrintEngine::render(const QStringList &qpsScript,
                      const QString &absFilePath)
{
    QPainter pt(printer);
    PaintCommands pcmd(qpsScript, 800, 800);
    pcmd.setPainter(&pt);
    pcmd.setFilePath(absFilePath);
    pcmd.runCommands();
    pt.end();
}

bool WinPrintEngine::drawOnPainter(QPainter *)
{
    return false;
}

void WinPrintEngine::save(const QString &file)
{
    QProcess toPng;
    QStringList args1;
    args1 << "-sDEVICE=png16m"
          << QString("-sOutputFile=") + file
          << "-r97x69"
          << "-dBATCH"
          << "-dNOPAUSE";
    args1 << m_tempFile;
    toPng.start("gswin32", args1);
    toPng.waitForFinished();

    QString pfile = file;
    pfile.replace(".png", ".ps");
    QFile::rename(m_tempFile, pfile);
}

void WinPrintEngine::cleanup()
{
    delete printer; printer = 0;
}

#endif
