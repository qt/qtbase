// Copyright (C) 2026 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.icu.text.CollationKey;
import android.icu.text.Collator;
import java.util.Locale;

class QtCollator
{
    static Collator getCollator(String bcp47Name, int strength)
    {
        Locale locale = Locale.forLanguageTag(bcp47Name);
        Collator c = Collator.getInstance(locale);
        // Match qcollator_icu.cpp which enables UCOL_NORMALIZATION_MODE on its native path.
        c.setDecomposition(Collator.CANONICAL_DECOMPOSITION);
        if (strength >= 0) {
            c.setStrength(strength);
        }
        return c;
    }

    static byte[] getCollationKey(Collator collator, String s)
    {
        return collator.getCollationKey(s).toByteArray();
    }
}
