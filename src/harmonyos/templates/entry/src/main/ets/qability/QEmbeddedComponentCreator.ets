import { UIContext, ComponentContent } from '@kit.ArkUI';
import { webview } from '@kit.ArkWeb';

interface EmbeddedWebComponentAttributes {
  webviewController: webview.WebviewController,
  onErrorReceive: (event: OnErrorReceiveEvent) => void,
  onPageBegin: (event: OnPageBeginEvent) => void,
  onPageEnd: (event: OnPageVisibleEvent) => void,
  onProgressChange: (event: OnProgressChangeEvent) => void,
  onTitleReceive: (event: OnTitleReceiveEvent) => void,
}

@Builder
function createWebComponent(attributes: EmbeddedWebComponentAttributes) {
  Web({ src: '', controller: attributes.webviewController })
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
