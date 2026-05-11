import { getLocalStorageTsInterface } from './LocalStorageTsModule';
import { getOhosExportModules } from './OhosExportModules';
import { getQEmbeddedComponentCreatorTsInterface } from './QEmbeddedComponentCreatorTsModule';

export default class QtUtils {
  static getModulesMapForQt(): object {
    let modules = getOhosExportModules();
    modules['LocalStorage'] = getLocalStorageTsInterface();
    modules['QEmbeddedComponentCreator'] = getQEmbeddedComponentCreatorTsInterface();
    return modules;
  }
}
