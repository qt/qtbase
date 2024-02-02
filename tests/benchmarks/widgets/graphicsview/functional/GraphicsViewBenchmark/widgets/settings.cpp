// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "settings.h"


Settings::Settings()
    : QObject(),
    m_scriptName(),
    m_outputFileName(),
    m_resultFormat(0),
    m_size(0,0),
    m_angle(0),
    m_listItemCount(200),
    m_options()
{
}

Settings::~Settings()
{
}
