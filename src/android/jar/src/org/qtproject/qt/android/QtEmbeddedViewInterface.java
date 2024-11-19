// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

/**
 * QtEmbeddedViewInterface is intended to encapsulate the needs of QtView, so that the Activity and
 * Service implementations of these functions may be split clearly, and the interface can be stored
 * and used conveniently in QtView.
**/
interface QtEmbeddedViewInterface {
    void startQtApplication(String appParams, String mainLib);
    void addView(QtView view);
    void removeView(QtView view);
}
