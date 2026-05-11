interface WindowNativeNodeCreateInfo {
  xComponentId: string
  onAppear: () => void
  onAttach: () => void
  onDisAppear: () => void
}

@Entry
@Component
struct WindowNativeNode {
  @State private createInfo: WindowNativeNodeCreateInfo | undefined = undefined

  aboutToAppear(): void {
    const localStorage  = this.getUIContext().getSharedLocalStorage();
    if (localStorage ) {
      this.createInfo = localStorage.get<WindowNativeNodeCreateInfo>('createInfo');
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
        .onAttach(this.createInfo.onAttach)
        .onAppear(this.createInfo.onAppear)
        .onDisAppear(this.createInfo.onDisAppear)
        .width('100%')
        .height('100%')
      }
    }
    .width('100%')
    .height('100%')
  }
}
