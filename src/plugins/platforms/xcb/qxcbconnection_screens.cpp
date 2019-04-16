/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbintegration.h"

#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtCore/QString>
#include <QtCore/QList>

#include <qpa/qwindowsysteminterface.h>

#include <xcb/xinerama.h>

void QXcbConnection::xrandrSelectEvents()
{
    xcb_screen_iterator_t rootIter = xcb_setup_roots_iterator(setup());
    for (; rootIter.rem; xcb_screen_next(&rootIter)) {
        xcb_randr_select_input(xcb_connection(),
            rootIter.data->root,
            XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE |
            XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE |
            XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE |
            XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY
        );
    }
}

QXcbScreen* QXcbConnection::findScreenForCrtc(xcb_window_t rootWindow, xcb_randr_crtc_t crtc) const
{
    for (QXcbScreen *screen : m_screens) {
        if (screen->root() == rootWindow && screen->crtc() == crtc)
            return screen;
    }

    return nullptr;
}

QXcbScreen* QXcbConnection::findScreenForOutput(xcb_window_t rootWindow, xcb_randr_output_t output) const
{
    for (QXcbScreen *screen : m_screens) {
        if (screen->root() == rootWindow && screen->output() == output)
            return screen;
    }

    return nullptr;
}

QXcbVirtualDesktop* QXcbConnection::virtualDesktopForRootWindow(xcb_window_t rootWindow) const
{
    for (QXcbVirtualDesktop *virtualDesktop : m_virtualDesktops) {
        if (virtualDesktop->screen()->root == rootWindow)
            return virtualDesktop;
    }

    return nullptr;
}

/*!
    \brief Synchronizes the screen list, adds new screens, removes deleted ones
*/
void QXcbConnection::updateScreens(const xcb_randr_notify_event_t *event)
{
    if (event->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
        xcb_randr_crtc_change_t crtc = event->u.cc;
        QXcbVirtualDesktop *virtualDesktop = virtualDesktopForRootWindow(crtc.window);
        if (!virtualDesktop)
            // Not for us
            return;

        QXcbScreen *screen = findScreenForCrtc(crtc.window, crtc.crtc);
        qCDebug(lcQpaScreen) << "QXcbConnection: XCB_RANDR_NOTIFY_CRTC_CHANGE:" << crtc.crtc
                             << "mode" << crtc.mode << "relevant screen" << screen;
        // Only update geometry when there's a valid mode on the CRTC
        // CRTC with node mode could mean that output has been disabled, and we'll
        // get RRNotifyOutputChange notification for that.
        if (screen && crtc.mode) {
            if (crtc.rotation == XCB_RANDR_ROTATION_ROTATE_90 ||
                crtc.rotation == XCB_RANDR_ROTATION_ROTATE_270)
                std::swap(crtc.width, crtc.height);
            screen->updateGeometry(QRect(crtc.x, crtc.y, crtc.width, crtc.height), crtc.rotation);
            if (screen->mode() != crtc.mode)
                screen->updateRefreshRate(crtc.mode);
        }

    } else if (event->subCode == XCB_RANDR_NOTIFY_OUTPUT_CHANGE) {
        xcb_randr_output_change_t output = event->u.oc;
        QXcbVirtualDesktop *virtualDesktop = virtualDesktopForRootWindow(output.window);
        if (!virtualDesktop)
            // Not for us
            return;

        QXcbScreen *screen = findScreenForOutput(output.window, output.output);
        qCDebug(lcQpaScreen) << "QXcbConnection: XCB_RANDR_NOTIFY_OUTPUT_CHANGE:" << output.output;

        if (screen && output.connection == XCB_RANDR_CONNECTION_DISCONNECTED) {
            qCDebug(lcQpaScreen) << "screen" << screen->name() << "has been disconnected";
            destroyScreen(screen);
        } else if (!screen && output.connection == XCB_RANDR_CONNECTION_CONNECTED) {
            // New XRandR output is available and it's enabled
            if (output.crtc != XCB_NONE && output.mode != XCB_NONE) {
                auto outputInfo = Q_XCB_REPLY(xcb_randr_get_output_info, xcb_connection(),
                                              output.output, output.config_timestamp);
                // Find a fake screen
                const auto scrs = virtualDesktop->screens();
                for (QPlatformScreen *scr : scrs) {
                    QXcbScreen *xcbScreen = static_cast<QXcbScreen *>(scr);
                    if (xcbScreen->output() == XCB_NONE) {
                        screen = xcbScreen;
                        break;
                    }
                }

                if (screen) {
                    QString nameWas = screen->name();
                    // Transform the fake screen into a physical screen
                    screen->setOutput(output.output, outputInfo.get());
                    updateScreen(screen, output);
                    qCDebug(lcQpaScreen) << "output" << screen->name()
                                         << "is connected and enabled; was fake:" << nameWas;
                } else {
                    screen = createScreen(virtualDesktop, output, outputInfo.get());
                    qCDebug(lcQpaScreen) << "output" << screen->name() << "is connected and enabled";
                }
                QHighDpiScaling::updateHighDpiScaling();
            }
        } else if (screen) {
            if (output.crtc == XCB_NONE && output.mode == XCB_NONE) {
                // Screen has been disabled
                auto outputInfo = Q_XCB_REPLY(xcb_randr_get_output_info, xcb_connection(),
                                              output.output, output.config_timestamp);
                if (outputInfo->crtc == XCB_NONE) {
                    qCDebug(lcQpaScreen) << "output" << screen->name() << "has been disabled";
                    destroyScreen(screen);
                } else {
                    qCDebug(lcQpaScreen) << "output" << screen->name() << "has been temporarily disabled for the mode switch";
                    // Reset crtc to skip RRCrtcChangeNotify events,
                    // because they may be invalid in the middle of the mode switch
                    screen->setCrtc(XCB_NONE);
                }
            } else {
                updateScreen(screen, output);
                qCDebug(lcQpaScreen) << "output has changed" << screen;
            }
        }

        qCDebug(lcQpaScreen) << "primary output is" << qAsConst(m_screens).first()->name();
    }
}

