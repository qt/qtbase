// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.view.WindowInsets;
import android.view.WindowInsetsAnimation;
import android.widget.EditText;

import java.util.List;

public final class TestEditText extends EditText
{
    public volatile boolean m_imeAnimationFinished;

    public TestEditText(Context context)
    {
        super(context);
        setWindowInsetsAnimationCallback(new WindowInsetsAnimation.Callback(
                WindowInsetsAnimation.Callback.DISPATCH_MODE_CONTINUE_ON_SUBTREE) {
            @Override
            public WindowInsets onProgress(WindowInsets insets,
                                           List<WindowInsetsAnimation> animations)
            {
                return insets;
            }

            @Override
            public void onEnd(WindowInsetsAnimation animation)
            {
                if ((animation.getTypeMask() & WindowInsets.Type.ime()) != 0)
                    m_imeAnimationFinished = true;
            }
        });
    }
}
