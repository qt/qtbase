// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

public class QtConstants {
    public static final String ERROR_CODE_KEY = "error.code";
    public static final String ERROR_MESSAGE_KEY = "error.message";
    public static final String DEX_PATH_KEY = "dex.path";
    public static final String LIB_PATH_KEY = "lib.path";
    public static final String LOADER_CLASS_NAME_KEY = "loader.class.name";
    public static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    public static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    public static final String APPLICATION_PARAMETERS_KEY = "application.parameters";
    public static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    public static final String MAIN_LIBRARY_KEY = "main.library";
    public static final String STATIC_INIT_CLASSES_KEY = "static.init.classes";
    public static final String EXTRACT_STYLE_KEY = "extract.android.style";
    public static final String EXTRACT_STYLE_MINIMAL_KEY = "extract.android.style.option";

    // Application states
    public static class ApplicationState {
        public static final int ApplicationSuspended = 0x0;
        public static final int ApplicationHidden = 0x1;
        public static final int ApplicationInactive = 0x2;
        public static final int ApplicationActive = 0x4;
    }
}
