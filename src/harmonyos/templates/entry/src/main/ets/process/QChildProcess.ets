import ChildProcess from '@ohos.app.ability.ChildProcess';
import QtUtils from '../qability/QtUtils';
import hilog from '@ohos.hilog';
import qpa from 'libqohos.so';
import {LOG_DOMAIN, LOG_TAG} from '../common/QtAppConstants';

export default class QChildProcess extends ChildProcess {
  onStart() {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QChildProcess::onStart()');
    qpa.runQtChildProcess({
      modulesFactories: QtUtils.getModulesFactoriesMapForQt(),
    });
  }
}
