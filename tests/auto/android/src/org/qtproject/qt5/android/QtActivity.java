/*
    Copyright (c) 2012, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt-project.org/legal

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.qtproject.qt5.android;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;

import org.qtproject.qt5.android.tests.R;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.method.MetaKeyKeyListener;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

public class QtActivity extends Activity {
    private int m_id =- 1;
    private boolean softwareKeyboardIsVisible = false;
    private long m_metaState;
    private int m_lastChar = 0;
    private boolean m_fullScreen = false;
    private boolean m_started = false;
    private QtSurface m_surface = null;
    private boolean m_usesGL = false;
    private void loadQtLibs(String[] libs, String environment, String params, String mainLib, String nativeLibDir) throws Exception
    {
        QtNative.loadQtLibraries(libs);
        // start application

        final String envPaths = "NECESSITAS_API_LEVEL=2\tHOME=" + getDir("files", MODE_WORLD_WRITEABLE).getAbsolutePath() +
                              "\tTMPDIR=" + getDir("files", MODE_WORLD_WRITEABLE).getAbsolutePath() +
                              "\tCACHE_PATH=" + getDir("files", MODE_WORLD_WRITEABLE).getAbsolutePath();
        if (environment != null && environment.length() > 0)
            environment = envPaths + "\t" + environment;
        else
            environment = envPaths;

        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        QtNative.startApplication(params, environment, mainLib, nativeLibDir);
        m_surface.applicationStarted(m_usesGL);
        m_started = true;
    }

    private boolean m_quitApp = true;
    private Process m_debuggerProcess = null; // debugger process

    private void startApp(final boolean firstStart)
    {
        try {
            String qtLibs[] = getResources().getStringArray(R.array.qt_libs);
            ArrayList<String> libraryList = new ArrayList<String>();
            for (int i = 0; i < qtLibs.length; i++)
                libraryList.add("/data/local/tmp/qt/lib/lib" + qtLibs[i] + ".so");

            String mainLib = null;
            String nativeLibDir = null;
            if (getIntent().getExtras() != null) {
                if (getIntent().getExtras().containsKey("extra_libs")) {
                    String extra_libs = getIntent().getExtras().getString("extra_libs");
                    for (String lib : extra_libs.split(":"))
                        libraryList.add(lib);
                }
                if (getIntent().getExtras().containsKey("lib_name")) {
                    mainLib = getIntent().getExtras().getString("lib_name");
                    int slash = mainLib.lastIndexOf("/");
                    if (slash >= 0) {
                        nativeLibDir = mainLib.substring(0, slash+1);
                        mainLib = mainLib.substring(slash+1+3, mainLib.length()-3); //remove lib and .so
                    } else {
                        nativeLibDir = "";
                    }
                }

                if (getIntent().getExtras().containsKey("needsOpenGl"))
                    m_usesGL = getIntent().getExtras().getBoolean("needsOpenGl");
            } else {
                try {
                    Thread.sleep(5000);
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                System.exit(0);
            }
            String[] libs = new String[libraryList.size()];
            libs = libraryList.toArray(libs);
            loadQtLibs(libs
                        ,"QT_QPA_EGLFS_HIDECURSOR=1\tQML2_IMPORT_PATH=/data/local/tmp/qt/qml\tQML_IMPORT_PATH=/data/local/tmp/qt/imports\tQT_PLUGIN_PATH=/data/local/tmp/qt/plugins"
                        , "-xml\t-silent\t-o\toutput.xml", mainLib, nativeLibDir);
        } catch (Exception e) {
            Log.e(QtNative.QtTAG, "Can't create main activity", e);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        getDir("files", MODE_WORLD_WRITEABLE);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        m_quitApp = true;
        QtNative.setMainActivity(this);
        if (null == getLastNonConfigurationInstance()) {
            DisplayMetrics metrics = new DisplayMetrics();
            getWindowManager().getDefaultDisplay().getMetrics(metrics);
            QtNative.setApplicationDisplayMetrics(metrics.widthPixels, metrics.heightPixels,
                            metrics.widthPixels, metrics.heightPixels,
                            metrics.xdpi, metrics.ydpi);
        }
        m_surface = new QtSurface(this, m_id);
        setContentView(m_surface);
        if (null == getLastNonConfigurationInstance())
            startApp(true);
    }

    public QtSurface getQtSurface()
    {
        return m_surface;
    }

    @Override
    public Object onRetainNonConfigurationInstance()
    {
        super.onRetainNonConfigurationInstance();
        m_quitApp = false;
        return true;
    }

    @Override
    protected void onDestroy()
    {
        QtNative.setMainActivity(null);
        super.onDestroy();
        if (m_quitApp) {
            Log.i(QtNative.QtTAG, "onDestroy");
            if (m_debuggerProcess != null)
                m_debuggerProcess.destroy();
            System.exit(0);// FIXME remove it or find a better way
        }
        QtNative.setMainActivity(null);
    }

    @Override
    protected void onResume()
    {
        // fire all lostActions
        synchronized (QtNative.m_mainActivityMutex) {
            Iterator<Runnable> itr = QtNative.getLostActions().iterator();
            while (itr.hasNext())
                runOnUiThread(itr.next());
            if (m_started) {
                QtNative.clearLostActions();
                QtNative.updateWindow();
            }
        }
        super.onResume();
    }

    public void redrawWindow(int left, int top, int right, int bottom)
    {
        m_surface.drawBitmap(new Rect(left, top, right, bottom));
    }

    public void setFullScreen(boolean enterFullScreen)
    {
        if (m_fullScreen == enterFullScreen)
            return;
        if (m_fullScreen = enterFullScreen)
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        else
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState)
    {
        super.onSaveInstanceState(outState);
        outState.putBoolean("FullScreen", m_fullScreen);
        outState.putBoolean("Started", m_started);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState)
    {
        super.onRestoreInstanceState(savedInstanceState);
        setFullScreen(savedInstanceState.getBoolean("FullScreen"));
        m_started = savedInstanceState.getBoolean("Started");
        if (m_started)
            m_surface.applicationStarted(true);
    }

    public void showSoftwareKeyboard()
    {
        softwareKeyboardIsVisible = true;
        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, InputMethodManager.HIDE_IMPLICIT_ONLY);
    }

    public void resetSoftwareKeyboard()
    {
    }

    public void hideSoftwareKeyboard()
    {
        if (softwareKeyboardIsVisible) {
            InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.toggleSoftInput(0, 0);
        }
        softwareKeyboardIsVisible = false;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (m_started && event.getAction() == KeyEvent.ACTION_MULTIPLE &&
            event.getCharacters() != null &&
            event.getCharacters().length() == 1 &&
            event.getKeyCode() == 0) {
            Log.i(QtNative.QtTAG, "dispatchKeyEvent at MULTIPLE with one character: " + event.getCharacters());
            QtNative.keyDown(0, event.getCharacters().charAt(0), event.getMetaState());
            QtNative.keyUp(0, event.getCharacters().charAt(0), event.getMetaState());
        }

        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;
        m_metaState = MetaKeyKeyListener.handleKeyDown(m_metaState, keyCode, event);
        int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(m_metaState));
        int lc = c;
        m_metaState = MetaKeyKeyListener.adjustMetaAfterKeypress(m_metaState);

        if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0) {
            c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
            int composed = KeyEvent.getDeadChar(m_lastChar, c);
            c = composed;
        }
        m_lastChar = lc;
        if (keyCode != KeyEvent.KEYCODE_BACK)
                QtNative.keyDown(keyCode, c, event.getMetaState());
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;
        m_metaState = MetaKeyKeyListener.handleKeyUp(m_metaState, keyCode, event);
        QtNative.keyUp(keyCode, event.getUnicodeChar(), event.getMetaState());
        return true;
    }

    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

/*    public boolean onCreateOptionsMenu(Menu menu)
    {
        QtNative.createOptionsMenu(menu);
        try {
            return onPrepareOptionsMenu(menu);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean onPrepareOptionsMenu(Menu menu)
    {
        QtNative.prepareOptionsMenu(menu);
        try {
            return (Boolean) onPrepareOptionsMenu(menu);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean onOptionsItemSelected(MenuItem item)
    {
        return QtNative.optionsItemSelected(item.getGroupId(), item.getItemId());
    }*/
}
