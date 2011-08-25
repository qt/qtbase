var sm = new ScriptSharedMemory;
sm.setKey("readonly_segfault");
sm.createReadOnly(1024);
var data = sm.set(0, "a");
