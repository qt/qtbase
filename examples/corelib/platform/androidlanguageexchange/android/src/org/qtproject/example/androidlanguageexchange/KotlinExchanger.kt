// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.example.androidlanguageexchange;

class KotlinExchanger(private val cppExchanger: Long) {

    private external fun connectToCppExchanger(cppExchanger: Long)
    private external fun fromOther(cppExchanger: Long, str: String)

    init {
        connectToCppExchanger(cppExchanger)
    }

    fun fromCpp(type: Int, str: String) {
        if (type == OtherLanguageType.Kotlin.code) {
            val msg = "$str And hello back to you from\nKotlin!"
            fromOther(cppExchanger, msg)
        }
    }
}