bool QXcbConnection::checkOutputIsPrimary(xcb_window_t rootWindow, xcb_randr_output_t output)
{
    auto primary = Q_XCB_REPLY(xcb_randr_get_output_primary, xcb_connection(), rootWindow);
    if (!primary)
        qWarning("failed to get the primary output of the screen");

    const bool isPrimary = primary ? (primary->output == output) : false;

    return isPrimary;
}

void QXcbConnection::updateScreen(QXcbScreen *screen, const xcb_randr_output_change_t &outputChange)
{
    screen->setCrtc(outputChange.crtc); // Set the new crtc, because it can be invalid
    screen->updateGeometry(outputChange.config_timestamp);
    if (screen->mode() != outputChange.mode)
        screen->updateRefreshRate(outputChange.mode);
    // Only screen which belongs to the primary virtual desktop can be a primary screen
    if (screen->screenNumber() == primaryScreenNumber()) {
        if (!screen->isPrimary() && checkOutputIsPrimary(outputChange.window, outputChange.output)) {
            screen->setPrimary(true);

            // If the screen became primary, reshuffle the order in QGuiApplicationPrivate
            const int idx = m_screens.indexOf(screen);
            if (idx > 0) {
                qAsConst(m_screens).first()->setPrimary(false);
                m_screens.swapItemsAt(0, idx);
            }
            screen->virtualDesktop()->setPrimaryScreen(screen);
            QWindowSystemInterface::handlePrimaryScreenChanged(screen);
        }
    }
}

QXcbScreen *QXcbConnection::createScreen(QXcbVirtualDesktop *virtualDesktop,
                                         const xcb_randr_output_change_t &outputChange,
                                         xcb_randr_get_output_info_reply_t *outputInfo)
{
    QXcbScreen *screen = new QXcbScreen(this, virtualDesktop, outputChange.output, outputInfo);
    // Only screen which belongs to the primary virtual desktop can be a primary screen
    if (screen->screenNumber() == primaryScreenNumber())
        screen->setPrimary(checkOutputIsPrimary(outputChange.window, outputChange.output));

    if (screen->isPrimary()) {
        if (!m_screens.isEmpty())
            qAsConst(m_screens).first()->setPrimary(false);

        m_screens.prepend(screen);
    } else {
        m_screens.append(screen);
    }
    virtualDesktop->addScreen(screen);
    QWindowSystemInterface::handleScreenAdded(screen, screen->isPrimary());

    return screen;
}

