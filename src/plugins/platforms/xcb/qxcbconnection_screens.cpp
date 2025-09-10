// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbintegration.h"

#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtCore/QString>
#include <QtCore/QList>

#include <qpa/qwindowsysteminterface.h>

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
        if (screen->root() == rootWindow) {
            if (screen->m_monitor) {
                if (screen->crtcs().contains(crtc))
                    return screen;
            } else {
                if (screen->crtc() == crtc)
                    return screen;
            }
        }
    }

    return nullptr;
}

QXcbScreen* QXcbConnection::findScreenForOutput(xcb_window_t rootWindow, xcb_randr_output_t output) const
{
    for (QXcbScreen *screen : m_screens) {
        if (screen->root() == rootWindow) {
            if (screen->m_monitor) {
                if (screen->outputs().contains(output))
                    return screen;
            } else {
                if (screen->output() == output)
                    return screen;
            }
        }
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
            }
        } else if (screen) {
            if (output.crtc == XCB_NONE && output.mode == XCB_NONE) {
                // Screen has been disabled
                auto outputInfo = Q_XCB_REPLY(xcb_randr_get_output_info, xcb_connection(),
                                              output.output, output.config_timestamp);
                if (!outputInfo || outputInfo->crtc == XCB_NONE) {
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

        qCDebug(lcQpaScreen) << "updateScreens: primary output is" << std::as_const(m_screens).first()->name();
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
                std::as_const(m_screens).first()->setPrimary(false);
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
            std::as_const(m_screens).first()->setPrimary(false);

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
    if (virtualDesktop->screens().size() == 1) {
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

        qCDebug(lcQpaScreen) << "destroyScreen: destroy" << screen;
        QWindowSystemInterface::handleScreenRemoved(screen);
    }
}

QXcbVirtualDesktop *QXcbConnection::virtualDesktopForNumber(int n) const
{
    for (QXcbVirtualDesktop *virtualDesktop : m_virtualDesktops) {
        if (virtualDesktop->number() == n)
            return virtualDesktop;
    }

    return nullptr;
}

QXcbScreen *QXcbConnection::findScreenForMonitorInfo(const QList<QPlatformScreen *> &screens, xcb_randr_monitor_info_t *monitorInfo)
{
    for (int i = 0; i < screens.size(); ++i) {
        auto s = static_cast<QXcbScreen*>(screens[i]);
        if (monitorInfo) {
            QByteArray ba2 = atomName(monitorInfo->name);
            if (s->name().toLocal8Bit() == ba2)
                return s;
        }
    }

    return nullptr;
}

void QXcbConnection::initializeScreens(bool initialized)
{
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup());
    int xcbScreenNumber = 0;    // screen number in the xcb sense
    QXcbScreen *primaryScreen = nullptr;
    if (isAtLeastXRandR15() && initialized)
        m_screens.clear();

    while (it.rem) {
        if (isAtLeastXRandR15())
            initializeScreensFromMonitor(&it, xcbScreenNumber, &primaryScreen, initialized);
        else if (isAtLeastXRandR12())
            initializeScreensFromOutput(&it, xcbScreenNumber, &primaryScreen);
        else {
            qWarning("There is no XRandR 1.2 and later version available. There will be only fake screen(s) to use.");
            initializeScreensWithoutXRandR(&it, xcbScreenNumber, &primaryScreen);
        }

        xcb_screen_next(&it);
        ++xcbScreenNumber;
    }

    for (QXcbVirtualDesktop *virtualDesktop : std::as_const(m_virtualDesktops))
        virtualDesktop->subscribeToXFixesSelectionNotify();

    if (m_virtualDesktops.isEmpty()) {
        qFatal("QXcbConnection: no screens available");
    } else {
        // Ensure the primary screen is first on the list
        if (primaryScreen) {
            if (std::as_const(m_screens).first() != primaryScreen) {
                m_screens.removeOne(primaryScreen);
                m_screens.prepend(primaryScreen);
            }
        }

        // Push the screens to QGuiApplication
        if (!initialized) {
            for (QXcbScreen *screen : std::as_const(m_screens)) {
                qCDebug(lcQpaScreen) << "adding" << screen << "(Primary:" << screen->isPrimary() << ")";
                QWindowSystemInterface::handleScreenAdded(screen, screen->isPrimary());
            }
        }

        if (!m_screens.isEmpty())
            qCDebug(lcQpaScreen) << "initializeScreens: primary output is" << std::as_const(m_screens).first()->name();
    }
}

void QXcbConnection::initializeScreensWithoutXRandR(xcb_screen_iterator_t *it, int xcbScreenNumber, QXcbScreen **primaryScreen)
{
    // XRandR extension is missing, then create a fake/legacy screen.
    xcb_screen_t *xcbScreen = it->data;
    QXcbVirtualDesktop *virtualDesktop = new QXcbVirtualDesktop(this, xcbScreen, xcbScreenNumber);
    m_virtualDesktops.append(virtualDesktop);
    QList<QPlatformScreen *> siblings;

    QXcbScreen *screen = new QXcbScreen(this, virtualDesktop, XCB_NONE, nullptr);
    qCDebug(lcQpaScreen) << "created fake screen" << screen;
    m_screens << screen;

    if (primaryScreenNumber() == xcbScreenNumber) {
        *primaryScreen = screen;
        (*primaryScreen)->setPrimary(true);
    }
    siblings << screen;
    virtualDesktop->setScreens(std::move(siblings));
}

void QXcbConnection::initializeScreensFromOutput(xcb_screen_iterator_t *it, int xcbScreenNumber, QXcbScreen **primaryScreen)
{
    // Each "screen" in xcb terminology is a virtual desktop,
    // potentially a collection of separate juxtaposed monitors.
    // But we want a separate QScreen for each output (e.g. DVI-I-1, VGA-1, etc.)
    // which will become virtual siblings.
    xcb_screen_t *xcbScreen = it->data;
    QXcbVirtualDesktop *virtualDesktop = new QXcbVirtualDesktop(this, xcbScreen, xcbScreenNumber);
    m_virtualDesktops.append(virtualDesktop);
    QList<QPlatformScreen *> siblings;
    if (isAtLeastXRandR12()) {
        // RRGetScreenResourcesCurrent is fast but it may return nothing if the
        // configuration is not initialized wrt to the hardware. We should call
        // RRGetScreenResources in this case.
        auto resources_current = Q_XCB_REPLY(xcb_randr_get_screen_resources_current,
                                                xcb_connection(), xcbScreen->root);
        decltype(Q_XCB_REPLY(xcb_randr_get_screen_resources,
                                xcb_connection(), xcbScreen->root)) resources;
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
                resources = Q_XCB_REPLY(xcb_randr_get_screen_resources,
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
                            if (!(*primaryScreen) || (primary && outputs[i] == primary->output)) {
                                if (*primaryScreen)
                                    (*primaryScreen)->setPrimary(false);
                                *primaryScreen = screen;
                                (*primaryScreen)->setPrimary(true);
                                siblings.prepend(siblings.takeLast());
                            }
                        }
                    }
                }
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
            *primaryScreen = screen;
            (*primaryScreen)->setPrimary(true);
        }
        siblings << screen;
    }
    virtualDesktop->setScreens(std::move(siblings));
}

