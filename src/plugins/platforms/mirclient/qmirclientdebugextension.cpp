/****************************************************************************
**
** Copyright (C) 2016 Canonical, Ltd.
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


#include "qmirclientdebugextension.h"

#include "qmirclientlogging.h"

// mir client debug
#include <mir_toolkit/debug/surface.h>

Q_LOGGING_CATEGORY(mirclientDebug, "qt.qpa.mirclient.debug")

QMirClientDebugExtension::QMirClientDebugExtension()
    : m_mirclientDebug(QStringLiteral("mirclient-debug-extension"), 1)
    , m_mapper(nullptr)
{
    qCDebug(mirclientDebug) << "NOTICE: Loading mirclient-debug-extension";
    m_mapper = (MapperPrototype) m_mirclientDebug.resolve("mir_debug_surface_coords_to_screen");

    if (!m_mirclientDebug.isLoaded()) {
        qCWarning(mirclientDebug) << "ERROR: mirclient-debug-extension failed to load:"
                                  << m_mirclientDebug.errorString();
    } else if (!m_mapper) {
        qCWarning(mirclientDebug) << "ERROR: unable to find required symbols in mirclient-debug-extension:"
                                  << m_mirclientDebug.errorString();
    }
}

QPoint QMirClientDebugExtension::mapSurfacePointToScreen(MirSurface *surface, const QPoint &point)
{
    if (!m_mapper) {
        return point;
    }

    QPoint mappedPoint;
    bool status = m_mapper(surface, point.x(), point.y(), &mappedPoint.rx(), &mappedPoint.ry());
    if (status) {
        return mappedPoint;
    } else {
        return point;
    }
}
