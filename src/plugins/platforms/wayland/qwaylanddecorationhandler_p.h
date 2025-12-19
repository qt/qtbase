// Copyright (C) 2025
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDDECORATIONHANDLER_P_H
#define QWAYLANDDECORATIONHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwaylandsurface_p.h"
#include "qwaylandwindow_p.h"

#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtCore/QSize>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandWindow;
class QWaylandShmBuffer;
class QWaylandDisplay;
class QWaylandAbstractDecoration;

class Q_WAYLANDCLIENT_EXPORT QWaylandDecorationSurface
{
public:
    enum class Part { Top, Left, Right, Bottom };

    QWaylandDecorationSurface(QWaylandWindow *parentWindow, QWaylandAbstractDecoration *decoration,
                              Part part);
    ~QWaylandDecorationSurface();

    void setGeometry(const QRect &windowGeometry, const QRect &paintGeometry);
    bool paint();

    void handleMouse(QWaylandInputDevice *inputDevice, const QWaylandPointerEvent &e);
    QPointF mapToGlobal(const QPointF &pointIn);

private:
    struct SubsurfaceDeleter
    {
        void operator()(QtWayland::wl_subsurface *subsurface) const;
    };

    void createBuffer(const QSize &size);
    QWaylandWindow *mParentWindow = nullptr;
    QWaylandAbstractDecoration *mWindowDecoration;
    Part mPart;
    QRect mWindowGeometry;
    QRect mPaintGeometry;

    std::unique_ptr<QWaylandShmBuffer> mFrontBuffer;
    std::unique_ptr<QWaylandShmBuffer> mBackBuffer;

    std::unique_ptr<QWaylandSurface> mSurface;
    std::unique_ptr<QtWayland::wl_subsurface, SubsurfaceDeleter> mSubsurface;
    std::unique_ptr<QWaylandViewport> mViewport;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandDecorationHandler : public QObject
{
    Q_OBJECT
public:
    explicit QWaylandDecorationHandler(QWaylandWindow *window);
    ~QWaylandDecorationHandler();

    // update the contents of the buffers/surfaces and commit them
    void update();

    void reset();

    // the underlying decoration, may be null
    QWaylandAbstractDecoration *decoration() const;

    // called when the window flags or hints change
    void updateNeedsDecoration();

private:
    void updateTile(std::unique_ptr<QWaylandDecorationSurface> &surface,
                    const QRect &windowGeometry,
                    const QRect &paintGeometry,
                    QWaylandDecorationSurface::Part part);
    QWaylandWindow *mWindow = nullptr;

    std::unique_ptr<QWaylandAbstractDecoration> mWindowDecoration;
    std::unique_ptr<QWaylandDecorationSurface> mTopDecorationSurface;
    std::unique_ptr<QWaylandDecorationSurface> mLeftDecorationSurface;
    std::unique_ptr<QWaylandDecorationSurface> mRightDecorationSurface;
    std::unique_ptr<QWaylandDecorationSurface> mBottomDecorationSurface;
    bool mWindowDecorationEnabled = false;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDDECORATIONHANDLER_P_H
