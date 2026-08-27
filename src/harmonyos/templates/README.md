# Qt Template for HarmonyOS

This DevEco Studio project provides the baseline application shell used to launch Qt applications on HarmonyOS.

For prerequisites (DevEco Studio version, HarmonyOS SDK API level, required third-party packages), the Qt build procedure, and the recommended deployment workflow using `harmonydeployqt`, see:

- [Qt for HarmonyOS](https://wiki.qt.io/Qt_for_HarmonyOS) — overview and reference documentation
- [Building Qt6 for HarmonyOS](https://wiki.qt.io/Building_Qt6_for_HarmonyOS) — build and deployment guide

## Using the template directly

When integrating Qt manually instead of running `harmonydeployqt`:

1. In `entry/src/main/ets/common/QtAppConstants.ets`, set `APP_LIBRARY_NAME` to the file name of your application's shared library, e.g. `libwidgettest.so`.

2. Copy the application library and its dependencies — the HarmonyOS platform plugin (`libqohos.so`) and the required Qt6 modules (`libQt6Core.so`, `libQt6Gui.so`, `libQt6Widgets.so`, …) — into `entry/libs/<architecture>/`, where `<architecture>` is `arm64-v8a` for devices or `x86_64` for the emulator.

3. For Qt Quick applications, additionally copy QML module files into the following locations:
   - `.so` files (Qt Quick plugins) → `entry/libs/<architecture>/`
   - All other files (`qmldir`, `.qml`, `.js`, etc.) → `entry/src/main/resources/resfile/qml/`, preserving the original module directory structure.

   See [How to deploy a QML app](https://wiki.qt.io/Qt_for_HarmonyOS/user_development_guide/how_to_deply_qml_app) for details.

## Launch flags

The HarmonyOS platform plugin reads Want parameters when the ability is launched. In DevEco Studio's run configuration, extra flags can be passed using `--pb` for boolean values and `--ps` for string values, for example:

```
--pb io.qt.useDefaultUiAbilityInstanceInQt false --ps io.qt.appSharedLibNameOverride libmyapp.so
```

Commonly used flags:

| Flag | Type | Description |
|------|------|-------------|
| `io.qt.useDefaultUiAbilityInstanceInQt` | bool | Reuse the existing UI ability instance instead of creating a new one. |
| `io.qt.appSharedLibNameOverride` | string | Overrides `APP_LIBRARY_NAME` at launch time without modifying `QtAppConstants.ets`. |
