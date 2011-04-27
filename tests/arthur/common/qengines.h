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
#ifndef QENGINES_H
#define QENGINES_H

#if defined(BUILD_OPENGL)
#include <QGLPixelBuffer>
#endif
#include <QPrinter>
#include <QPixmap>
#include <QImage>
#include <QMap>
#include <QList>
#include <QColor>

QT_FORWARD_DECLARE_CLASS(QSvgRenderer)
QT_FORWARD_DECLARE_CLASS(QGLWidget)

class QEngine
{
public:
    virtual ~QEngine();
    virtual QString name() const =0;
    virtual void prepare(const QSize &size, const QColor &fillColor = Qt::white) =0;
    virtual void render(QSvgRenderer *r, const QString &)    =0;
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath)    =0;
    virtual bool drawOnPainter(QPainter *p) =0;
    virtual void save(const QString &file)  =0;
    virtual void cleanup();
};

class QtEngines
{
public:
    static QtEngines *self();
    QtEngines();

    QList<QEngine*> engines() const;
    QList<QEngine*> foreignEngines() const;

    QEngine *defaultEngine() const;
private:
    void init();
private:
    QList<QEngine*> m_engines;
    QList<QEngine*> m_foreignEngines;
    QEngine        *m_defaultEngine;
};

class RasterEngine : public QEngine
{
public:
    RasterEngine();

    virtual QString name() const;
    virtual void prepare(const QSize &size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
private:
    QImage image;
};

class NativeEngine : public QEngine
{
public:
    NativeEngine();

    virtual QString name() const;
    virtual void prepare(const QSize &size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
private:
    QPixmap pixmap;
};


#if defined(BUILD_OPENGL)
class GLEngine : public QEngine
{
public:
    GLEngine();
    virtual QString name() const;
    virtual void prepare(const QSize &_size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
    virtual void cleanup();
private:
    QGLPixelBuffer *pbuffer;
    QGLWidget *widget;
    bool usePixelBuffers;
    QSize size;
    QColor fillColor;
};
#endif

class WidgetEngineWidget;
class WidgetEngine : public QEngine
{
public:
    WidgetEngine();
    virtual QString name() const;
    virtual void prepare(const QSize &_size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
    virtual void cleanup();
private:
    WidgetEngineWidget *m_widget;
};

#ifndef QT_NO_PRINTER
class PDFEngine : public QEngine
{
public:
    PDFEngine();

    virtual QString name() const;
    virtual void prepare(const QSize &size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
    virtual void cleanup();
private:
    QPrinter *printer;
    QSize     m_size;
    QString   m_tempFile;
};

#ifdef Q_WS_X11
class PSEngine : public QEngine
{
public:
    PSEngine();

    virtual QString name() const;
    virtual void prepare(const QSize &size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
    virtual void cleanup();
private:
    QPrinter *printer;
    QSize     m_size;
    QString   m_tempFile;
};
#endif

#ifdef Q_WS_WIN
class WinPrintEngine : public QEngine
{
public:
    WinPrintEngine();

    virtual QString name() const;
    virtual void prepare(const QSize &size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
    virtual void cleanup();
private:
    QPrinter *printer;
    QSize     m_size;
    QString   m_tempFile;
};
#endif
#endif //QT_NO_PRINTER

class RSVGEngine : public QEngine
{
public:
    RSVGEngine();

    virtual QString name() const;
    virtual void prepare(const QSize &size, const QColor &fillColor = Qt::white);
    virtual void render(QSvgRenderer *r, const QString &);
    virtual void render(const QStringList &qpsScript,
                        const QString &absFilePath);
    virtual bool drawOnPainter(QPainter *p);
    virtual void save(const QString &file);
private:
    QString m_fileName;
    QSize   m_size;
};

#endif
