# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("sqlmodel" PUBLIC
    LABEL "SQL item models"
    PURPOSE "Provides item model classes backed by SQL databases."
    CONDITION QT_FEATURE_itemmodel
)
qt_configure_add_summary_section(NAME "Qt Sql")
qt_configure_add_summary_entry(ARGS "sqlmodel")
qt_configure_end_summary_section() # end of "Qt Sql" section