void QXcbConnection::destroyScreen(QXcbScreen *screen)
{
    QXcbVirtualDesktop *virtualDesktop = screen->virtualDesktop();
    if (virtualDesktop->screens().count() == 1) {
        // If there are no other screens on the same virtual desktop,
        // then transform the physical screen into a fake screen.
        const QString nameWas = screen->name();
        screen->setOutput(XCB_NONE, nullptr);
        qCDebug(lcQpaScreen) << "transformed" << nameWas << "to fake" << screen;
    } else {
        // There is more than one screen on the same virtual desktop, remove the screen
        m_screens.removeOne(screen);
        virtualDesktop->removeScreen(screen);

        // When primary screen is removed, set the new primary screen
        // which belongs to the primary virtual desktop.
        if (screen->isPrimary()) {
            QXcbScreen *newPrimary = static_cast<QXcbScreen *>(virtualDesktop->screens().at(0));
            newPrimary->setPrimary(true);
            const int idx = m_screens.indexOf(newPrimary);
            if (idx > 0)
                m_screens.swapItemsAt(0, idx);
            QWindowSystemInterface::handlePrimaryScreenChanged(newPrimary);
        }

        QWindowSystemInterface::handleScreenRemoved(screen);
    }
}

void QXcbConnection::initializeScreens()
{
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup());
    int xcbScreenNumber = 0;    // screen number in the xcb sense
    QXcbScreen *primaryScreen = nullptr;
    while (it.rem) {
        // Each "screen" in xcb terminology is a virtual desktop,
        // potentially a collection of separate juxtaposed monitors.
        // But we want a separate QScreen for each output (e.g. DVI-I-1, VGA-1, etc.)
        // which will become virtual siblings.
        xcb_screen_t *xcbScreen = it.data;
        QXcbVirtualDesktop *virtualDesktop = new QXcbVirtualDesktop(this, xcbScreen, xcbScreenNumber);
        m_virtualDesktops.append(virtualDesktop);
        QList<QPlatformScreen *> siblings;
        if (hasXRandr()) {
            // RRGetScreenResourcesCurrent is fast but it may return nothing if the
            // configuration is not initialized wrt to the hardware. We should call
            // RRGetScreenResources in this case.
            auto resources_current = Q_XCB_REPLY(xcb_randr_get_screen_resources_current,
                                                 xcb_connection(), xcbScreen->root);
            if (!resources_current) {
                qWarning("failed to get the current screen resources");
            } else {
                xcb_timestamp_t timestamp = 0;
                xcb_randr_output_t *outputs = nullptr;
                int outputCount = xcb_randr_get_screen_resources_current_outputs_length(resources_current.get());
                if (outputCount) {
                    timestamp = resources_current->config_timestamp;
                    outputs = xcb_randr_get_screen_resources_current_outputs(resources_current.get());
                } else {
                    auto resources = Q_XCB_REPLY(xcb_randr_get_screen_resources,
                                                 xcb_connection(), xcbScreen->root);
                    if (!resources) {
                        qWarning("failed to get the screen resources");
                    } else {
                        timestamp = resources->config_timestamp;
                        outputCount = xcb_randr_get_screen_resources_outputs_length(resources.get());
                        outputs = xcb_randr_get_screen_resources_outputs(resources.get());
                    }
                }

                if (outputCount) {
                    auto primary = Q_XCB_REPLY(xcb_randr_get_output_primary, xcb_connection(), xcbScreen->root);
                    if (!primary) {
                        qWarning("failed to get the primary output of the screen");
                    } else {
                        for (int i = 0; i < outputCount; i++) {
                            auto output = Q_XCB_REPLY_UNCHECKED(xcb_randr_get_output_info,
                                                                xcb_connection(), outputs[i], timestamp);
                            // Invalid, disconnected or disabled output
                            if (!output)
                                continue;

                            if (output->connection != XCB_RANDR_CONNECTION_CONNECTED) {
                                qCDebug(lcQpaScreen, "Output %s is not connected", qPrintable(
                                            QString::fromUtf8((const char*)xcb_randr_get_output_info_name(output.get()),
                                                              xcb_randr_get_output_info_name_length(output.get()))));
                                continue;
                            }

                            if (output->crtc == XCB_NONE) {
                                qCDebug(lcQpaScreen, "Output %s is not enabled", qPrintable(
                                            QString::fromUtf8((const char*)xcb_randr_get_output_info_name(output.get()),
                                                              xcb_randr_get_output_info_name_length(output.get()))));
                                continue;
                            }

                            QXcbScreen *screen = new QXcbScreen(this, virtualDesktop, outputs[i], output.get());
                            siblings << screen;
                            m_screens << screen;

                            // There can be multiple outputs per screen, use either
                            // the first or an exact match.  An exact match isn't
                            // always available if primary->output is XCB_NONE
                            // or currently disconnected output.
                            if (primaryScreenNumber() == xcbScreenNumber) {
                                if (!primaryScreen || (primary && outputs[i] == primary->output)) {
                                    if (primaryScreen)
                                        primaryScreen->setPrimary(false);
                                    primaryScreen = screen;
                                    primaryScreen->setPrimary(true);
                                    siblings.prepend(siblings.takeLast());
                                }
                            }
                        }
                    }
                }
            }
        } else if (hasXinerama()) {
            // Xinerama is available
            auto screens = Q_XCB_REPLY(xcb_xinerama_query_screens, xcb_connection());
            if (screens) {
                xcb_xinerama_screen_info_iterator_t it = xcb_xinerama_query_screens_screen_info_iterator(screens.get());
                while (it.rem) {
                    xcb_xinerama_screen_info_t *screen_info = it.data;
                    QXcbScreen *screen = new QXcbScreen(this, virtualDesktop,
                                                        XCB_NONE, nullptr,
                                                        screen_info, it.index);
                    siblings << screen;
                    m_screens << screen;
                    xcb_xinerama_screen_info_next(&it);
                }
            }
        }
        if (siblings.isEmpty()) {
            // If there are no XRandR outputs or XRandR extension is missing,
            // then create a fake/legacy screen.
            QXcbScreen *screen = new QXcbScreen(this, virtualDesktop, XCB_NONE, nullptr);
            qCDebug(lcQpaScreen) << "created fake screen" << screen;
            m_screens << screen;
            if (primaryScreenNumber() == xcbScreenNumber) {
                primaryScreen = screen;
                primaryScreen->setPrimary(true);
            }
            siblings << screen;
        }
        virtualDesktop->setScreens(std::move(siblings));
        xcb_screen_next(&it);
        ++xcbScreenNumber;
    } // for each xcb screen

    for (QXcbVirtualDesktop *virtualDesktop : qAsConst(m_virtualDesktops))
        virtualDesktop->subscribeToXFixesSelectionNotify();

    if (m_virtualDesktops.isEmpty()) {
        qFatal("QXcbConnection: no screens available");
    } else {
        // Ensure the primary screen is first on the list
        if (primaryScreen) {
            if (qAsConst(m_screens).first() != primaryScreen) {
                m_screens.removeOne(primaryScreen);
                m_screens.prepend(primaryScreen);
            }
        }

        // Push the screens to QGuiApplication
        for (QXcbScreen *screen : qAsConst(m_screens)) {
            qCDebug(lcQpaScreen) << "adding" << screen << "(Primary:" << screen->isPrimary() << ")";
            QWindowSystemInterface::handleScreenAdded(screen, screen->isPrimary());
        }

        qCDebug(lcQpaScreen) << "primary output is" << qAsConst(m_screens).first()->name();
    }
}
