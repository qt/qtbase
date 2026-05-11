import __ohos__abilityAccessCtrl from '@ohos.abilityAccessCtrl';
import __ohos__abilityAccessCtrl__Permissions from '@ohos.abilityAccessCtrl';
import __ohos__app__ability__AbilityConstant from '@ohos.app.ability.AbilityConstant';
import __ohos__app__ability__ConfigurationConstant from '@ohos.app.ability.ConfigurationConstant';
import __ohos__app__ability__childProcessManager from '@ohos.app.ability.childProcessManager';
import __ohos__app__ability__contextConstant from '@ohos.app.ability.contextConstant';
import __ohos__bluetooth__access from '@ohos.bluetooth.access';
import __ohos__bluetooth__connection from '@ohos.bluetooth.connection';
import __ohos__bluetooth__socket from '@ohos.bluetooth.socket';
import __ohos__bundle__bundleManager from '@ohos.bundle.bundleManager';
import __ohos__deviceInfo from '@ohos.deviceInfo'
import __ohos__display from '@ohos.display';
import __ohos__file__fileuri from '@ohos.file.fileuri';
import __ohos__file__picker from '@ohos.file.picker'
import __ohos__font from '@ohos.font';
import __ohos__graphics__text from '@ohos.graphics.text';
import __ohos__i18n from '@ohos.i18n';
import __ohos__inputMethod from '@ohos.inputMethod';
import __ohos__intl from '@ohos.intl';
import __ohos__multimedia__image from '@ohos.multimedia.image';
import __ohos__multimodalInput__inputDevice from '@ohos.multimodalInput.inputDevice';
import __ohos__multimodalInput__pointer from '@ohos.multimodalInput.pointer';
import __ohos__pasteboard from '@ohos.pasteboard';
import __ohos__settings from '@ohos.settings'
import __ohos__wifiManager from '@ohos.wifiManager'
import __ohos__window from '@ohos.window'
import { wantConstant as __kit__AbilityKit__wantConstant } from '@kit.AbilityKit';
import { accessibility as __kit__AccessibilityKit__accessibility } from '@kit.AccessibilityKit';
import { uniformTypeDescriptor as __kit__ArkData__uniformTypeDescriptor } from '@kit.ArkData';
import { uiExtension as __kit__ArkUI__uiExtension } from '@kit.ArkUI';
import { webview as __kit__ArkWeb__webview } from '@kit.ArkWeb';
import { textToSpeech as __kit__CoreSpeechKit__textToSpeech } from '@kit.CoreSpeechKit';
import { fileManagerService as __kit__FileManagerServiceKit__fileManagerService } from '@kit.FileManagerServiceKit';
import { geoLocationManager as __kit__LocationKit__geoLocationManager } from '@kit.LocationKit';
import { media as __kit__MediaKit__media } from '@kit.MediaKit';
import { notificationManager as __kit__NotificationKit__notificationManager } from '@kit.NotificationKit';
import { imageFeaturePicker as __kit__Penkit__imageFeaturePicker } from '@kit.Penkit';
import { systemShare as __kit__ShareKit__systemShare } from '@kit.ShareKit';
import { statusBarManager as __kit__StatusBarExtensionKit__statusBarManager } from '@kit.StatusBarExtensionKit';

export function getOhosExportModules(): object {
  return {
    '@kit.AbilityKit.wantConstant': __kit__AbilityKit__wantConstant,
    '@kit.AccessibilityKit.accessibility': __kit__AccessibilityKit__accessibility,
    '@kit.ArkData.uniformTypeDescriptor': __kit__ArkData__uniformTypeDescriptor,
    '@kit.ArkUI.uiExtension': __kit__ArkUI__uiExtension,
    '@kit.ArkWeb.webview': __kit__ArkWeb__webview,
    '@kit.CoreSpeechKit.textToSpeech': __kit__CoreSpeechKit__textToSpeech,
    '@kit.FileManagerServiceKit.fileManagerService': __kit__FileManagerServiceKit__fileManagerService,
    '@kit.LocationKit.geoLocationManager': __kit__LocationKit__geoLocationManager,
    '@kit.MediaKit.media': __kit__MediaKit__media,
    '@kit.NotificationKit.notificationManager': __kit__NotificationKit__notificationManager,
    '@kit.Penkit.imageFeaturePicker': __kit__Penkit__imageFeaturePicker,
    '@kit.ShareKit.systemShare': __kit__ShareKit__systemShare,
    '@kit.StatusBarExtensionKit.statusBarManager': __kit__StatusBarExtensionKit__statusBarManager,
    '@ohos.abilityAccessCtrl': __ohos__abilityAccessCtrl,
    '@ohos.abilityAccessCtrl.Permissions': __ohos__abilityAccessCtrl__Permissions,
    '@ohos.app.ability.AbilityConstant': __ohos__app__ability__AbilityConstant,
    '@ohos.app.ability.ConfigurationConstant': __ohos__app__ability__ConfigurationConstant,
    '@ohos.app.ability.childProcessManager': __ohos__app__ability__childProcessManager,
    '@ohos.app.ability.contextConstant': __ohos__app__ability__contextConstant,
    '@ohos.bluetooth.access': __ohos__bluetooth__access,
    '@ohos.bluetooth.connection': __ohos__bluetooth__connection,
    '@ohos.bluetooth.socket': __ohos__bluetooth__socket,
    '@ohos.bundle.bundleManager': __ohos__bundle__bundleManager,
    '@ohos.deviceInfo': __ohos__deviceInfo,
    '@ohos.display': __ohos__display,
    '@ohos.file.fileuri': __ohos__file__fileuri,
    '@ohos.file.picker': __ohos__file__picker,
    '@ohos.font': __ohos__font,
    '@ohos.graphics.text': __ohos__graphics__text,
    '@ohos.i18n': __ohos__i18n,
    '@ohos.inputMethod': __ohos__inputMethod,
    '@ohos.intl': __ohos__intl,
    '@ohos.multimedia.image': __ohos__multimedia__image,
    '@ohos.multimodalInput.inputDevice': __ohos__multimodalInput__inputDevice,
    '@ohos.multimodalInput.pointer': __ohos__multimodalInput__pointer,
    '@ohos.pasteboard': __ohos__pasteboard,
    '@ohos.settings': __ohos__settings,
    '@ohos.wifiManager': __ohos__wifiManager,
    '@ohos.window': __ohos__window,
  };
}
