// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "application.h"

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    m_actionManager = std::make_unique<ActionManager>();
}

ActionManager *Application::actionManager() const
{
    return m_actionManager.get();
}
