/****************************************************************************
**
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt5.android;

import android.view.ActionMode;
import android.view.ActionMode.Callback;
import android.view.Menu;
import android.view.MenuItem;
import android.app.Activity;
import android.content.Context;
import android.content.res.TypedArray;

/**
 * The edit menu actions (when there is selection)
 */
class EditMenu implements ActionMode.Callback {

    private final Activity m_activity;
    private ActionMode m_actionMode;

    public EditMenu(Activity activity) {
        m_activity = activity;
    }

    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
        mode.setTitle(null);
        mode.setSubtitle(null);
        mode.setTitleOptionalHint(true);

        Context context = m_activity;
        int[] attrs = {
            android.R.attr.actionModeSelectAllDrawable, android.R.attr.actionModeCutDrawable,
            android.R.attr.actionModeCopyDrawable, android.R.attr.actionModePasteDrawable
        };
        TypedArray a = context.getTheme().obtainStyledAttributes(attrs);

        menu.add(Menu.NONE, android.R.id.selectAll, Menu.NONE, android.R.string.selectAll)
                .setIcon(a.getResourceId(0, 0))
                .setAlphabeticShortcut('a')
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS | MenuItem.SHOW_AS_ACTION_WITH_TEXT);

        menu.add(Menu.NONE, android.R.id.cut, Menu.NONE, android.R.string.cut)
                .setIcon(a.getResourceId(1, 0))
                .setAlphabeticShortcut('x')
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS | MenuItem.SHOW_AS_ACTION_WITH_TEXT);

        menu.add(Menu.NONE, android.R.id.copy, Menu.NONE, android.R.string.copy)
                .setIcon(a.getResourceId(2, 0))
                .setAlphabeticShortcut('c')
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS | MenuItem.SHOW_AS_ACTION_WITH_TEXT);

        menu.add(Menu.NONE, android.R.id.paste, Menu.NONE, android.R.string.paste)
                .setIcon(a.getResourceId(3, 0))
                .setAlphabeticShortcut('v')
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS | MenuItem.SHOW_AS_ACTION_WITH_TEXT);

        return true;
    }

    @Override
    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
        return true;
    }

    @Override
    public void onDestroyActionMode(ActionMode mode) {
        m_actionMode = null;
    }

    @Override
    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {

        switch (item.getItemId()) {
        case android.R.id.cut:
            return QtNativeInputConnection.cut();
        case android.R.id.copy:
            return QtNativeInputConnection.copy();
        case android.R.id.paste:
            return QtNativeInputConnection.paste();
        case android.R.id.selectAll:
            return QtNativeInputConnection.selectAll();
        }
        return false;
    }

    public void hide()
    {
        if (m_actionMode != null) {
            m_actionMode.finish();
        }
    }

    public void show()
    {
        if (m_actionMode == null) {
            m_actionMode = m_activity.startActionMode(this);
        }
    }

    public boolean isShown()
    {
        return m_actionMode != null;
    }
}
