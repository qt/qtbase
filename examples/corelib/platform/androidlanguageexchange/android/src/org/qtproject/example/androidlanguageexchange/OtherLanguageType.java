// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.example.androidlanguageexchange;

public enum OtherLanguageType {
    Java(0),
    Kotlin(1);

    private final int code;

    OtherLanguageType(int code) {
        this.code = code;
    }

    public int getCode() {
        return code;
    }
}
