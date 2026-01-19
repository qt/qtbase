// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.example.tst_androidactivityalias;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;

import android.util.Log;

public class QtActivityTest extends org.qtproject.qt.android.bindings.QtActivity
{
    private boolean m_resultSet = false;
    private static final String EXTRA_FINISH_IMMEDIATELY = "finish_immediately";
    private static final String EXTRA_ALIAS_COMPONENT = "alias_component";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        // To avoid calling native methods in onDestroy()
        onRetainNonConfigurationInstance();
        finishIfRequested(getIntent());
    }

    @Override
    public void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
        finishIfRequested(intent);
    }

    private void finishIfRequested(Intent intent)
    {
        if (intent == null)
            return;

        if (intent.getBooleanExtra(EXTRA_FINISH_IMMEDIATELY, false))
            finish();
    }

    @Override
    public void finish()
    {
        if (!m_resultSet) {
            Intent resultIntent = new Intent();
            ComponentName component = getIntent() != null ? getIntent().getComponent() : null;
            if (component != null)
                resultIntent.putExtra(EXTRA_ALIAS_COMPONENT, component.getClassName());
            setResult(Activity.RESULT_OK, resultIntent);
            m_resultSet = true;
        }
        super.finish();
    }
}
