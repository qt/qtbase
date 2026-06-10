import { UIContext, ComponentContent } from '@kit.ArkUI';
import { webview } from '@kit.ArkWeb';

interface EmbeddedWebComponentAttributes {
  webviewController: webview.WebviewController,
  domStorageAccess: boolean,
  javaScriptAccess: boolean,
  fileAccess: boolean,
  onErrorReceive: (event: OnErrorReceiveEvent) => void,
  onPageBegin: (event: OnPageBeginEvent) => void,
  onPageEnd: (event: OnPageVisibleEvent) => void,
  onProgressChange: (event: OnProgressChangeEvent) => void,
  onTitleReceive: (event: OnTitleReceiveEvent) => void,
}

@Builder
function createWebComponent(attributes: EmbeddedWebComponentAttributes) {
  Web({ src: '', controller: attributes.webviewController })
    .domStorageAccess(attributes.domStorageAccess ?? false)
    .javaScriptAccess(attributes.javaScriptAccess ?? true)
    .fileAccess(attributes.fileAccess ?? false)
    .onErrorReceive(attributes.onErrorReceive)
    .onPageBegin(attributes.onPageBegin)
    .onPageEnd(attributes.onPageEnd)
    .onProgressChange(attributes.onProgressChange)
    .onTitleReceive(attributes.onTitleReceive);
}

export default class QEmbeddedComponentCreator {
  createEmbeddedWebComponent(
    appContext: UIContext,
    webComponentAttributes: EmbeddedWebComponentAttributes): ComponentContent<EmbeddedWebComponentAttributes> {
    return new ComponentContent<EmbeddedWebComponentAttributes>(
      appContext,
      wrapBuilder<[EmbeddedWebComponentAttributes]>(createWebComponent),
      webComponentAttributes);
  }
}
