// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.NinePatch;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.drawable.AnimatedStateListDrawable;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ClipDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.GradientDrawable.Orientation;
import android.graphics.drawable.InsetDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.NinePatchDrawable;
import android.graphics.drawable.RippleDrawable;
import android.graphics.drawable.RotateDrawable;
import android.graphics.drawable.ScaleDrawable;
import android.graphics.drawable.StateListDrawable;
import android.graphics.drawable.VectorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.util.Xml;
import android.view.ContextThemeWrapper;
import android.view.inputmethod.EditorInfo;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.xmlpull.v1.XmlPullParser;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;


public class ExtractStyle {

    // This used to be retrieved from android.R.styleable.ViewDrawableStates field via reflection,
    // but since the access to that is restricted, we need to have hard-coded here.
    final int[] viewDrawableStatesState = new int[]{
            android.R.attr.state_focused,
            android.R.attr.state_window_focused,
            android.R.attr.state_enabled,
            android.R.attr.state_selected,
            android.R.attr.state_pressed,
            android.R.attr.state_activated,
            android.R.attr.state_accelerated,
            android.R.attr.state_hovered,
            android.R.attr.state_drag_can_accept,
            android.R.attr.state_drag_hovered
    };
    final int[] EMPTY_STATE_SET = {};
    final int[] ENABLED_STATE_SET = {android.R.attr.state_enabled};
    final int[] FOCUSED_STATE_SET = {android.R.attr.state_focused};
    final int[] SELECTED_STATE_SET = {android.R.attr.state_selected};
    final int[] PRESSED_STATE_SET = {android.R.attr.state_pressed};
    final int[] WINDOW_FOCUSED_STATE_SET = {android.R.attr.state_window_focused};
    final int[] ENABLED_FOCUSED_STATE_SET = stateSetUnion(ENABLED_STATE_SET, FOCUSED_STATE_SET);
    final int[] ENABLED_SELECTED_STATE_SET = stateSetUnion(ENABLED_STATE_SET, SELECTED_STATE_SET);
    final int[] ENABLED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] FOCUSED_SELECTED_STATE_SET = stateSetUnion(FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] ENABLED_FOCUSED_SELECTED_STATE_SET = stateSetUnion(ENABLED_FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_SELECTED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_FOCUSED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, FOCUSED_STATE_SET);
    final int[] PRESSED_FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_FOCUSED_SELECTED_STATE_SET = stateSetUnion(PRESSED_FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, ENABLED_STATE_SET);
    final int[] PRESSED_ENABLED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_SELECTED_STATE_SET = stateSetUnion(PRESSED_ENABLED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_STATE_SET, FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET = stateSetUnion(PRESSED_ENABLED_FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final Resources.Theme m_theme;
    final String m_extractPath;
    final int defaultBackgroundColor;
    final int defaultTextColor;
    final boolean m_minimal;
    final int[] DrawableStates = { android.R.attr.state_active, android.R.attr.state_checked,
            android.R.attr.state_enabled, android.R.attr.state_focused,
            android.R.attr.state_pressed, android.R.attr.state_selected,
            android.R.attr.state_window_focused, 16908288, android.R.attr.state_multiline,
            android.R.attr.state_activated, android.R.attr.state_accelerated};
    final String[] DrawableStatesLabels = {"active", "checked", "enabled", "focused", "pressed",
            "selected", "window_focused", "background", "multiline", "activated", "accelerated"};
    final String[] DisableDrawableStatesLabels = {"inactive", "unchecked", "disabled",
            "not_focused", "no_pressed", "unselected", "window_not_focused", "background",
            "multiline", "activated", "accelerated"};
    final String[] sScaleTypeArray = {
            "MATRIX",
            "FIT_XY",
            "FIT_START",
            "FIT_CENTER",
            "FIT_END",
            "CENTER",
            "CENTER_CROP",
            "CENTER_INSIDE"
    };
    Context m_context;
    private final HashMap<String, DrawableCache> m_drawableCache = new HashMap<>();

    private static final String EXTRACT_STYLE_KEY = "extract.android.style";
    private static final String EXTRACT_STYLE_MINIMAL_KEY = "extract.android.style.option";

    private static boolean m_missingNormalStyle = false;
    private static boolean m_missingDarkStyle = false;
    private static String  m_stylePath = null;
    private static boolean m_extractMinimal = false;

    public static void setup(Bundle loaderParams) {
        if (loaderParams.containsKey(EXTRACT_STYLE_KEY)) {
            m_stylePath = loaderParams.getString(EXTRACT_STYLE_KEY);

            boolean darkModeFileMissing = !(new File(m_stylePath + "darkUiMode/style.json").exists());
            m_missingDarkStyle = Build.VERSION.SDK_INT > 28 && darkModeFileMissing;

            m_missingNormalStyle = !(new File(m_stylePath + "style.json").exists());

            m_extractMinimal = loaderParams.containsKey(EXTRACT_STYLE_MINIMAL_KEY) &&
                               loaderParams.getBoolean(EXTRACT_STYLE_MINIMAL_KEY);
        }
    }

    public static void runIfNeeded(Context context, boolean extractDarkMode) {
        if (m_stylePath == null)
            return;
        if (extractDarkMode) {
            if (m_missingDarkStyle) {
                new ExtractStyle(context, m_stylePath + "darkUiMode/", m_extractMinimal);
                m_missingDarkStyle = false;
            }
        } else if (m_missingNormalStyle) {
            new ExtractStyle(context, m_stylePath, m_extractMinimal);
            m_missingNormalStyle = false;
        }
    }

    public ExtractStyle(Context context, String extractPath, boolean minimal) {
        m_minimal = minimal;
        m_extractPath = extractPath + "/";
        boolean dirCreated = new File(m_extractPath).mkdirs();
        if (!dirCreated)
            Log.w(QtNative.QtTAG, "Cannot create Android style directory.");
        m_context = context;
        m_theme = context.getTheme();
        TypedArray array = m_theme.obtainStyledAttributes(new int[]{
                android.R.attr.colorBackground,
                android.R.attr.textColorPrimary,
                android.R.attr.textColor
        });
        defaultBackgroundColor = array.getColor(0, 0);
        int textColor = array.getColor(1, 0xFFFFFF);
        if (textColor == 0xFFFFFF)
            textColor = array.getColor(2, 0xFFFFFF);
        defaultTextColor = textColor;
        array.recycle();

        try {
            SimpleJsonWriter jsonWriter = new SimpleJsonWriter(m_extractPath + "style.json");
            jsonWriter.beginObject();
            try {
                jsonWriter.name("defaultStyle").value(extractDefaultPalette());
                extractWindow(jsonWriter);
                jsonWriter.name("buttonStyle").value(extractTextAppearanceInformation(android.R.attr.buttonStyle, "QPushButton"));
                jsonWriter.name("spinnerStyle").value(extractTextAppearanceInformation(android.R.attr.spinnerStyle, "QComboBox"));
                extractProgressBar(jsonWriter, android.R.attr.progressBarStyleHorizontal, "progressBarStyleHorizontal", "QProgressBar");
                extractProgressBar(jsonWriter, android.R.attr.progressBarStyleLarge, "progressBarStyleLarge", null);
                extractProgressBar(jsonWriter, android.R.attr.progressBarStyleSmall, "progressBarStyleSmall", null);
                extractProgressBar(jsonWriter, android.R.attr.progressBarStyle, "progressBarStyle", null);
                extractAbsSeekBar(jsonWriter);
                extractSwitch(jsonWriter);
                extractCompoundButton(jsonWriter, android.R.attr.checkboxStyle, "checkboxStyle", "QCheckBox");
                jsonWriter.name("editTextStyle").value(extractTextAppearanceInformation(android.R.attr.editTextStyle, "QLineEdit"));
                extractCompoundButton(jsonWriter, android.R.attr.radioButtonStyle, "radioButtonStyle", "QRadioButton");
                jsonWriter.name("textViewStyle").value(extractTextAppearanceInformation(android.R.attr.textViewStyle, "QWidget"));
                jsonWriter.name("scrollViewStyle").value(extractTextAppearanceInformation(android.R.attr.scrollViewStyle, "QAbstractScrollArea"));
                extractListView(jsonWriter);
                jsonWriter.name("listSeparatorTextViewStyle").value(extractTextAppearanceInformation(android.R.attr.listSeparatorTextViewStyle, null));
                extractItemsStyle(jsonWriter);
                extractCompoundButton(jsonWriter, android.R.attr.buttonStyleToggle, "buttonStyleToggle", null);
                extractCalendar(jsonWriter);
                extractToolBar(jsonWriter);
                jsonWriter.name("actionButtonStyle").value(extractTextAppearanceInformation(android.R.attr.actionButtonStyle, "QToolButton"));
                jsonWriter.name("actionBarTabTextStyle").value(extractTextAppearanceInformation(android.R.attr.actionBarTabTextStyle, null));
                jsonWriter.name("actionBarTabStyle").value(extractTextAppearanceInformation(android.R.attr.actionBarTabStyle, null));
                jsonWriter.name("actionOverflowButtonStyle").value(extractImageViewInformation(android.R.attr.actionOverflowButtonStyle, null));
                extractTabBar(jsonWriter);
            } catch (Exception e) {
                e.printStackTrace();
            }
            jsonWriter.endObject();
            jsonWriter.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    native static int[] extractNativeChunkInfo20(long nativeChunk);

    private int[] stateSetUnion(final int[] stateSet1, final int[] stateSet2) {
        try {
            final int stateSet1Length = stateSet1.length;
            final int stateSet2Length = stateSet2.length;
            final int[] newSet = new int[stateSet1Length + stateSet2Length];
            int k = 0;
            int i = 0;
            int j = 0;
            // This is a merge of the two input state sets and assumes that the
            // input sets are sorted by the order imposed by ViewDrawableStates.
            for (int viewState : viewDrawableStatesState) {
                if (i < stateSet1Length && stateSet1[i] == viewState) {
                    newSet[k++] = viewState;
                    i++;
                } else if (j < stateSet2Length && stateSet2[j] == viewState) {
                    newSet[k++] = viewState;
                    j++;
                }
                assert k <= 1 || (newSet[k - 1] > newSet[k - 2]);
            }
            return newSet;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    Field getAccessibleField(Class<?> clazz, String fieldName) {
        try {
            Field f = clazz.getDeclaredField(fieldName);
            f.setAccessible(true);
            return f;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    Field tryGetAccessibleField(Class<?> clazz, String fieldName) {
        if (clazz == null)
            return null;

        try {
            Field f = clazz.getDeclaredField(fieldName);
            f.setAccessible(true);
            return f;
        } catch (Exception e) {
            for (Class<?> c : clazz.getInterfaces()) {
                Field f = tryGetAccessibleField(c, fieldName);
                if (f != null)
                    return f;
            }
        }
        return tryGetAccessibleField(clazz.getSuperclass(), fieldName);
    }

    JSONObject getColorStateList(ColorStateList colorList) {
        JSONObject json = new JSONObject();
        try {
            json.put("EMPTY_STATE_SET", colorList.getColorForState(EMPTY_STATE_SET, 0));
            json.put("WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(WINDOW_FOCUSED_STATE_SET, 0));
            json.put("SELECTED_STATE_SET", colorList.getColorForState(SELECTED_STATE_SET, 0));
            json.put("SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("FOCUSED_STATE_SET", colorList.getColorForState(FOCUSED_STATE_SET, 0));
            json.put("FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(FOCUSED_SELECTED_STATE_SET, 0));
            json.put("FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_STATE_SET", colorList.getColorForState(ENABLED_STATE_SET, 0));
            json.put("ENABLED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_SELECTED_STATE_SET", colorList.getColorForState(ENABLED_SELECTED_STATE_SET, 0));
            json.put("ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_SELECTED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_STATE_SET", colorList.getColorForState(PRESSED_STATE_SET, 0));
            json.put("PRESSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return json;
    }

    JSONObject getStatesList(int[] states) throws JSONException {
        JSONObject json = new JSONObject();
        for (int s : states) {
            boolean found = false;
            for (int d = 0; d < DrawableStates.length; d++) {
                if (s == DrawableStates[d]) {
                    json.put(DrawableStatesLabels[d], true);
                    found = true;
                    break;
                } else if (s == -DrawableStates[d]) {
                    json.put(DrawableStatesLabels[d], false);

                    found = true;
                    break;
                }
            }
            if (!found) {
                json.put("unhandled_state_" + s, s > 0);
            }
        }
        return json;
    }

    String getStatesName(int[] states) {
        StringBuilder statesName = new StringBuilder();
        for (int s : states) {
            boolean found = false;
            for (int d = 0; d < DrawableStates.length; d++) {
                if (s == DrawableStates[d]) {
                    if (statesName.length() > 0)
                        statesName.append("__");
                    statesName.append(DrawableStatesLabels[d]);
                    found = true;
                    break;
                } else if (s == -DrawableStates[d]) {
                    if (statesName.length() > 0)
                        statesName.append("__");
                    statesName.append(DisableDrawableStatesLabels[d]);
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (statesName.length() > 0)
                    statesName.append(";");
                statesName.append(s);
            }
        }
        if (statesName.length() > 0)
            return statesName.toString();
        return "empty";
    }

    private JSONObject getLayerDrawable(Object drawable, String filename) {
        JSONObject json = new JSONObject();
        LayerDrawable layers = (LayerDrawable) drawable;
        final int nr = layers.getNumberOfLayers();
        try {
            JSONArray array = new JSONArray();
            for (int i = 0; i < nr; i++) {
                int id = layers.getId(i);
                if (id == -1)
                    id = i;
                JSONObject layerJsonObject = getDrawable(layers.getDrawable(i), filename + "__" + id, null);
                layerJsonObject.put("id", id);
                array.put(layerJsonObject);
            }
            json.put("type", "layer");
            Rect padding = new Rect();
            if (layers.getPadding(padding))
                json.put("padding", getJsonRect(padding));
            json.put("layers", array);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getStateListDrawable(Object drawable, String filename) {
        JSONObject json = new JSONObject();
        try {
            StateListDrawable stateList = (StateListDrawable) drawable;
            JSONArray array = new JSONArray();
            final int numStates;
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q)
                numStates = (Integer) StateListDrawable.class.getMethod("getStateCount").invoke(stateList);
            else
                numStates = stateList.getStateCount();
            for (int i = 0; i < numStates; i++) {
                JSONObject stateJson = new JSONObject();
                final Drawable d = (Drawable) StateListDrawable.class.getMethod("getStateDrawable", Integer.TYPE).invoke(stateList, i);
                final int[] states = (int[]) StateListDrawable.class.getMethod("getStateSet", Integer.TYPE).invoke(stateList, i);
                if (states != null)
                    stateJson.put("states", getStatesList(states));
                stateJson.put("drawable", getDrawable(d, filename + "__" + (states != null ? getStatesName(states) : ("state_pos_" + i)), null));
                array.put(stateJson);
            }
            json.put("type", "stateslist");
            Rect padding = new Rect();
            if (stateList.getPadding(padding))
                json.put("padding", getJsonRect(padding));
            json.put("stateslist", array);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getGradientDrawable(GradientDrawable drawable) {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "gradient");
            Object obj = drawable.getConstantState();
            Class<?> gradientStateClass = obj.getClass();
            json.put("shape", gradientStateClass.getField("mShape").getInt(obj));
            json.put("gradient", gradientStateClass.getField("mGradient").getInt(obj));
            GradientDrawable.Orientation orientation = (Orientation) gradientStateClass.getField("mOrientation").get(obj);
            if (orientation != null)
                json.put("orientation", orientation.name());
            int[] intArray = (int[]) gradientStateClass.getField("mGradientColors").get(obj);
            if (intArray != null)
                json.put("colors", getJsonArray(intArray, 0, intArray.length));
            json.put("positions", getJsonArray((float[]) gradientStateClass.getField("mPositions").get(obj)));
            json.put("strokeWidth", gradientStateClass.getField("mStrokeWidth").getInt(obj));
            json.put("strokeDashWidth", gradientStateClass.getField("mStrokeDashWidth").getFloat(obj));
            json.put("strokeDashGap", gradientStateClass.getField("mStrokeDashGap").getFloat(obj));
            json.put("radius", gradientStateClass.getField("mRadius").getFloat(obj));
            float[] floatArray = (float[]) gradientStateClass.getField("mRadiusArray").get(obj);
            if (floatArray != null)
                json.put("radiusArray", getJsonArray(floatArray));
            Rect rc = (Rect) gradientStateClass.getField("mPadding").get(obj);
            if (rc != null)
                json.put("padding", getJsonRect(rc));
            json.put("width", gradientStateClass.getField("mWidth").getInt(obj));
            json.put("height", gradientStateClass.getField("mHeight").getInt(obj));
            json.put("innerRadiusRatio", gradientStateClass.getField("mInnerRadiusRatio").getFloat(obj));
            json.put("thicknessRatio", gradientStateClass.getField("mThicknessRatio").getFloat(obj));
            json.put("innerRadius", gradientStateClass.getField("mInnerRadius").getInt(obj));
            json.put("thickness", gradientStateClass.getField("mThickness").getInt(obj));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getRotateDrawable(RotateDrawable drawable, String filename) {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "rotate");
            Object obj = drawable.getConstantState();
            Class<?> rotateStateClass = obj.getClass();
            json.put("drawable", getDrawable(drawable.getClass().getMethod("getDrawable").invoke(drawable), filename, null));
            json.put("pivotX", getAccessibleField(rotateStateClass, "mPivotX").getFloat(obj));
            json.put("pivotXRel", getAccessibleField(rotateStateClass, "mPivotXRel").getBoolean(obj));
            json.put("pivotY", getAccessibleField(rotateStateClass, "mPivotY").getFloat(obj));
            json.put("pivotYRel", getAccessibleField(rotateStateClass, "mPivotYRel").getBoolean(obj));
            json.put("fromDegrees", getAccessibleField(rotateStateClass, "mFromDegrees").getFloat(obj));
            json.put("toDegrees", getAccessibleField(rotateStateClass, "mToDegrees").getFloat(obj));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getAnimationDrawable(AnimationDrawable drawable, String filename) {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "animation");
            json.put("oneshot", drawable.isOneShot());
            final int count = drawable.getNumberOfFrames();
            JSONArray frames = new JSONArray();
            for (int i = 0; i < count; ++i) {
                JSONObject frame = new JSONObject();
                frame.put("duration", drawable.getDuration(i));
                frame.put("drawable", getDrawable(drawable.getFrame(i), filename + "__" + i, null));
                frames.put(frame);
            }
            json.put("frames", frames);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getJsonRect(Rect rect) throws JSONException {
        JSONObject jsonRect = new JSONObject();
        jsonRect.put("left", rect.left);
        jsonRect.put("top", rect.top);
        jsonRect.put("right", rect.right);
        jsonRect.put("bottom", rect.bottom);
        return jsonRect;

    }

    private JSONArray getJsonArray(int[] array, int pos, int len) {
        JSONArray a = new JSONArray();
        final int l = pos + len;
        for (int i = pos; i < l; i++)
            a.put(array[i]);
        return a;
    }

    private JSONArray getJsonArray(float[] array) throws JSONException {
        JSONArray a = new JSONArray();
        if (array != null)
            for (float val : array)
                a.put(val);
        return a;
    }

    private JSONObject getJsonChunkInfo(int[] chunkData) throws JSONException {
        JSONObject jsonRect = new JSONObject();
        if (chunkData == null)
            return jsonRect;

        jsonRect.put("xdivs", getJsonArray(chunkData, 3, chunkData[0]));
        jsonRect.put("ydivs", getJsonArray(chunkData, 3 + chunkData[0], chunkData[1]));
        jsonRect.put("colors", getJsonArray(chunkData, 3 + chunkData[0] + chunkData[1], chunkData[2]));
        return jsonRect;
    }

    private JSONObject findPatchesMarings(Drawable d) throws JSONException, IllegalAccessException {
        NinePatch np;
        Field f = tryGetAccessibleField(NinePatchDrawable.class, "mNinePatch");
        if (f != null) {
            np = (NinePatch) f.get(d);
        } else {
            Object state = getAccessibleField(NinePatchDrawable.class, "mNinePatchState").get(d);
            np = (NinePatch) getAccessibleField(Objects.requireNonNull(state).getClass(), "mNinePatch").get(state);
        }
        return getJsonChunkInfo(extractNativeChunkInfo20(getAccessibleField(Objects.requireNonNull(np).getClass(), "mNativeChunk").getLong(np)));
    }

    private JSONObject getRippleDrawable(Object drawable, String filename, Rect padding) {
        JSONObject json = getLayerDrawable(drawable, filename);
        JSONObject ripple = new JSONObject();
        try {
            Class<?> rippleDrawableClass = Class.forName("android.graphics.drawable.RippleDrawable");
            final Object mState = getAccessibleField(rippleDrawableClass, "mState").get(drawable);
            ripple.put("mask", getDrawable((Drawable) getAccessibleField(rippleDrawableClass, "mMask").get(drawable), filename, padding));
            if (mState != null) {
                ripple.put("maxRadius", getAccessibleField(mState.getClass(), "mMaxRadius").getInt(mState));
                ColorStateList color = (ColorStateList) getAccessibleField(mState.getClass(), "mColor").get(mState);
                if (color != null)
                    ripple.put("color", getColorStateList(color));
            }
            json.put("ripple", ripple);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private HashMap<Long, Long> getStateTransitions(Object sa) throws Exception {
        HashMap<Long, Long> transitions = new HashMap<>();
        final int sz = getAccessibleField(sa.getClass(), "mSize").getInt(sa);
        long[] keys = (long[]) getAccessibleField(sa.getClass(), "mKeys").get(sa);
        long[] values = (long[]) getAccessibleField(sa.getClass(), "mValues").get(sa);
        for (int i = 0; i < sz; i++) {
            if (keys != null && values != null)
                transitions.put(keys[i], values[i]);
        }
        return transitions;
    }

    private HashMap<Integer, Integer> getStateIds(Object sa) throws Exception {
        HashMap<Integer, Integer> states = new HashMap<>();
        final int sz = getAccessibleField(sa.getClass(), "mSize").getInt(sa);
        int[] keys = (int[]) getAccessibleField(sa.getClass(), "mKeys").get(sa);
        int[] values = (int[]) getAccessibleField(sa.getClass(), "mValues").get(sa);
        for (int i = 0; i < sz; i++) {
            if (keys != null && values != null)
                states.put(keys[i], values[i]);
        }
        return states;
    }

    private int findStateIndex(int id, HashMap<Integer, Integer> stateIds) {
        for (Map.Entry<Integer, Integer> s : stateIds.entrySet()) {
            if (id == s.getValue())
                return s.getKey();
        }
        return -1;
    }

    private JSONObject getAnimatedStateListDrawable(Object drawable, String filename) {
        JSONObject json = getStateListDrawable(drawable, filename);
        try {
            Class<?> animatedStateListDrawableClass = Class.forName("android.graphics.drawable.AnimatedStateListDrawable");
            Object state = getAccessibleField(animatedStateListDrawableClass, "mState").get(drawable);

            if (state != null) {
                Class<?> stateClass = state.getClass();
                HashMap<Integer, Integer> stateIds = getStateIds(Objects.requireNonNull(getAccessibleField(stateClass, "mStateIds").get(state)));
                HashMap<Long, Long> transitions = getStateTransitions(Objects.requireNonNull(getAccessibleField(stateClass, "mTransitions").get(state)));

                for (Map.Entry<Long, Long> t : transitions.entrySet()) {
                    final int toState = findStateIndex(t.getKey().intValue(), stateIds);
                    final int fromState = findStateIndex((int) (t.getKey() >> 32), stateIds);

                    JSONObject transition = new JSONObject();
                    transition.put("from", fromState);
                    transition.put("to", toState);
                    transition.put("reverse", (t.getValue() >> 32) != 0);

                    JSONArray stateslist = json.getJSONArray("stateslist");
                    JSONObject stateobj = stateslist.getJSONObject(t.getValue().intValue());
                    stateobj.put("transition", transition);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getVPath(Object path) throws Exception {
        JSONObject json = new JSONObject();
        final Class<?> pathClass = path.getClass();
        json.put("type", "path");
        json.put("name", tryGetAccessibleField(pathClass, "mPathName").get(path));
        Object[] mNodes = (Object[]) tryGetAccessibleField(pathClass, "mNodes").get(path);
        JSONArray nodes = new JSONArray();
        if (mNodes != null) {
            for (Object n : mNodes) {
                JSONObject node = new JSONObject();
                node.put("type", String.valueOf(getAccessibleField(n.getClass(), "mType").getChar(n)));
                node.put("params", getJsonArray((float[]) getAccessibleField(n.getClass(), "mParams").get(n)));
                nodes.put(node);
            }
            json.put("nodes", nodes);
        }
        json.put("isClip", (Boolean) pathClass.getMethod("isClipPath").invoke(path));

        if (tryGetAccessibleField(pathClass, "mStrokeColor") == null)
            return json; // not VFullPath

        json.put("strokeColor", getAccessibleField(pathClass, "mStrokeColor").getInt(path));
        json.put("strokeWidth", getAccessibleField(pathClass, "mStrokeWidth").getFloat(path));
        json.put("fillColor", getAccessibleField(pathClass, "mFillColor").getInt(path));
        json.put("strokeAlpha", getAccessibleField(pathClass, "mStrokeAlpha").getFloat(path));
        json.put("fillRule", getAccessibleField(pathClass, "mFillRule").getInt(path));
        json.put("fillAlpha", getAccessibleField(pathClass, "mFillAlpha").getFloat(path));
        json.put("trimPathStart", getAccessibleField(pathClass, "mTrimPathStart").getFloat(path));
        json.put("trimPathEnd", getAccessibleField(pathClass, "mTrimPathEnd").getFloat(path));
        json.put("trimPathOffset", getAccessibleField(pathClass, "mTrimPathOffset").getFloat(path));
        json.put("strokeLineCap", (Paint.Cap) getAccessibleField(pathClass, "mStrokeLineCap").get(path));
        json.put("strokeLineJoin", (Paint.Join) getAccessibleField(pathClass, "mStrokeLineJoin").get(path));
        json.put("strokeMiterlimit", getAccessibleField(pathClass, "mStrokeMiterlimit").getFloat(path));
        return json;
    }

    @SuppressWarnings("unchecked")
    private JSONObject getVGroup(Object group) throws Exception {
        JSONObject json = new JSONObject();
        json.put("type", "group");
        final Class<?> groupClass = group.getClass();
        json.put("name", getAccessibleField(groupClass, "mGroupName").get(group));
        json.put("rotate", getAccessibleField(groupClass, "mRotate").getFloat(group));
        json.put("pivotX", getAccessibleField(groupClass, "mPivotX").getFloat(group));
        json.put("pivotY", getAccessibleField(groupClass, "mPivotY").getFloat(group));
        json.put("scaleX", getAccessibleField(groupClass, "mScaleX").getFloat(group));
        json.put("scaleY", getAccessibleField(groupClass, "mScaleY").getFloat(group));
        json.put("translateX", getAccessibleField(groupClass, "mTranslateX").getFloat(group));
        json.put("translateY", getAccessibleField(groupClass, "mTranslateY").getFloat(group));

        ArrayList<Object> mChildren = (ArrayList<Object>) getAccessibleField(groupClass, "mChildren").get(group);
        JSONArray children = new JSONArray();
        if (mChildren != null) {
            for (Object c : mChildren) {
                if (groupClass.isInstance(c))
                    children.put(getVGroup(c));
                else
                    children.put(getVPath(c));
            }
            json.put("children", children);
        }
        return json;
    }

    private JSONObject getVectorDrawable(Object drawable) {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "vector");
            Class<?> vectorDrawableClass = Class.forName("android.graphics.drawable.VectorDrawable");
            final Object state = getAccessibleField(vectorDrawableClass, "mVectorState").get(drawable);
            final Class<?> stateClass = Objects.requireNonNull(state).getClass();
            final ColorStateList mTint = (ColorStateList) getAccessibleField(stateClass, "mTint").get(state);
            if (mTint != null) {
                json.put("tintList", getColorStateList(mTint));
                json.put("tintMode", (PorterDuff.Mode) getAccessibleField(stateClass, "mTintMode").get(state));
            }
            final Object mVPathRenderer = getAccessibleField(stateClass, "mVPathRenderer").get(state);
            final Class<?> VPathRendererClass = Objects.requireNonNull(mVPathRenderer).getClass();
            json.put("baseWidth", getAccessibleField(VPathRendererClass, "mBaseWidth").getFloat(mVPathRenderer));
            json.put("baseHeight", getAccessibleField(VPathRendererClass, "mBaseHeight").getFloat(mVPathRenderer));
            json.put("viewportWidth", getAccessibleField(VPathRendererClass, "mViewportWidth").getFloat(mVPathRenderer));
            json.put("viewportHeight", getAccessibleField(VPathRendererClass, "mViewportHeight").getFloat(mVPathRenderer));
            json.put("rootAlpha", getAccessibleField(VPathRendererClass, "mRootAlpha").getInt(mVPathRenderer));
            json.put("rootName", getAccessibleField(VPathRendererClass, "mRootName").get(mVPathRenderer));
            json.put("rootGroup", getVGroup(Objects.requireNonNull(getAccessibleField(VPathRendererClass, "mRootGroup").get(mVPathRenderer))));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    public JSONObject getDrawable(Object drawable, String filename, Rect padding) {
        if (drawable == null || m_minimal)
            return null;

        DrawableCache dc = m_drawableCache.get(filename);
        if (dc != null) {
            if (dc.drawable.equals(drawable))
                return dc.object;
            else
                Log.e(QtNative.QtTAG, "Different drawable objects points to the same file name \"" + filename + "\"");
        }
        JSONObject json = new JSONObject();
        Bitmap bmp = null;
        if (drawable instanceof Bitmap)
            bmp = (Bitmap) drawable;
        else {
            if (drawable instanceof BitmapDrawable) {
                BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
                bmp = bitmapDrawable.getBitmap();
                try {
                    json.put("gravity", bitmapDrawable.getGravity());
                    json.put("tileModeX", bitmapDrawable.getTileModeX());
                    json.put("tileModeY", bitmapDrawable.getTileModeY());
                    json.put("antialias", (Boolean) BitmapDrawable.class.getMethod("hasAntiAlias").invoke(bitmapDrawable));
                    json.put("mipMap", (Boolean) BitmapDrawable.class.getMethod("hasMipMap").invoke(bitmapDrawable));
                    json.put("tintMode", (PorterDuff.Mode) BitmapDrawable.class.getMethod("getTintMode").invoke(bitmapDrawable));
                    ColorStateList tintList = (ColorStateList) BitmapDrawable.class.getMethod("getTint").invoke(bitmapDrawable);
                    if (tintList != null)
                        json.put("tintList", getColorStateList(tintList));
                } catch (Exception e) {
                    e.printStackTrace();
                }
            } else {

                if (drawable instanceof RippleDrawable)
                    return getRippleDrawable(drawable, filename, padding);

                if (drawable instanceof AnimatedStateListDrawable)
                    return getAnimatedStateListDrawable(drawable, filename);

                if (drawable instanceof VectorDrawable)
                    return getVectorDrawable(drawable);

                if (drawable instanceof ScaleDrawable) {
                    return getDrawable(((ScaleDrawable) drawable).getDrawable(), filename, null);
                }
                if (drawable instanceof LayerDrawable) {
                    return getLayerDrawable(drawable, filename);
                }
                if (drawable instanceof StateListDrawable) {
                    return getStateListDrawable(drawable, filename);
                }
                if (drawable instanceof GradientDrawable) {
                    return getGradientDrawable((GradientDrawable) drawable);
                }
                if (drawable instanceof RotateDrawable) {
                    return getRotateDrawable((RotateDrawable) drawable, filename);
                }
                if (drawable instanceof AnimationDrawable) {
                    return getAnimationDrawable((AnimationDrawable) drawable, filename);
                }
                if (drawable instanceof ClipDrawable) {
                    try {
                        json.put("type", "clipDrawable");
                        Drawable.ConstantState dcs = ((ClipDrawable) drawable).getConstantState();
                        json.put("drawable", getDrawable(getAccessibleField(dcs.getClass(), "mDrawable").get(dcs), filename, null));
                        if (null != padding)
                            json.put("padding", getJsonRect(padding));
                        else {
                            Rect _padding = new Rect();
                            if (((Drawable) drawable).getPadding(_padding))
                                json.put("padding", getJsonRect(_padding));
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    return json;
                }
                if (drawable instanceof ColorDrawable) {
                    bmp = Bitmap.createBitmap(1, 1, Config.ARGB_8888);
                    Drawable d = (Drawable) drawable;
                    d.setBounds(0, 0, 1, 1);
                    d.draw(new Canvas(bmp));
                    try {
                        json.put("type", "color");
                        json.put("color", bmp.getPixel(0, 0));
                        if (null != padding)
                            json.put("padding", getJsonRect(padding));
                        else {
                            Rect _padding = new Rect();
                            if (d.getPadding(_padding))
                                json.put("padding", getJsonRect(_padding));
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    return json;
                }
                if (drawable instanceof InsetDrawable) {
                    try {
                        InsetDrawable d = (InsetDrawable) drawable;
                        Object mInsetStateObject = getAccessibleField(InsetDrawable.class, "mState").get(d);
                        Rect _padding = new Rect();
                        boolean hasPadding = d.getPadding(_padding);
                        return getDrawable(getAccessibleField(Objects.requireNonNull(mInsetStateObject).getClass(), "mDrawable").get(mInsetStateObject), filename, hasPadding ? _padding : null);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                } else {
                    Drawable d = (Drawable) drawable;
                    int w = d.getIntrinsicWidth();
                    int h = d.getIntrinsicHeight();
                    d.setLevel(10000);
                    if (w < 1 || h < 1) {
                        w = 100;
                        h = 100;
                    }
                    bmp = Bitmap.createBitmap(w, h, Config.ARGB_8888);
                    d.setBounds(0, 0, w, h);
                    d.draw(new Canvas(bmp));
                    if (drawable instanceof NinePatchDrawable) {
                        NinePatchDrawable npd = (NinePatchDrawable) drawable;
                        try {
                            json.put("type", "9patch");
                            json.put("drawable", getDrawable(bmp, filename, null));
                            if (padding != null)
                                json.put("padding", getJsonRect(padding));
                            else {
                                Rect _padding = new Rect();
                                if (npd.getPadding(_padding))
                                    json.put("padding", getJsonRect(_padding));
                            }

                            json.put("chunkInfo", findPatchesMarings(d));
                            return json;
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
        }
        FileOutputStream out;
        try {
            filename = m_extractPath + filename + ".png";
            out = new FileOutputStream(filename);
            if (bmp != null)
                bmp.compress(Bitmap.CompressFormat.PNG, 100, out);
            out.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        try {
            json.put("type", "image");
            json.put("path", filename);
            if (bmp != null) {
                json.put("width", bmp.getWidth());
                json.put("height", bmp.getHeight());
            }
            m_drawableCache.put(filename, new DrawableCache(json, drawable));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return json;
    }

    private TypedArray obtainStyledAttributes(int styleName, int[] attributes)
    {
        TypedValue typedValue = new TypedValue();
        Context ctx = new ContextThemeWrapper(m_context, m_theme);
        ctx.getTheme().resolveAttribute(styleName, typedValue, true);
        return ctx.obtainStyledAttributes(typedValue.data, attributes);
    }

    private ArrayList<Integer> getArrayListFromIntArray(int[] attributes) {
        ArrayList<Integer> sortedAttrs = new ArrayList<>();
        for (int attr : attributes)
            sortedAttrs.add(attr);
        return sortedAttrs;
    }

    public void extractViewInformation(int styleName, JSONObject json, String qtClassName) {
        extractViewInformation(styleName, json, qtClassName, null);
    }

    public void extractViewInformation(int styleName, JSONObject json, String qtClassName, AttributeSet attributeSet) {
        try {
            TypedValue typedValue = new TypedValue();
            Context ctx = new ContextThemeWrapper(m_context, m_theme);
            ctx.getTheme().resolveAttribute(styleName, typedValue, true);

            int[] attributes = new int[]{
                    android.R.attr.digits,
                    android.R.attr.background,
                    android.R.attr.padding,
                    android.R.attr.paddingLeft,
                    android.R.attr.paddingTop,
                    android.R.attr.paddingRight,
                    android.R.attr.paddingBottom,
                    android.R.attr.scrollX,
                    android.R.attr.scrollY,
                    android.R.attr.id,
                    android.R.attr.tag,
                    android.R.attr.fitsSystemWindows,
                    android.R.attr.focusable,
                    android.R.attr.focusableInTouchMode,
                    android.R.attr.clickable,
                    android.R.attr.longClickable,
                    android.R.attr.saveEnabled,
                    android.R.attr.duplicateParentState,
                    android.R.attr.visibility,
                    android.R.attr.drawingCacheQuality,
                    android.R.attr.contentDescription,
                    android.R.attr.soundEffectsEnabled,
                    android.R.attr.hapticFeedbackEnabled,
                    android.R.attr.scrollbars,
                    android.R.attr.fadingEdge,
                    android.R.attr.scrollbarStyle,
                    android.R.attr.scrollbarFadeDuration,
                    android.R.attr.scrollbarDefaultDelayBeforeFade,
                    android.R.attr.scrollbarSize,
                    android.R.attr.scrollbarThumbHorizontal,
                    android.R.attr.scrollbarThumbVertical,
                    android.R.attr.scrollbarTrackHorizontal,
                    android.R.attr.scrollbarTrackVertical,
                    android.R.attr.isScrollContainer,
                    android.R.attr.keepScreenOn,
                    android.R.attr.filterTouchesWhenObscured,
                    android.R.attr.nextFocusLeft,
                    android.R.attr.nextFocusRight,
                    android.R.attr.nextFocusUp,
                    android.R.attr.nextFocusDown,
                    android.R.attr.minWidth,
                    android.R.attr.minHeight,
                    android.R.attr.onClick,
                    android.R.attr.overScrollMode,
                    android.R.attr.paddingStart,
                    android.R.attr.paddingEnd,
            };

            // The array must be sorted in ascending order, otherwise obtainStyledAttributes()
            // might fail to find some attributes
            Arrays.sort(attributes);
            TypedArray array;
            if (attributeSet != null)
                array = m_theme.obtainStyledAttributes(attributeSet, attributes, styleName, 0);
            else
                array = obtainStyledAttributes(styleName, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            if (null != qtClassName)
                json.put("qtClass", qtClassName);

            json.put("defaultBackgroundColor", defaultBackgroundColor);
            json.put("defaultTextColorPrimary", defaultTextColor);
            json.put("TextView_digits", array.getText(sortedAttrs.indexOf(android.R.attr.digits)));
            json.put("View_background", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.background)), styleName + "_View_background", null));
            json.put("View_padding", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.padding), -1));
            json.put("View_paddingLeft", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.paddingLeft), -1));
            json.put("View_paddingTop", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.paddingTop), -1));
            json.put("View_paddingRight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.paddingRight), -1));
            json.put("View_paddingBottom", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.paddingBottom), -1));
            json.put("View_paddingBottom", array.getDimensionPixelOffset(sortedAttrs.indexOf(android.R.attr.scrollX), 0));
            json.put("View_scrollY", array.getDimensionPixelOffset(sortedAttrs.indexOf(android.R.attr.scrollY), 0));
            json.put("View_id", array.getResourceId(sortedAttrs.indexOf(android.R.attr.id), -1));
            json.put("View_tag", array.getText(sortedAttrs.indexOf(android.R.attr.tag)));
            json.put("View_fitsSystemWindows", array.getBoolean(sortedAttrs.indexOf(android.R.attr.fitsSystemWindows), false));
            json.put("View_focusable", array.getBoolean(sortedAttrs.indexOf(android.R.attr.focusable), false));
            json.put("View_focusableInTouchMode", array.getBoolean(sortedAttrs.indexOf(android.R.attr.focusableInTouchMode), false));
            json.put("View_clickable", array.getBoolean(sortedAttrs.indexOf(android.R.attr.clickable), false));
            json.put("View_longClickable", array.getBoolean(sortedAttrs.indexOf(android.R.attr.longClickable), false));
            json.put("View_saveEnabled", array.getBoolean(sortedAttrs.indexOf(android.R.attr.saveEnabled), true));
            json.put("View_duplicateParentState", array.getBoolean(sortedAttrs.indexOf(android.R.attr.duplicateParentState), false));
            json.put("View_visibility", array.getInt(sortedAttrs.indexOf(android.R.attr.visibility), 0));
            json.put("View_drawingCacheQuality", array.getInt(sortedAttrs.indexOf(android.R.attr.drawingCacheQuality), 0));
            json.put("View_contentDescription", array.getString(sortedAttrs.indexOf(android.R.attr.contentDescription)));
            json.put("View_soundEffectsEnabled", array.getBoolean(sortedAttrs.indexOf(android.R.attr.soundEffectsEnabled), true));
            json.put("View_hapticFeedbackEnabled", array.getBoolean(sortedAttrs.indexOf(android.R.attr.hapticFeedbackEnabled), true));
            json.put("View_scrollbars", array.getInt(sortedAttrs.indexOf(android.R.attr.scrollbars), 0));
            json.put("View_fadingEdge", array.getInt(sortedAttrs.indexOf(android.R.attr.fadingEdge), 0));
            json.put("View_scrollbarStyle", array.getInt(sortedAttrs.indexOf(android.R.attr.scrollbarStyle), 0));
            json.put("View_scrollbarFadeDuration", array.getInt(sortedAttrs.indexOf(android.R.attr.scrollbarFadeDuration), 0));
            json.put("View_scrollbarDefaultDelayBeforeFade", array.getInt(sortedAttrs.indexOf(android.R.attr.scrollbarDefaultDelayBeforeFade), 0));
            json.put("View_scrollbarSize", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.scrollbarSize), -1));
            json.put("View_scrollbarThumbHorizontal", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.scrollbarThumbHorizontal)), styleName + "_View_scrollbarThumbHorizontal", null));
            json.put("View_scrollbarThumbVertical", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.scrollbarThumbVertical)), styleName + "_View_scrollbarThumbVertical", null));
            json.put("View_scrollbarTrackHorizontal", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.scrollbarTrackHorizontal)), styleName + "_View_scrollbarTrackHorizontal", null));
            json.put("View_scrollbarTrackVertical", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.scrollbarTrackVertical)), styleName + "_View_scrollbarTrackVertical", null));
            json.put("View_isScrollContainer", array.getBoolean(sortedAttrs.indexOf(android.R.attr.isScrollContainer), false));
            json.put("View_keepScreenOn", array.getBoolean(sortedAttrs.indexOf(android.R.attr.keepScreenOn), false));
            json.put("View_filterTouchesWhenObscured", array.getBoolean(sortedAttrs.indexOf(android.R.attr.filterTouchesWhenObscured), false));
            json.put("View_nextFocusLeft", array.getResourceId(sortedAttrs.indexOf(android.R.attr.nextFocusLeft), -1));
            json.put("View_nextFocusRight", array.getResourceId(sortedAttrs.indexOf(android.R.attr.nextFocusRight), -1));
            json.put("View_nextFocusUp", array.getResourceId(sortedAttrs.indexOf(android.R.attr.nextFocusUp), -1));
            json.put("View_nextFocusDown", array.getResourceId(sortedAttrs.indexOf(android.R.attr.nextFocusDown), -1));
            json.put("View_minWidth", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.minWidth), 0));
            json.put("View_minHeight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.minHeight), 0));
            json.put("View_onClick", array.getString(sortedAttrs.indexOf(android.R.attr.onClick)));
            json.put("View_overScrollMode", array.getInt(sortedAttrs.indexOf(android.R.attr.overScrollMode), 1));
            json.put("View_paddingStart", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.paddingStart), 0));
            json.put("View_paddingEnd", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.paddingEnd), 0));
            array.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public JSONObject extractTextAppearance(int styleName)
    {
        return extractTextAppearance(styleName, false);
    }

    @SuppressLint("ResourceType")
    public JSONObject extractTextAppearance(int styleName, boolean subStyle)
    {
        final int[] attributes = new int[]{
                android.R.attr.textSize,
                android.R.attr.textStyle,
                android.R.attr.textColor,
                android.R.attr.typeface,
                android.R.attr.textAllCaps,
                android.R.attr.textColorHint,
                android.R.attr.textColorLink,
                android.R.attr.textColorHighlight
        };
        Arrays.sort(attributes);
        TypedArray array;
        if (subStyle)
            array = m_theme.obtainStyledAttributes(styleName, attributes);
        else
            array = obtainStyledAttributes(styleName, attributes);
        ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);
        JSONObject json = new JSONObject();
        try {
            int attr = sortedAttrs.indexOf(android.R.attr.textSize);
            if (array.hasValue(attr))
                json.put("TextAppearance_textSize", array.getDimensionPixelSize(attr, 15));
            attr = sortedAttrs.indexOf(android.R.attr.textStyle);
            if (array.hasValue(attr))
                json.put("TextAppearance_textStyle", array.getInt(attr, -1));
            ColorStateList color = array.getColorStateList(sortedAttrs.indexOf(android.R.attr.textColor));
            if (color != null)
                json.put("TextAppearance_textColor", getColorStateList(color));
            attr = sortedAttrs.indexOf(android.R.attr.typeface);
            if (array.hasValue(attr))
                json.put("TextAppearance_typeface", array.getInt(attr, -1));
            attr = sortedAttrs.indexOf(android.R.attr.textAllCaps);
            if (array.hasValue(attr))
                json.put("TextAppearance_textAllCaps", array.getBoolean(attr, false));
            color = array.getColorStateList(sortedAttrs.indexOf(android.R.attr.textColorHint));
            if (color != null)
                json.put("TextAppearance_textColorHint", getColorStateList(color));
            color = array.getColorStateList(sortedAttrs.indexOf(android.R.attr.textColorLink));
            if (color != null)
                json.put("TextAppearance_textColorLink", getColorStateList(color));
            attr = sortedAttrs.indexOf(android.R.attr.textColorHighlight);
            if (array.hasValue(attr))
                json.put("TextAppearance_textColorHighlight", array.getColor(attr, 0));
            array.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    public JSONObject extractTextAppearanceInformation(int styleName, String qtClass, AttributeSet attributeSet) {
        return extractTextAppearanceInformation(styleName, qtClass, android.R.attr.textAppearance, attributeSet);
    }

    public JSONObject extractTextAppearanceInformation(int styleName, String qtClass) {
        return extractTextAppearanceInformation(styleName, qtClass, android.R.attr.textAppearance, null);
    }

    public JSONObject extractTextAppearanceInformation(int styleName, String qtClass, int textAppearance, AttributeSet attributeSet) {
        JSONObject json = new JSONObject();
        extractViewInformation(styleName, json, qtClass, attributeSet);

        if (textAppearance == -1)
            textAppearance = android.R.attr.textAppearance;

        try {
            TypedValue typedValue = new TypedValue();
            Context ctx = new ContextThemeWrapper(m_context, m_theme);
            ctx.getTheme().resolveAttribute(styleName, typedValue, true);

            // Get textAppearance values
            int[] textAppearanceAttr = new int[]{textAppearance};
            TypedArray textAppearanceArray = ctx.obtainStyledAttributes(typedValue.data, textAppearanceAttr);
            int textAppearanceId = textAppearanceArray.getResourceId(0, -1);
            textAppearanceArray.recycle();

            int textSize = 15;
            int styleIndex = -1;
            int typefaceIndex = -1;
            int textColorHighlight = 0;
            boolean allCaps = false;

            if (textAppearanceId != -1) {
                int[] attributes = new int[]{
                        android.R.attr.textSize,
                        android.R.attr.textStyle,
                        android.R.attr.typeface,
                        android.R.attr.textAllCaps,
                        android.R.attr.textColorHighlight
                };
                Arrays.sort(attributes);
                TypedArray array = m_theme.obtainStyledAttributes(textAppearanceId, attributes);
                ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

                textSize = array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.textSize), 15);
                styleIndex = array.getInt(sortedAttrs.indexOf(android.R.attr.textStyle), -1);
                typefaceIndex = array.getInt(sortedAttrs.indexOf(android.R.attr.typeface), -1);
                textColorHighlight = array.getColor(sortedAttrs.indexOf(android.R.attr.textColorHighlight), 0);
                allCaps = array.getBoolean(sortedAttrs.indexOf(android.R.attr.textAllCaps), false);
                array.recycle();
            }
            // Get TextView values
            int[] attributes = new int[]{
                    android.R.attr.editable,
                    android.R.attr.inputMethod,
                    android.R.attr.numeric,
                    android.R.attr.digits,
                    android.R.attr.phoneNumber,
                    android.R.attr.autoText,
                    android.R.attr.capitalize,
                    android.R.attr.bufferType,
                    android.R.attr.selectAllOnFocus,
                    android.R.attr.autoLink,
                    android.R.attr.linksClickable,
                    android.R.attr.drawableLeft,
                    android.R.attr.drawableTop,
                    android.R.attr.drawableRight,
                    android.R.attr.drawableBottom,
                    android.R.attr.drawableStart,
                    android.R.attr.drawableEnd,
                    android.R.attr.maxLines,
                    android.R.attr.drawablePadding,
                    android.R.attr.textCursorDrawable,
                    android.R.attr.maxHeight,
                    android.R.attr.lines,
                    android.R.attr.height,
                    android.R.attr.minLines,
                    android.R.attr.minHeight,
                    android.R.attr.maxEms,
                    android.R.attr.maxWidth,
                    android.R.attr.ems,
                    android.R.attr.width,
                    android.R.attr.minEms,
                    android.R.attr.minWidth,
                    android.R.attr.gravity,
                    android.R.attr.hint,
                    android.R.attr.text,
                    android.R.attr.scrollHorizontally,
                    android.R.attr.singleLine,
                    android.R.attr.ellipsize,
                    android.R.attr.marqueeRepeatLimit,
                    android.R.attr.includeFontPadding,
                    android.R.attr.cursorVisible,
                    android.R.attr.maxLength,
                    android.R.attr.textScaleX,
                    android.R.attr.freezesText,
                    android.R.attr.shadowColor,
                    android.R.attr.shadowDx,
                    android.R.attr.shadowDy,
                    android.R.attr.shadowRadius,
                    android.R.attr.enabled,
                    android.R.attr.textColorHighlight,
                    android.R.attr.textColor,
                    android.R.attr.textColorHint,
                    android.R.attr.textColorLink,
                    android.R.attr.textSize,
                    android.R.attr.typeface,
                    android.R.attr.textStyle,
                    android.R.attr.password,
                    android.R.attr.lineSpacingExtra,
                    android.R.attr.lineSpacingMultiplier,
                    android.R.attr.inputType,
                    android.R.attr.imeOptions,
                    android.R.attr.imeActionLabel,
                    android.R.attr.imeActionId,
                    android.R.attr.privateImeOptions,
                    android.R.attr.textSelectHandleLeft,
                    android.R.attr.textSelectHandleRight,
                    android.R.attr.textSelectHandle,
                    android.R.attr.textIsSelectable,
                    android.R.attr.textAllCaps
            };

            // The array must be sorted in ascending order, otherwise obtainStyledAttributes()
            // might fail to find some attributes
            Arrays.sort(attributes);
            TypedArray array = ctx.obtainStyledAttributes(typedValue.data, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            textSize = array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.textSize), textSize);
            styleIndex = array.getInt(sortedAttrs.indexOf(android.R.attr.textStyle), styleIndex);
            typefaceIndex = array.getInt(sortedAttrs.indexOf(android.R.attr.typeface), typefaceIndex);
            textColorHighlight = array.getColor(sortedAttrs.indexOf(android.R.attr.textColorHighlight), textColorHighlight);
            allCaps = array.getBoolean(sortedAttrs.indexOf(android.R.attr.textAllCaps), allCaps);

            ColorStateList textColor = array.getColorStateList(sortedAttrs.indexOf(android.R.attr.textColor));
            ColorStateList textColorHint = array.getColorStateList(sortedAttrs.indexOf(android.R.attr.textColorHint));
            ColorStateList textColorLink = array.getColorStateList(sortedAttrs.indexOf(android.R.attr.textColorLink));

            json.put("TextAppearance_textSize", textSize);
            json.put("TextAppearance_textStyle", styleIndex);
            json.put("TextAppearance_typeface", typefaceIndex);
            json.put("TextAppearance_textColorHighlight", textColorHighlight);
            json.put("TextAppearance_textAllCaps", allCaps);
            if (textColor != null)
                json.put("TextAppearance_textColor", getColorStateList(textColor));
            if (textColorHint != null)
                json.put("TextAppearance_textColorHint", getColorStateList(textColorHint));
            if (textColorLink != null)
                json.put("TextAppearance_textColorLink", getColorStateList(textColorLink));

            json.put("TextView_editable", array.getBoolean(sortedAttrs.indexOf(android.R.attr.editable), false));
            json.put("TextView_inputMethod", array.getText(sortedAttrs.indexOf(android.R.attr.inputMethod)));
            json.put("TextView_numeric", array.getInt(sortedAttrs.indexOf(android.R.attr.numeric), 0));
            json.put("TextView_digits", array.getText(sortedAttrs.indexOf(android.R.attr.digits)));
            json.put("TextView_phoneNumber", array.getBoolean(sortedAttrs.indexOf(android.R.attr.phoneNumber), false));
            json.put("TextView_autoText", array.getBoolean(sortedAttrs.indexOf(android.R.attr.autoText), false));
            json.put("TextView_capitalize", array.getInt(sortedAttrs.indexOf(android.R.attr.capitalize), -1));
            json.put("TextView_bufferType", array.getInt(sortedAttrs.indexOf(android.R.attr.bufferType), 0));
            json.put("TextView_selectAllOnFocus", array.getBoolean(sortedAttrs.indexOf(android.R.attr.selectAllOnFocus), false));
            json.put("TextView_autoLink", array.getInt(sortedAttrs.indexOf(android.R.attr.autoLink), 0));
            json.put("TextView_linksClickable", array.getBoolean(sortedAttrs.indexOf(android.R.attr.linksClickable), true));
            json.put("TextView_drawableLeft", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.drawableLeft)), styleName + "_TextView_drawableLeft", null));
            json.put("TextView_drawableTop", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.drawableTop)), styleName + "_TextView_drawableTop", null));
            json.put("TextView_drawableRight", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.drawableRight)), styleName + "_TextView_drawableRight", null));
            json.put("TextView_drawableBottom", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.drawableBottom)), styleName + "_TextView_drawableBottom", null));
            json.put("TextView_drawableStart", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.drawableStart)), styleName + "_TextView_drawableStart", null));
            json.put("TextView_drawableEnd", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.drawableEnd)), styleName + "_TextView_drawableEnd", null));
            json.put("TextView_maxLines", array.getInt(sortedAttrs.indexOf(android.R.attr.maxLines), -1));
            json.put("TextView_drawablePadding", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.drawablePadding), 0));

            try {
                json.put("TextView_textCursorDrawable", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.textCursorDrawable)), styleName + "_TextView_textCursorDrawable", null));
            } catch (Exception e_) {
                json.put("TextView_textCursorDrawable", getDrawable(m_context.getResources().getDrawable(array.getResourceId(sortedAttrs.indexOf(android.R.attr.textCursorDrawable), 0), m_theme), styleName + "_TextView_textCursorDrawable", null));
            }

            json.put("TextView_maxLines", array.getInt(sortedAttrs.indexOf(android.R.attr.maxLines), -1));
            json.put("TextView_maxHeight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.maxHeight), -1));
            json.put("TextView_lines", array.getInt(sortedAttrs.indexOf(android.R.attr.lines), -1));
            json.put("TextView_height", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.height), -1));
            json.put("TextView_minLines", array.getInt(sortedAttrs.indexOf(android.R.attr.minLines), -1));
            json.put("TextView_minHeight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.minHeight), -1));
            json.put("TextView_maxEms", array.getInt(sortedAttrs.indexOf(android.R.attr.maxEms), -1));
            json.put("TextView_maxWidth", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.maxWidth), -1));
            json.put("TextView_ems", array.getInt(sortedAttrs.indexOf(android.R.attr.ems), -1));
            json.put("TextView_width", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.width), -1));
            json.put("TextView_minEms", array.getInt(sortedAttrs.indexOf(android.R.attr.minEms), -1));
            json.put("TextView_minWidth", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.minWidth), -1));
            json.put("TextView_gravity", array.getInt(sortedAttrs.indexOf(android.R.attr.gravity), -1));
            json.put("TextView_hint", array.getText(sortedAttrs.indexOf(android.R.attr.hint)));
            json.put("TextView_text", array.getText(sortedAttrs.indexOf(android.R.attr.text)));
            json.put("TextView_scrollHorizontally", array.getBoolean(sortedAttrs.indexOf(android.R.attr.scrollHorizontally), false));
            json.put("TextView_singleLine", array.getBoolean(sortedAttrs.indexOf(android.R.attr.singleLine), false));
            json.put("TextView_ellipsize", array.getInt(sortedAttrs.indexOf(android.R.attr.ellipsize), -1));
            json.put("TextView_marqueeRepeatLimit", array.getInt(sortedAttrs.indexOf(android.R.attr.marqueeRepeatLimit), 3));
            json.put("TextView_includeFontPadding", array.getBoolean(sortedAttrs.indexOf(android.R.attr.includeFontPadding), true));
            json.put("TextView_cursorVisible", array.getBoolean(sortedAttrs.indexOf(android.R.attr.maxLength), true));
            json.put("TextView_maxLength", array.getInt(sortedAttrs.indexOf(android.R.attr.maxLength), -1));
            json.put("TextView_textScaleX", array.getFloat(sortedAttrs.indexOf(android.R.attr.textScaleX), 1.0f));
            json.put("TextView_freezesText", array.getBoolean(sortedAttrs.indexOf(android.R.attr.freezesText), false));
            json.put("TextView_shadowColor", array.getInt(sortedAttrs.indexOf(android.R.attr.shadowColor), 0));
            json.put("TextView_shadowDx", array.getFloat(sortedAttrs.indexOf(android.R.attr.shadowDx), 0));
            json.put("TextView_shadowDy", array.getFloat(sortedAttrs.indexOf(android.R.attr.shadowDy), 0));
            json.put("TextView_shadowRadius", array.getFloat(sortedAttrs.indexOf(android.R.attr.shadowRadius), 0));
            json.put("TextView_enabled", array.getBoolean(sortedAttrs.indexOf(android.R.attr.enabled), true));
            json.put("TextView_password", array.getBoolean(sortedAttrs.indexOf(android.R.attr.password), false));
            json.put("TextView_lineSpacingExtra", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.lineSpacingExtra), 0));
            json.put("TextView_lineSpacingMultiplier", array.getFloat(sortedAttrs.indexOf(android.R.attr.lineSpacingMultiplier), 1.0f));
            json.put("TextView_inputType", array.getInt(sortedAttrs.indexOf(android.R.attr.inputType), EditorInfo.TYPE_NULL));
            json.put("TextView_imeOptions", array.getInt(sortedAttrs.indexOf(android.R.attr.imeOptions), EditorInfo.IME_NULL));
            json.put("TextView_imeActionLabel", array.getText(sortedAttrs.indexOf(android.R.attr.imeActionLabel)));
            json.put("TextView_imeActionId", array.getInt(sortedAttrs.indexOf(android.R.attr.imeActionId), 0));
            json.put("TextView_privateImeOptions", array.getString(sortedAttrs.indexOf(android.R.attr.privateImeOptions)));

            try {
                json.put("TextView_textSelectHandleLeft", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.textSelectHandleLeft)), styleName + "_TextView_textSelectHandleLeft", null));
            } catch (Exception _e) {
                json.put("TextView_textSelectHandleLeft", getDrawable(m_context.getResources().getDrawable(array.getResourceId(sortedAttrs.indexOf(android.R.attr.textSelectHandleLeft), 0), m_theme), styleName + "_TextView_textSelectHandleLeft", null));
            }

            try {
                json.put("TextView_textSelectHandleRight", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.textSelectHandleRight)), styleName + "_TextView_textSelectHandleRight", null));
            } catch (Exception _e) {
                json.put("TextView_textSelectHandleRight", getDrawable(m_context.getResources().getDrawable(array.getResourceId(sortedAttrs.indexOf(android.R.attr.textSelectHandleRight), 0), m_theme), styleName + "_TextView_textSelectHandleRight", null));
            }

            try {
                json.put("TextView_textSelectHandle", getDrawable(array.getDrawable(sortedAttrs.indexOf(android.R.attr.textSelectHandle)), styleName + "_TextView_textSelectHandle", null));
            } catch (Exception _e) {
                json.put("TextView_textSelectHandle", getDrawable(m_context.getResources().getDrawable(array.getResourceId(sortedAttrs.indexOf(android.R.attr.textSelectHandle), 0), m_theme), styleName + "_TextView_textSelectHandle", null));
            }
            json.put("TextView_textIsSelectable", array.getBoolean(sortedAttrs.indexOf(android.R.attr.textIsSelectable), false));
            array.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    public JSONObject extractImageViewInformation(int styleName, String qtClassName) {
        JSONObject json = new JSONObject();
        try {
            extractViewInformation(styleName, json, qtClassName);

            int[] attributes = new int[]{
                    android.R.attr.src,
                    android.R.attr.baselineAlignBottom,
                    android.R.attr.adjustViewBounds,
                    android.R.attr.maxWidth,
                    android.R.attr.maxHeight,
                    android.R.attr.scaleType,
                    android.R.attr.cropToPadding,
                    android.R.attr.tint

            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(styleName, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable drawable = array.getDrawable(sortedAttrs.indexOf(android.R.attr.src));
            if (drawable != null)
                json.put("ImageView_src", getDrawable(drawable, styleName + "_ImageView_src", null));

            json.put("ImageView_baselineAlignBottom", array.getBoolean(sortedAttrs.indexOf(android.R.attr.baselineAlignBottom), false));
            json.put("ImageView_adjustViewBounds", array.getBoolean(sortedAttrs.indexOf(android.R.attr.baselineAlignBottom), false));
            json.put("ImageView_maxWidth", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.maxWidth), Integer.MAX_VALUE));
            json.put("ImageView_maxHeight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.maxHeight), Integer.MAX_VALUE));
            int index = array.getInt(sortedAttrs.indexOf(android.R.attr.scaleType), -1);
            if (index >= 0)
                json.put("ImageView_scaleType", sScaleTypeArray[index]);

            int tint = array.getInt(sortedAttrs.indexOf(android.R.attr.tint), 0);
            if (tint != 0)
                json.put("ImageView_tint", tint);

            json.put("ImageView_cropToPadding", array.getBoolean(sortedAttrs.indexOf(android.R.attr.cropToPadding), false));
            array.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    void extractCompoundButton(SimpleJsonWriter jsonWriter, int styleName, String className, String qtClass) {
        JSONObject json = extractTextAppearanceInformation(styleName, qtClass);

        TypedValue typedValue = new TypedValue();
        Context ctx = new ContextThemeWrapper(m_context, m_theme);
        ctx.getTheme().resolveAttribute(styleName, typedValue, true);
        final int[] attributes = new int[]{android.R.attr.button};
        TypedArray array = ctx.obtainStyledAttributes(typedValue.data, attributes);
        Drawable drawable = array.getDrawable(0);
        array.recycle();

        try {
            if (drawable != null)
                json.put("CompoundButton_button", getDrawable(drawable, styleName + "_CompoundButton_button", null));
            jsonWriter.name(className).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractProgressBarInfo(JSONObject json, int styleName) {
        try {
            final int[] attributes = new int[]{
                    android.R.attr.minWidth,
                    android.R.attr.maxWidth,
                    android.R.attr.minHeight,
                    android.R.attr.maxHeight,
                    android.R.attr.indeterminateDuration,
                    android.R.attr.progressDrawable,
                    android.R.attr.indeterminateDrawable
            };

            // The array must be sorted in ascending order, otherwise obtainStyledAttributes()
            // might fail to find some attributes
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(styleName, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            json.put("ProgressBar_indeterminateDuration", array.getInt(sortedAttrs.indexOf(android.R.attr.indeterminateDuration), 4000));
            json.put("ProgressBar_minWidth", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.minWidth), 24));
            json.put("ProgressBar_maxWidth", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.maxWidth), 48));
            json.put("ProgressBar_minHeight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.minHeight), 24));
            json.put("ProgressBar_maxHeight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.maxHeight), 28));
            json.put("ProgressBar_progress_id", android.R.id.progress);
            json.put("ProgressBar_secondaryProgress_id", android.R.id.secondaryProgress);

            Drawable drawable = array.getDrawable(sortedAttrs.indexOf(android.R.attr.progressDrawable));
            if (drawable != null)
                json.put("ProgressBar_progressDrawable", getDrawable(drawable,
                        styleName + "_ProgressBar_progressDrawable", null));

            drawable = array.getDrawable(sortedAttrs.indexOf(android.R.attr.indeterminateDrawable));
            if (drawable != null)
                json.put("ProgressBar_indeterminateDrawable", getDrawable(drawable,
                        styleName + "_ProgressBar_indeterminateDrawable", null));

            array.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractProgressBar(SimpleJsonWriter writer, int styleName, String className, String qtClass) {
        JSONObject json = extractTextAppearanceInformation(android.R.attr.progressBarStyle, qtClass);
        try {
            extractProgressBarInfo(json, styleName);
            writer.name(className).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractAbsSeekBar(SimpleJsonWriter jsonWriter) {
        JSONObject json = extractTextAppearanceInformation(android.R.attr.seekBarStyle, "QSlider");
        extractProgressBarInfo(json, android.R.attr.seekBarStyle);
        try {
            int[] attributes = new int[]{
                    android.R.attr.thumb,
                    android.R.attr.thumbOffset
            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.seekBarStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable d = array.getDrawable(sortedAttrs.indexOf(android.R.attr.thumb));
            if (d != null)
                json.put("SeekBar_thumb", getDrawable(d, android.R.attr.seekBarStyle + "_SeekBar_thumb", null));
            json.put("SeekBar_thumbOffset", array.getDimensionPixelOffset(sortedAttrs.indexOf(android.R.attr.thumbOffset), -1));
            array.recycle();
            jsonWriter.name("seekBarStyle").value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractSwitch(SimpleJsonWriter jsonWriter) {
        JSONObject json = new JSONObject();
        try {
            int[] attributes = new int[]{
                    android.R.attr.thumb,
                    android.R.attr.track,
                    android.R.attr.switchTextAppearance,
                    android.R.attr.textOn,
                    android.R.attr.textOff,
                    android.R.attr.switchMinWidth,
                    android.R.attr.switchPadding,
                    android.R.attr.thumbTextPadding,
                    android.R.attr.showText,
                    android.R.attr.splitTrack
            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.switchStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable thumb = array.getDrawable(sortedAttrs.indexOf(android.R.attr.thumb));
            if (thumb != null)
                json.put("Switch_thumb", getDrawable(thumb, android.R.attr.switchStyle + "_Switch_thumb", null));

            Drawable track = array.getDrawable(sortedAttrs.indexOf(android.R.attr.track));
            if (track != null)
                json.put("Switch_track", getDrawable(track, android.R.attr.switchStyle + "_Switch_track", null));

            json.put("Switch_textOn", array.getText(sortedAttrs.indexOf(android.R.attr.textOn)));
            json.put("Switch_textOff", array.getText(sortedAttrs.indexOf(android.R.attr.textOff)));
            json.put("Switch_switchMinWidth", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.switchMinWidth), 0));
            json.put("Switch_switchPadding", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.switchPadding), 0));
            json.put("Switch_thumbTextPadding", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.thumbTextPadding), 0));
            json.put("Switch_showText", array.getBoolean(sortedAttrs.indexOf(android.R.attr.showText), true));
            json.put("Switch_splitTrack", array.getBoolean(sortedAttrs.indexOf(android.R.attr.splitTrack), false));

            // Get textAppearance values
            final int textAppearanceId = array.getResourceId(sortedAttrs.indexOf(android.R.attr.switchTextAppearance), -1);
            json.put("Switch_switchTextAppearance", extractTextAppearance(textAppearanceId, true));

            array.recycle();
            jsonWriter.name("switchStyle").value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    JSONObject extractCheckedTextView(String itemName) {
        JSONObject json = extractTextAppearanceInformation(android.R.attr.checkedTextViewStyle, itemName);
        try {
            int[] attributes = new int[]{
                    android.R.attr.checkMark,
            };

            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.switchStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable drawable = array.getDrawable(sortedAttrs.indexOf(android.R.attr.checkMark));
            if (drawable != null)
                json.put("CheckedTextView_checkMark", getDrawable(drawable, itemName + "_CheckedTextView_checkMark", null));
            array.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject extractItemStyle(int resourceId, String itemName)
    {
        try {
            XmlResourceParser parser = m_context.getResources().getLayout(resourceId);
            int type = parser.next();
            while (type != XmlPullParser.START_TAG && type != XmlPullParser.END_DOCUMENT)
                type = parser.next();

            if (type != XmlPullParser.START_TAG)
                return null;

            AttributeSet attributes = Xml.asAttributeSet(parser);
            String name = parser.getName();
            if (name.equals("TextView"))
                return extractTextAppearanceInformation(android.R.attr.textViewStyle, itemName, android.R.attr.textAppearanceListItem, attributes);
            else if (name.equals("CheckedTextView"))
                return extractCheckedTextView(itemName);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    private void extractItemsStyle(SimpleJsonWriter jsonWriter) {
        try {
            JSONObject itemStyle = extractItemStyle(android.R.layout.simple_list_item_1, "simple_list_item");
            if (itemStyle != null)
                jsonWriter.name("simple_list_item").value(itemStyle);
            itemStyle = extractItemStyle(android.R.layout.simple_list_item_checked, "simple_list_item_checked");
            if (itemStyle != null)
                jsonWriter.name("simple_list_item_checked").value(itemStyle);
            itemStyle = extractItemStyle(android.R.layout.simple_list_item_multiple_choice, "simple_list_item_multiple_choice");
            if (itemStyle != null)
                jsonWriter.name("simple_list_item_multiple_choice").value(itemStyle);
            itemStyle = extractItemStyle(android.R.layout.simple_list_item_single_choice, "simple_list_item_single_choice");
            if (itemStyle != null)
                jsonWriter.name("simple_list_item_single_choice").value(itemStyle);
            itemStyle = extractItemStyle(android.R.layout.simple_spinner_item, "simple_spinner_item");
            if (itemStyle != null)
                jsonWriter.name("simple_spinner_item").value(itemStyle);
            itemStyle = extractItemStyle(android.R.layout.simple_spinner_dropdown_item, "simple_spinner_dropdown_item");
            if (itemStyle != null)
                jsonWriter.name("simple_spinner_dropdown_item").value(itemStyle);
            itemStyle = extractItemStyle(android.R.layout.simple_dropdown_item_1line, "simple_dropdown_item_1line");
            if (itemStyle != null)
                jsonWriter.name("simple_dropdown_item_1line").value(itemStyle);
            itemStyle = extractItemStyle(android.R.layout.simple_selectable_list_item, "simple_selectable_list_item");
            if (itemStyle != null)
                jsonWriter.name("simple_selectable_list_item").value(itemStyle);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractListView(SimpleJsonWriter writer) {
        JSONObject json = extractTextAppearanceInformation(android.R.attr.listViewStyle, "QListView");
        try {
            int[] attributes = new int[]{
                    android.R.attr.divider,
                    android.R.attr.dividerHeight
            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.listViewStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable divider = array.getDrawable(sortedAttrs.indexOf(android.R.attr.divider));
            if (divider != null)
                json.put("ListView_divider", getDrawable(divider, android.R.attr.listViewStyle + "_ListView_divider", null));

            json.put("ListView_dividerHeight", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.dividerHeight), 0));

            array.recycle();
            writer.name("listViewStyle").value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractCalendar(SimpleJsonWriter writer) {
        JSONObject json = extractTextAppearanceInformation(android.R.attr.calendarViewStyle, "QCalendarWidget");
        try {
            int[] attributes = new int[]{
                    android.R.attr.firstDayOfWeek,
                    android.R.attr.focusedMonthDateColor,
                    android.R.attr.selectedWeekBackgroundColor,
                    android.R.attr.showWeekNumber,
                    android.R.attr.shownWeekCount,
                    android.R.attr.unfocusedMonthDateColor,
                    android.R.attr.weekNumberColor,
                    android.R.attr.weekSeparatorLineColor,
                    android.R.attr.selectedDateVerticalBar,
                    android.R.attr.dateTextAppearance,
                    android.R.attr.weekDayTextAppearance
            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.calendarViewStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable d = array.getDrawable(sortedAttrs.indexOf(android.R.attr.selectedDateVerticalBar));
            if (d != null)
                json.put("CalendarView_selectedDateVerticalBar", getDrawable(d, android.R.attr.calendarViewStyle + "_CalendarView_selectedDateVerticalBar", null));

            int textAppearanceId = array.getResourceId(sortedAttrs.indexOf(android.R.attr.dateTextAppearance), -1);
            json.put("CalendarView_dateTextAppearance", extractTextAppearance(textAppearanceId, true));
            textAppearanceId = array.getResourceId(sortedAttrs.indexOf(android.R.attr.weekDayTextAppearance), -1);
            json.put("CalendarView_weekDayTextAppearance", extractTextAppearance(textAppearanceId, true));


            json.put("CalendarView_firstDayOfWeek", array.getInt(sortedAttrs.indexOf(android.R.attr.firstDayOfWeek), 0));
            json.put("CalendarView_focusedMonthDateColor", array.getColor(sortedAttrs.indexOf(android.R.attr.focusedMonthDateColor), 0));
            json.put("CalendarView_selectedWeekBackgroundColor", array.getColor(sortedAttrs.indexOf(android.R.attr.selectedWeekBackgroundColor), 0));
            json.put("CalendarView_showWeekNumber", array.getBoolean(sortedAttrs.indexOf(android.R.attr.showWeekNumber), true));
            json.put("CalendarView_shownWeekCount", array.getInt(sortedAttrs.indexOf(android.R.attr.shownWeekCount), 6));
            json.put("CalendarView_unfocusedMonthDateColor", array.getColor(sortedAttrs.indexOf(android.R.attr.unfocusedMonthDateColor), 0));
            json.put("CalendarView_weekNumberColor", array.getColor(sortedAttrs.indexOf(android.R.attr.weekNumberColor), 0));
            json.put("CalendarView_weekSeparatorLineColor", array.getColor(sortedAttrs.indexOf(android.R.attr.weekSeparatorLineColor), 0));
            array.recycle();
            writer.name("calendarViewStyle").value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractToolBar(SimpleJsonWriter writer) {
        JSONObject json = extractTextAppearanceInformation(android.R.attr.toolbarStyle, "QToolBar");
        try {
            int[] attributes = new int[]{
                    android.R.attr.background,
                    android.R.attr.backgroundStacked,
                    android.R.attr.backgroundSplit,
                    android.R.attr.divider,
                    android.R.attr.itemPadding
            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.toolbarStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable d = array.getDrawable(sortedAttrs.indexOf(android.R.attr.background));
            if (d != null)
                json.put("ActionBar_background", getDrawable(d, android.R.attr.toolbarStyle + "_ActionBar_background", null));

            d = array.getDrawable(sortedAttrs.indexOf(android.R.attr.backgroundStacked));
            if (d != null)
                json.put("ActionBar_backgroundStacked", getDrawable(d, android.R.attr.toolbarStyle + "_ActionBar_backgroundStacked", null));

            d = array.getDrawable(sortedAttrs.indexOf(android.R.attr.backgroundSplit));
            if (d != null)
                json.put("ActionBar_backgroundSplit", getDrawable(d, android.R.attr.toolbarStyle + "_ActionBar_backgroundSplit", null));

            d = array.getDrawable(sortedAttrs.indexOf(android.R.attr.divider));
            if (d != null)
                json.put("ActionBar_divider", getDrawable(d, android.R.attr.toolbarStyle + "_ActionBar_divider", null));

            json.put("ActionBar_itemPadding", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.itemPadding), 0));

            array.recycle();
            writer.name("actionBarStyle").value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractTabBar(SimpleJsonWriter writer) {
        JSONObject json = extractTextAppearanceInformation(android.R.attr.actionBarTabBarStyle, "QTabBar");
        try {
            int[] attributes = new int[]{
                    android.R.attr.showDividers,
                    android.R.attr.dividerPadding,
                    android.R.attr.divider
            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.actionBarTabStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable d = array.getDrawable(sortedAttrs.indexOf(android.R.attr.divider));
            if (d != null)
                json.put("LinearLayout_divider", getDrawable(d, android.R.attr.actionBarTabStyle + "_LinearLayout_divider", null));
            json.put("LinearLayout_showDividers", array.getInt(sortedAttrs.indexOf(android.R.attr.showDividers), 0));
            json.put("LinearLayout_dividerPadding", array.getDimensionPixelSize(sortedAttrs.indexOf(android.R.attr.dividerPadding), 0));

            array.recycle();
            writer.name("actionBarTabBarStyle").value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void extractWindow(SimpleJsonWriter writer) {
        JSONObject json = new JSONObject();
        try {
            int[] attributes = new int[]{
                    android.R.attr.windowBackground,
                    android.R.attr.windowFrame
            };
            Arrays.sort(attributes);
            TypedArray array = obtainStyledAttributes(android.R.attr.popupWindowStyle, attributes);
            ArrayList<Integer> sortedAttrs = getArrayListFromIntArray(attributes);

            Drawable background = array.getDrawable(sortedAttrs.indexOf(android.R.attr.windowBackground));
            if (background != null)
                json.put("Window_windowBackground", getDrawable(background, android.R.attr.popupWindowStyle + "_Window_windowBackground", null));

            Drawable frame = array.getDrawable(sortedAttrs.indexOf(android.R.attr.windowFrame));
            if (frame != null)
                json.put("Window_windowFrame", getDrawable(frame, android.R.attr.popupWindowStyle + "_Window_windowFrame", null));
            array.recycle();
            writer.name("windowStyle").value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private JSONObject extractDefaultPalette() {
        JSONObject json = extractTextAppearance(android.R.attr.textAppearance);
        try {
            json.put("defaultBackgroundColor", defaultBackgroundColor);
            json.put("defaultTextColorPrimary", defaultTextColor);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    static class SimpleJsonWriter {
        private final OutputStreamWriter m_writer;
        private boolean m_addComma = false;
        private int m_indentLevel = 0;

        public SimpleJsonWriter(String filePath) throws FileNotFoundException {
            m_writer = new OutputStreamWriter(new FileOutputStream(filePath));
        }

        public void close() throws IOException {
            m_writer.close();
        }

        private void writeIndent() throws IOException {
            m_writer.write(" ", 0, m_indentLevel);
        }

        void beginObject() throws IOException {
            writeIndent();
            m_writer.write("{\n");
            ++m_indentLevel;
            m_addComma = false;
        }

        void endObject() throws IOException {
            m_writer.write("\n");
            writeIndent();
            m_writer.write("}\n");
            --m_indentLevel;
            m_addComma = false;
        }

        SimpleJsonWriter name(String name) throws IOException {
            if (m_addComma) {
                m_writer.write(",\n");
            }
            writeIndent();
            m_writer.write(JSONObject.quote(name) + ": ");
            m_addComma = true;
            return this;
        }

        void value(JSONObject value) throws IOException {
            m_writer.write(value.toString());
        }
    }

    static class DrawableCache {
        JSONObject object;
        Object drawable;
        public DrawableCache(JSONObject json, Object drawable) {
            object = json;
            this.drawable = drawable;
        }
    }
}
