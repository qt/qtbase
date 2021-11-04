# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_commandline_option(libproxy TYPE boolean)
qt_commandline_option(dtls TYPE boolean)
qt_commandline_option(ocsp TYPE boolean)
qt_commandline_option(sctp TYPE boolean)
qt_commandline_option(securetransport TYPE boolean)
qt_commandline_option(schannel TYPE boolean)
qt_commandline_option(ssl TYPE boolean)
qt_commandline_option(system-proxies TYPE boolean)
qt_commandline_option(publicsuffix TYPE optionalString VALUES system qt no all)