void QXcbConnection::initializeScreensFromMonitor(xcb_screen_iterator_t *it, int xcbScreenNumber, QXcbScreen **primaryScreen, bool initialized)
{
    // Each "screen" in xcb terminology is a virtual desktop,
    // potentially a collection of separate juxtaposed monitors.
    // But we want a separate QScreen for each output (e.g. DVI-I-1, VGA-1, etc.)
    // which will become virtual siblings.
    xcb_screen_t *xcbScreen = it->data;
    QXcbVirtualDesktop *virtualDesktop = nullptr;
    if (initialized)
        virtualDesktop = virtualDesktopForNumber(xcbScreenNumber);
    if (!virtualDesktop) {
        virtualDesktop = new QXcbVirtualDesktop(this, xcbScreen, xcbScreenNumber);
        m_virtualDesktops.append(virtualDesktop);
    }
    QList<QPlatformScreen *> old = virtualDesktop->m_screens;
    QList<QXcbScreen *> newScreens;

    QList<QPlatformScreen *> siblings;

    xcb_randr_get_monitors_cookie_t monitors_c = xcb_randr_get_monitors(xcb_connection(), xcbScreen->root, 1);
    xcb_randr_get_monitors_reply_t *monitors_r = xcb_randr_get_monitors_reply(xcb_connection(), monitors_c, nullptr);

    if (!monitors_r) {
        qWarning("RANDR GetMonitors failed; this should not be possible");
        return;
    }

    xcb_randr_monitor_info_iterator_t monitor_iter = xcb_randr_get_monitors_monitors_iterator(monitors_r);
    while (monitor_iter.rem) {
        xcb_randr_monitor_info_t *monitor_info = monitor_iter.data;
        QXcbScreen *screen = nullptr;
        if (!initialized) {
            screen = new QXcbScreen(this, virtualDesktop, monitor_info, monitors_r->timestamp);
        } else {
            screen = findScreenForMonitorInfo(old, monitor_info);
            if (!screen) {
                screen = new QXcbScreen(this, virtualDesktop, monitor_info, monitors_r->timestamp);
                newScreens.append(screen);
            } else {
                screen->setMonitor(monitor_info, monitors_r->timestamp);
                old.removeAll(screen);
            }
        }

        if (screen->isPrimary()) {
            siblings.prepend (screen);
            if (primaryScreenNumber() == xcbScreenNumber) {
                if (*primaryScreen)
                    (*primaryScreen)->setPrimary(false);
                *primaryScreen = screen;
                (*primaryScreen)->setPrimary(true);
            } else { // Only screens on the main virtual desktop can be primary
                screen->setPrimary(false);
            }
        } else {
            siblings.append(screen);
        }

        xcb_randr_monitor_info_next(&monitor_iter);
    }
    free(monitors_r);

    if (siblings.isEmpty()) {
        QXcbScreen *screen = nullptr;
        if (initialized && !old.isEmpty()) {
            // If there are no other screens on the same virtual desktop,
            // then transform the physical screen into a fake screen.
            screen = static_cast<QXcbScreen *>(old.takeFirst());
            const QString nameWas = screen->name();
            screen->setMonitor(nullptr, XCB_NONE);
            qCDebug(lcQpaScreen) << "transformed" << nameWas << "to fake" << screen;
        } else {
            // If there are no XRandR outputs or XRandR extension is missing,
            // then create a fake/legacy screen.
            screen = new QXcbScreen(this, virtualDesktop, nullptr);
            qCDebug(lcQpaScreen) << "create a fake screen: " << screen;
        }

        siblings << screen;
    }

    if (primaryScreenNumber() == xcbScreenNumber) {
        // If no screen was reported to be primary, use the first one
        if (!*primaryScreen) {
            (*primaryScreen) = static_cast<QXcbScreen *>(siblings.first());
            (*primaryScreen)->setPrimary(true);
        }

        // Prepand the siblings to the current list of screens
        QList<QXcbScreen *> new_m_screens;
        new_m_screens.reserve( siblings.size() + m_screens.size() );
        for (QPlatformScreen *ps:siblings) {
            new_m_screens.append(static_cast<QXcbScreen *>(ps));
        }
        new_m_screens.append(std::move(m_screens));
        m_screens = std::move(new_m_screens);
    } else {
        for (QPlatformScreen *ps:siblings) {
            m_screens.append(static_cast<QXcbScreen *>(ps));
        }
    }

    if (initialized) {
        if (primaryScreenNumber() == xcbScreenNumber && !newScreens.contains(*primaryScreen)) {
            qCDebug(lcQpaScreen) << "assigning screen as primary: " << *primaryScreen;
            virtualDesktop->setPrimaryScreen(*primaryScreen);
            QWindowSystemInterface::handlePrimaryScreenChanged(*primaryScreen);
        }

        for (QXcbScreen *screen: newScreens) {
            qCDebug(lcQpaScreen) << "adding screen: " << screen << "(Primary:" << screen->isPrimary() << ")";
            virtualDesktop->addScreen(screen);
            QWindowSystemInterface::handleScreenAdded(screen, screen->isPrimary());
        }

        for (QPlatformScreen *ps : old) {
            qCDebug(lcQpaScreen) << "destroy screen: " << ps;
            QWindowSystemInterface::handleScreenRemoved(ps);
            virtualDesktop->removeScreen(ps);
        }
    }

    virtualDesktop->setScreens(std::move(siblings));
}
