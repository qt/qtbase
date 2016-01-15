/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSDRAG_H
#define QWINDOWSDRAG_H

#include "qwindowsinternalmimedata.h"

#include <qpa/qplatformdrag.h>
#include <QtGui/QPixmap>

struct IDropTargetHelper;

QT_BEGIN_NAMESPACE

class QPlatformScreen;

class QWindowsDropMimeData : public QWindowsInternalMimeData {
public:
    QWindowsDropMimeData() {}
    IDataObject *retrieveDataObject() const Q_DECL_OVERRIDE;
};

class QWindowsOleDropTarget : public IDropTarget
{
public:
    explicit QWindowsOleDropTarget(QWindow *w);
    virtual ~QWindowsOleDropTarget();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IDropTarget methods
    STDMETHOD(DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

private:
    void handleDrag(QWindow *window, DWORD grfKeyState, const QPoint &, LPDWORD pdwEffect);

    ULONG m_refs;
    QWindow *const m_window;
    QRect m_answerRect;
    QPoint m_lastPoint;
    DWORD m_chosenEffect;
    DWORD m_lastKeyState;
};

class QWindowsDrag : public QPlatformDrag
{
public:
    QWindowsDrag();
    virtual ~QWindowsDrag();

    QMimeData *platformDropData() Q_DECL_OVERRIDE { return &m_dropData; }

    Qt::DropAction drag(QDrag *drag) Q_DECL_OVERRIDE;

    static QWindowsDrag *instance();
    void cancelDrag() Q_DECL_OVERRIDE { QWindowsDrag::m_canceled = true; }
    static bool isCanceled() { return QWindowsDrag::m_canceled; }

    IDataObject *dropDataObject() const             { return m_dropDataObject; }
    void setDropDataObject(IDataObject *dataObject) { m_dropDataObject = dataObject; }
    void releaseDropDataObject();
    QMimeData *dropData();

    IDropTargetHelper* dropHelper();

private:
    static bool m_canceled;

    QWindowsDropMimeData m_dropData;
    IDataObject *m_dropDataObject;

    IDropTargetHelper* m_cachedDropTargetHelper;
};

QT_END_NAMESPACE

#endif // QWINDOWSDRAG_H
