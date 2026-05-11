import QEmbeddedComponentCreator from './QEmbeddedComponentCreator.ets';

function makeNewQEmbeddedComponentCreator(): QEmbeddedComponentCreator {
  return new QEmbeddedComponentCreator();
}

export function getQEmbeddedComponentCreatorTsInterface(): Record<string, object> {
  return {
    'makeNewQEmbeddedComponentCreator': makeNewQEmbeddedComponentCreator,
  }
}
