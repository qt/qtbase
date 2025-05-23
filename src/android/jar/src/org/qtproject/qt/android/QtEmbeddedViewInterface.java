// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
};
