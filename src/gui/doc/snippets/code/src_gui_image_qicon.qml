// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls

Window {

//! [iconFont]
ToolButton {
    id: muteButton
    checkable: true
    icon.name: checked ? "volume_off" : "volume_up"
}
//! [iconFont]

}
