import hilog from '@ohos.hilog';
import {LOG_DOMAIN, LOG_TAG} from '../common/QtAppConstants';

interface UiExtensionWindowNativeNodeCreateInfo {
  xComponentId: string
  onAttach?: (uiContext: UIContext) => void
  onAppear: () => void
  onDisAppear: () => void
}

@Entry
@Component
struct UiExtensionWindowNativeNode {
  @State private createInfo: UiExtensionWindowNativeNodeCreateInfo | undefined = undefined

  aboutToAppear(): void {
    const localStorage  = this.getUIContext().getSharedLocalStorage();
    if (localStorage ) {
      this.createInfo = localStorage.get<UiExtensionWindowNativeNodeCreateInfo>('createInfo');
    }
  }

  build() {
    Stack() {
      if (this.createInfo !== undefined) {
        XComponent({
          type: XComponentType.NODE,
          id: this.createInfo.xComponentId,
          libraryname: 'qohos'
        })
          .onAppear(this.createInfo.onAppear)
          .onDisAppear(this.createInfo.onDisAppear)
          .onAttach(() => {
            if (this.createInfo !== undefined) {
              this.createInfo.onAttach?.(this.getUIContext());
            } else {
              hilog.error(LOG_DOMAIN, LOG_TAG, 'Cannot call onAttach, createInfo is undefined');
            }
          })
          .width('100%')
          .height('100%')
      }
    }
    .width('100%')
    .height('100%')
  }
}
