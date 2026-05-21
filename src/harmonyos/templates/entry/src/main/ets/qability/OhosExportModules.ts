import lazy { textToSpeech as __kit__CoreSpeechKit__textToSpeech } from '@kit.CoreSpeechKit';
import lazy { fileManagerService as __kit__FileManagerServiceKit__fileManagerService } from '@kit.FileManagerServiceKit';
import lazy { imageFeaturePicker as __kit__Penkit__imageFeaturePicker } from '@kit.Penkit';
import lazy { systemShare as __kit__ShareKit__systemShare } from '@kit.ShareKit';
import lazy { statusBarManager as __kit__StatusBarExtensionKit__statusBarManager } from '@kit.StatusBarExtensionKit';

export function getOhosExportModulesFactories(): object {
  return {
    '@kit.CoreSpeechKit.textToSpeech': () => __kit__CoreSpeechKit__textToSpeech,
    '@kit.FileManagerServiceKit.fileManagerService': () => __kit__FileManagerServiceKit__fileManagerService,
    '@kit.Penkit.imageFeaturePicker': () => __kit__Penkit__imageFeaturePicker,
    '@kit.ShareKit.systemShare': () => __kit__ShareKit__systemShare,
    '@kit.StatusBarExtensionKit.statusBarManager': () => __kit__StatusBarExtensionKit__statusBarManager,
  };
}
