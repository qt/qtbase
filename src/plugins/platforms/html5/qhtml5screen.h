/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTML5SCREEN_H
#define QHTML5SCREEN_H

#include <qpa/qplatformscreen.h>

#include <QScopedPointer>
#include <QtCore/QTextStream>

#include <QtEglSupport/private/qt_egl_p.h>

QT_BEGIN_NAMESPACE

class QPlatformOpenGLContext;
class QHtml5Window;
class QHTML5BackingStore;
class QHtml5Compositor;
class QOpenGLContext;

class QHTML5Screen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:

    QHTML5Screen(EGLNativeDisplayType display, QHtml5Compositor *compositor);
    ~QHTML5Screen();

    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    QPlatformOpenGLContext *platformContext() const;
    EGLSurface surface() const { return m_surface; }

    void resizeMaximizedWindows();
    QWindow *topWindow() const;
    QWindow *topLevelAt(const QPoint & p) const override;

    //virtual void addWindow(QHtml5Window *window);
    //virtual void removeWindow(QHtml5Window *window);
    //virtual void raise(QHtml5Window *window);
    //virtual void lower(QHtml5Window *window);
    //virtual void topWindowChanged(QWindow *) {}
    //virtual int windowCount() const;

    void addPendingBackingStore(QHTML5BackingStore *bs) { mPendingBackingStores << bs; }

    //void scheduleUpdate();

    void invalidateSize();

public slots:
    //virtual void setDirty(const QRect &rect);
    void setGeometry(const QRect &rect);
protected:
    //QList<QHtml5Window *> mWindowStack;
    //QRegion mRepaintRegion;

private:
    void createAndSetPlatformContext() const;
    void createAndSetPlatformContext();
    bool mUpdatePending;

private:
    QHtml5Compositor *mCompositor;

    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QPlatformOpenGLContext *m_platformContext;
    QScopedPointer<QOpenGLContext> m_context;

    EGLDisplay m_dpy;
    EGLSurface m_surface;

    QList<QHTML5BackingStore*> mPendingBackingStores;

};
//Q_DECLARE_OPERATORS_FOR_FLAGS(QHTML5Screen::Flags)

QT_END_NAMESPACE
#endif // QHTML5SCREEN_H
