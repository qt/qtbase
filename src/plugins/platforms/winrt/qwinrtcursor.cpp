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

#include "qwinrtcursor.h"
#include "qwinrtscreen.h"
#include <private/qeventdispatcher_winrt_p.h>

#include <QtCore/qfunctions_winrt.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <functional>
#include <wrl.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::Foundation;

QT_BEGIN_NAMESPACE

class QWinRTCursorPrivate
{
public:
    ComPtr<ICoreCursorFactory> cursorFactory;
};

QWinRTCursor::QWinRTCursor()
  : d_ptr(new QWinRTCursorPrivate)
{
    Q_D(QWinRTCursor);

    HRESULT hr;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Core_CoreCursor).Get(),
                                IID_PPV_ARGS(&d->cursorFactory));
    Q_ASSERT_SUCCEEDED(hr);
}

QWinRTCursor::~QWinRTCursor()
{
}

#ifndef QT_NO_CURSOR
void QWinRTCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    Q_D(QWinRTCursor);

    HRESULT hr;
    ICoreWindow *coreWindow = static_cast<QWinRTScreen *>(window->screen()->handle())->coreWindow();

    CoreCursorType type;
    switch (windowCursor ? windowCursor->shape() : Qt::ArrowCursor) {
    case Qt::BlankCursor:
        hr = QEventDispatcherWinRT::runOnXamlThread([coreWindow]() {
            coreWindow->put_PointerCursor(nullptr);
            return S_OK;
        });
        RETURN_VOID_IF_FAILED("Failed to set blank native cursor");
        return;
    default:
    case Qt::OpenHandCursor:
    case Qt::ClosedHandCursor:
    case Qt::DragCopyCursor:
    case Qt::DragMoveCursor:
    case Qt::DragLinkCursor:
        // (unavailable)
    case Qt::ArrowCursor:
        type = CoreCursorType_Arrow;
        break;
    case Qt::UpArrowCursor:
        type = CoreCursorType_UpArrow;
        break;
    case Qt::CrossCursor:
        type = CoreCursorType_Cross;
        break;
    case Qt::WaitCursor:
    case Qt::BusyCursor:
        type = CoreCursorType_Wait;
        break;
    case Qt::IBeamCursor:
        type = CoreCursorType_IBeam;
        break;
    case Qt::SizeVerCursor:
    case Qt::SplitVCursor:
        type = CoreCursorType_SizeNorthSouth;
        break;
    case Qt::SizeHorCursor:
    case Qt::SplitHCursor:
        type = CoreCursorType_SizeWestEast;
        break;
    case Qt::SizeBDiagCursor:
        type = CoreCursorType_SizeNortheastSouthwest;
        break;
    case Qt::SizeFDiagCursor:
        type = CoreCursorType_SizeNorthwestSoutheast;
        break;
    case Qt::SizeAllCursor:
        type = CoreCursorType_SizeAll;
        break;
    case Qt::PointingHandCursor:
        type = CoreCursorType_Hand;
        break;
    case Qt::ForbiddenCursor:
        type = CoreCursorType_UniversalNo;
        break;
    case Qt::WhatsThisCursor:
        type = CoreCursorType_Help;
        break;
    case Qt::BitmapCursor:
    case Qt::CustomCursor:
        // TODO: figure out if arbitrary bitmaps can be made into resource IDs
        // For now, we don't get enough info from QCursor to set a custom cursor
        type = CoreCursorType_Custom;
        break;
    }

    ComPtr<ICoreCursor> cursor;
    hr = d->cursorFactory->CreateCursor(type, 0, &cursor);
    RETURN_VOID_IF_FAILED("Failed to create native cursor.");

    hr = QEventDispatcherWinRT::runOnXamlThread([coreWindow, &cursor]() {
        return coreWindow->put_PointerCursor(cursor.Get());
    });
    RETURN_VOID_IF_FAILED("Failed to set native cursor");
}
#endif // QT_NO_CURSOR

QPoint QWinRTCursor::pos() const
{
    const QWinRTScreen *screen = static_cast<QWinRTScreen *>(QGuiApplication::primaryScreen()->handle());
    Q_ASSERT(screen);
    ICoreWindow *coreWindow = screen->coreWindow();
    Q_ASSERT(coreWindow);
    Point point;
    HRESULT hr = QEventDispatcherWinRT::runOnXamlThread([coreWindow, &point]() {
        return coreWindow->get_PointerPosition(&point);
    });
    Q_ASSERT_SUCCEEDED(hr);
    const QPoint position = QPoint(point.X, point.Y) * screen->scaleFactor();
    // If no cursor get_PointerPosition returns SHRT_MIN for x and y
    return position.x() == SHRT_MIN && position.y() == SHRT_MIN || FAILED(hr) ? QPointF(Q_INFINITY, Q_INFINITY).toPoint()
                                                                              : position;
}

QT_END_NAMESPACE

