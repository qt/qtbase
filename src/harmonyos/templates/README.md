# Ohos Template For Qt Application

This DevEco Studio project is developed to serve codebase required to launch Qt applications in OpenHarmony operating system.

## Getting Started
### Prerequisites

- [HUAWEI DevEco Studio](https://developer.harmonyos.com/cn/develop/deveco-studio/)
- Compiled [ohos-qt5](https://gitlab.siilicloud.com/automotive/customers/qt/qt-for-openharmony/qt/qt5) sources
- Qt application project you want to run in OHOS

### How to run project on emulator/physical device

Project is pretty much ready for running, there's one thing that you have to change in code:

- in `entry/src/main/ets/qability/QAbility.ts` file change line containing `qtApplicationName`:

    ```
    private qtApplicationName = "libwidgettest.so";
    ```
    to your Qt application name.

Another thing is that, you have to deliver required shared library files to:
```
entry/libs/<architecture>/
```
folder. `<architecture>` might be for example:
- `x86_64`, if you're building for example for x86 emulator
- `arm64-v8a`, if you're building for example for Huawei MatePad Pro tablet

Required libraries are usually:
- end Qt application, for example `libwidgettest.so`
- OHOS QPA plugin from build Qt sources: `libqohos.so`
- dependency Qt5 libraries, for example: `libQt5Core.so`, `libQt5Gui.so`, `libQt5Widgets.so` (handy tool might be `readelf` to examine needed shared libraries)
