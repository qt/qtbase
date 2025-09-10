// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*!
    \macro QT_VERSION_CHECK(major, minor, patch)
    \relates <QtVersionChecks>

    Turns the \a major, \a minor and \a patch numbers of a version into an
    integer that encodes all three. When expressed in hexadecimal, this integer
    is of form \c 0xMMNNPP wherein \c{0xMM ==} \a major, \c{0xNN ==} \a minor,
    and \c{0xPP ==} \a patch. This can be compared with another similarly
    processed version ID.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qt-version-check

    \note the parameters are read as integers in the normal way, so should
    normally be written in decimal (so a \c 0x prefix must be used if writing
    them in hexadecimal). Thus \c{QT_VERSION_CHECK(5, 15, 0)} is equal to \c
    0x050f00, which could equally be written \c{QT_VERSION_CHECK(5, 0xf, 0)}.

    \sa QT_VERSION
*/

/*!
    \macro QT_VERSION
    \relates <QtVersionChecks>

    This macro expands to a numeric value of the same form as \l
    QT_VERSION_CHECK() constructs, that specifies the version of Qt with which
    code using it is compiled. For example, if you compile your application with
    Qt 6.1.2, the QT_VERSION macro will expand to \c 0x060102, the same as
    \c{QT_VERSION_CHECK(6, 1, 2)}. Note that this need not agree with the
    version the application will find itself using at \e runtime.

    You can use QT_VERSION to select the latest Qt features where available
    while falling back to older implementations otherwise. Using
    QT_VERSION_CHECK() for the value to compare with is recommended.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 16

    \sa QT_VERSION_STR, QT_VERSION_CHECK(), qVersion()
*/
