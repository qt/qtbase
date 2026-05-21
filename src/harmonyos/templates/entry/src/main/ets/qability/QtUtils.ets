import { getLocalStorageTsInterface } from './LocalStorageTsModule';
import { getOhosExportModulesFactories } from './OhosExportModules';
import { getQEmbeddedComponentCreatorTsInterface } from './QEmbeddedComponentCreatorTsModule';

export default class QtUtils {
  static getModulesFactoriesMapForQt(): object {
    let factories = getOhosExportModulesFactories();
    factories['LocalStorage'] = getLocalStorageTsInterface;
    factories['QEmbeddedComponentCreator'] = getQEmbeddedComponentCreatorTsInterface;
    return factories;
  }
}
