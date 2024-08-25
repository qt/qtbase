# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_commandline_option(mysql_config TYPE string)
qt_commandline_option(psql_config TYPE string)
qt_commandline_option(sqlite CONTROLS_FEATURE TYPE enum NAME system-sqlite MAPPING qt no system yes)
qt_commandline_option(sql-db2 TYPE boolean)
qt_commandline_option(sql-ibase TYPE boolean)
qt_commandline_option(sql-mysql TYPE boolean)
qt_commandline_option(sql-oci TYPE boolean)
qt_commandline_option(sql-odbc TYPE boolean)
qt_commandline_option(sql-psql TYPE boolean)
qt_commandline_option(sql-sqlite TYPE boolean)
qt_commandline_option(sql-mimer TYPE boolean)
