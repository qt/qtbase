import __ohos__abilityAccessCtrl from '@ohos.abilityAccessCtrl';
import __ohos__abilityAccessCtrl__Permissions from '@ohos.abilityAccessCtrl';
import __ohos__accessibility from '@ohos.accessibility';
import __ohos__app__ability__AbilityConstant from '@ohos.app.ability.AbilityConstant';
import __ohos__app__ability__ConfigurationConstant from '@ohos.app.ability.ConfigurationConstant';
import __ohos__app__ability__childProcessManager from '@ohos.app.ability.childProcessManager';
import __ohos__app__ability__contextConstant from '@ohos.app.ability.contextConstant';
import __ohos__app__ability__wantConstant from '@ohos.app.ability.wantConstant';
import __ohos__arkui__uiExtension from '@ohos.arkui.uiExtension';
import __ohos__bluetooth__access from '@ohos.bluetooth.access';
import __ohos__bluetooth__connection from '@ohos.bluetooth.connection';
import __ohos__bluetooth__socket from '@ohos.bluetooth.socket';
import __ohos__bundle__bundleManager from '@ohos.bundle.bundleManager';
import __ohos__data__uniformTypeDescriptor from '@ohos.data.uniformTypeDescriptor';
import __ohos__deviceInfo from '@ohos.deviceInfo'
import __ohos__display from '@ohos.display';
import __ohos__file__fileuri from '@ohos.file.fileuri';
import __ohos__file__picker from '@ohos.file.picker'
import __ohos__font from '@ohos.font';
import __ohos__geoLocationManager from '@ohos.geoLocationManager';
import __ohos__graphics__text from '@ohos.graphics.text';
import __ohos__i18n from '@ohos.i18n';
import __ohos__inputMethod from '@ohos.inputMethod';
import __ohos__intl from '@ohos.intl';
import __ohos__multimedia__image from '@ohos.multimedia.image';
import __ohos__multimedia__media from '@ohos.multimedia.media';
import __ohos__multimodalInput__inputDevice from '@ohos.multimodalInput.inputDevice';
import __ohos__multimodalInput__pointer from '@ohos.multimodalInput.pointer';
import __ohos__notificationManager from '@ohos.notificationManager';
import __ohos__pasteboard from '@ohos.pasteboard';
import __ohos__settings from '@ohos.settings'
import __ohos__web__webview from '@ohos.web.webview';
import __ohos__wifiManager from '@ohos.wifiManager'
import __ohos__window from '@ohos.window'
import { textToSpeech as __kit__CoreSpeechKit__textToSpeech } from '@kit.CoreSpeechKit';
import { fileManagerService as __kit__FileManagerServiceKit__fileManagerService } from '@kit.FileManagerServiceKit';
import { imageFeaturePicker as __kit__Penkit__imageFeaturePicker } from '@kit.Penkit';
import { systemShare as __kit__ShareKit__systemShare } from '@kit.ShareKit';
import { statusBarManager as __kit__StatusBarExtensionKit__statusBarManager } from '@kit.StatusBarExtensionKit';

export function getOhosExportModules(): object {
  return {
    '@kit.CoreSpeechKit.textToSpeech': __kit__CoreSpeechKit__textToSpeech,
    '@kit.FileManagerServiceKit.fileManagerService': __kit__FileManagerServiceKit__fileManagerService,
    '@kit.Penkit.imageFeaturePicker': __kit__Penkit__imageFeaturePicker,
    '@kit.ShareKit.systemShare': __kit__ShareKit__systemShare,
    '@kit.StatusBarExtensionKit.statusBarManager': __kit__StatusBarExtensionKit__statusBarManager,
    '@ohos.abilityAccessCtrl': __ohos__abilityAccessCtrl,
    '@ohos.abilityAccessCtrl.Permissions': __ohos__abilityAccessCtrl__Permissions,
    '@ohos.accessibility': __ohos__accessibility,
    '@ohos.app.ability.AbilityConstant': __ohos__app__ability__AbilityConstant,
    '@ohos.app.ability.ConfigurationConstant': __ohos__app__ability__ConfigurationConstant,
    '@ohos.app.ability.childProcessManager': __ohos__app__ability__childProcessManager,
    '@ohos.app.ability.contextConstant': __ohos__app__ability__contextConstant,
    '@ohos.app.ability.wantConstant': __ohos__app__ability__wantConstant,
    '@ohos.arkui.uiExtension': __ohos__arkui__uiExtension,
    '@ohos.bluetooth.access': __ohos__bluetooth__access,
    '@ohos.bluetooth.connection': __ohos__bluetooth__connection,
    '@ohos.bluetooth.socket': __ohos__bluetooth__socket,
    '@ohos.bundle.bundleManager': __ohos__bundle__bundleManager,
    '@ohos.data.uniformTypeDescriptor': __ohos__data__uniformTypeDescriptor,
    '@ohos.deviceInfo': __ohos__deviceInfo,
    '@ohos.display': __ohos__display,
    '@ohos.file.fileuri': __ohos__file__fileuri,
    '@ohos.file.picker': __ohos__file__picker,
    '@ohos.font': __ohos__font,
    '@ohos.geoLocationManager': __ohos__geoLocationManager,
    '@ohos.graphics.text': __ohos__graphics__text,
    '@ohos.i18n': __ohos__i18n,
    '@ohos.inputMethod': __ohos__inputMethod,
    '@ohos.intl': __ohos__intl,
    '@ohos.multimedia.image': __ohos__multimedia__image,
    '@ohos.multimedia.media': __ohos__multimedia__media,
    '@ohos.multimodalInput.inputDevice': __ohos__multimodalInput__inputDevice,
    '@ohos.multimodalInput.pointer': __ohos__multimodalInput__pointer,
    '@ohos.notificationManager': __ohos__notificationManager,
    '@ohos.pasteboard': __ohos__pasteboard,
    '@ohos.settings': __ohos__settings,
    '@ohos.web.webview': __ohos__web__webview,
    '@ohos.wifiManager': __ohos__wifiManager,
    '@ohos.window': __ohos__window,
  };
}
