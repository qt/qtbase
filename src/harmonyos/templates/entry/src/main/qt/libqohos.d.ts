import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import AbilityStage from '@ohos.app.ability.AbilityStage';
import Want from '@ohos.app.ability.Want';
import Window from '@ohos.window'
import common from '@ohos.app.ability.common';
import { UIAbility } from '@kit.AbilityKit';

interface QtChildProcessParams {
  modules: object,
}

interface QtApplicationSetupParams {
  appContext: common.ApplicationContext,
  modules: object,
  appName: string,
  appArgs?: Array<string>,
  abilityClassName: string,
  _unusedQChildProcess: object,
}

export const handleAbilityOnPrepareToTerminate: (ability: UIAbility) => boolean;
export const handleAbilityOnPrepareToTerminateAsync: (ability: UIAbility) => Promise<boolean>;
export const handleAbilityOnCreate: (ability: UIAbility, want: Want, launchParam: AbilityConstant.LaunchParam) => void;
export const handleAbilityOnDestroy: (ability: UIAbility) => void | Promise<void>;
export const handleAbilityOnNewWant: (ability: UIAbility, want: Want, launchParam: AbilityConstant.LaunchParam) => void;
export const handleAbilityOnWindowStageCreate: (ability: UIAbility, windowStage: Window.WindowStage) => void;
export const handleAbilityOnWindowStageRestore: (ability: UIAbility, windowStage: Window.WindowStage) => void;
export const handleAbilityOnForeground: (ability: UIAbility) => void;
export const handleAbilityOnBackground: (ability: UIAbility) => void;
export const handleAbilityOnWindowStageDestroy: (ability: UIAbility) => void;
export const handleAbilityOnContinue: (ability: UIAbility, wantParam: Record<string, Object>) => AbilityConstant.OnContinueResult | Promise<AbilityConstant.OnContinueResult>;

export const handleAbilityStageOnCreate: (abilityStage: AbilityStage) => void;
export const handleAbilityStageOnNewProcessRequest: (abilityStage: AbilityStage, want: Want) => string;
export const handleAbilityStageOnAcceptWant: (abilityStage: AbilityStage, want: Want) => string;
export const handleAbilityStageOnDestroy: (abilityStage: AbilityStage) => void;

export const runQtChildProcess: (params: QtChildProcessParams) => void;
export const setupQtApplication: (params: QtApplicationSetupParams) => void;
