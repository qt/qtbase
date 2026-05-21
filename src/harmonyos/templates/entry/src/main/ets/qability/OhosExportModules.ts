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
  };
}
