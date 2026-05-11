function makeNewLocalStorage(initializingProperties?: object): LocalStorage {
  return new LocalStorage(initializingProperties)
}

export function getLocalStorageTsInterface(): Record<string, object> {
  return {
    'makeNewLocalStorage': makeNewLocalStorage,
  }
}
