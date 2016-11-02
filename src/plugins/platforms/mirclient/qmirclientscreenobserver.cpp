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


#include "qmirclientscreenobserver.h"
#include "qmirclientscreen.h"
#include "qmirclientwindow.h"
#include "qmirclientlogging.h"

// Qt
#include <QMetaObject>
#include <QPointer>

// Mir
#include <mirclient/mir_toolkit/mir_connection.h>
#include <mirclient/mir_toolkit/mir_display_configuration.h>

#include <memory>

namespace {
    static void displayConfigurationChangedCallback(MirConnection */*connection*/, void* context)
    {
        ASSERT(context != NULL);
        QMirClientScreenObserver *observer = static_cast<QMirClientScreenObserver *>(context);
        QMetaObject::invokeMethod(observer, "update");
    }

    const char *mirFormFactorToStr(MirFormFactor formFactor)
    {
        switch (formFactor) {
        case mir_form_factor_unknown: return "unknown";
        case mir_form_factor_phone: return "phone";
        case mir_form_factor_tablet: return "tablet";
        case mir_form_factor_monitor: return "monitor";
        case mir_form_factor_tv: return "tv";
        case mir_form_factor_projector: return "projector";
        }
        Q_UNREACHABLE();
    }
} // anonymous namespace

QMirClientScreenObserver::QMirClientScreenObserver(MirConnection *mirConnection)
    : mMirConnection(mirConnection)
{
    mir_connection_set_display_config_change_callback(mirConnection, ::displayConfigurationChangedCallback, this);
    update();
}

void QMirClientScreenObserver::update()
{
    // Wrap MirDisplayConfiguration to always delete when out of scope
    auto configDeleter = [](MirDisplayConfig *config) { mir_display_config_release(config); };
    using configUp = std::unique_ptr<MirDisplayConfig, decltype(configDeleter)>;
    configUp displayConfig(mir_connection_create_display_configuration(mMirConnection), configDeleter);

    // Mir only tells us something changed, it is up to us to figure out what.
    QList<QMirClientScreen*> newScreenList;
    QList<QMirClientScreen*> oldScreenList = mScreenList;
    mScreenList.clear();

    for (int i = 0; i < mir_display_config_get_num_outputs(displayConfig.get()); i++) {
        const MirOutput *output = mir_display_config_get_output(displayConfig.get(), i);
        if (mir_output_is_enabled(output)) {
            QMirClientScreen *screen = findScreenWithId(oldScreenList, mir_output_get_id(output));
            if (screen) { // we've already set up this display before
                screen->updateMirOutput(output);
                oldScreenList.removeAll(screen);
            } else {
                // new display, so create QMirClientScreen for it
                screen = new QMirClientScreen(output, mMirConnection);
                newScreenList.append(screen);
                qCDebug(mirclient) << "Added Screen with id" << mir_output_get_id(output)
                                         << "and geometry" << screen->geometry();
            }
            mScreenList.append(screen);
        }
    }

    // Announce old & unused Screens, should be deleted by the slot
    Q_FOREACH (const auto screen, oldScreenList) {
        Q_EMIT screenRemoved(screen);
    }

    /*
     * Mir's MirDisplayOutput does not include formFactor or scale for some reason, but Qt
     * will want that information on creating the QScreen. Only way we get that info is when
     * Mir positions a Window on that Screen. See "handleScreenPropertiesChange" method
     */

    // Announce new Screens
    Q_FOREACH (const auto screen, newScreenList) {
        Q_EMIT screenAdded(screen);
    }

    qCDebug(mirclient) << "=======================================";
    for (auto screen: mScreenList) {
        qCDebug(mirclient) << screen << "- id:" << screen->mirOutputId()
                                 << "geometry:" << screen->geometry()
                                 << "form factor:" << mirFormFactorToStr(screen->formFactor())
                                 << "scale:" << screen->scale();
    }
    qCDebug(mirclient) << "=======================================";
}

QMirClientScreen *QMirClientScreenObserver::findScreenWithId(int id)
{
    return findScreenWithId(mScreenList, id);
}

QMirClientScreen *QMirClientScreenObserver::findScreenWithId(const QList<QMirClientScreen *> &list, int id)
{
    Q_FOREACH (const auto screen, list) {
        if (screen->mirOutputId() == id) {
            return screen;
        }
    }
    return nullptr;
}

void QMirClientScreenObserver::handleScreenPropertiesChange(QMirClientScreen *screen, int dpi,
                                                        MirFormFactor formFactor, float scale)
{
    screen->setAdditionalMirDisplayProperties(scale, formFactor, dpi);
}

