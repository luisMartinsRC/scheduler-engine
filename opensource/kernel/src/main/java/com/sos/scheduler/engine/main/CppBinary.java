package com.sos.scheduler.engine.main;

import static com.sos.scheduler.engine.common.system.OperatingSystem.operatingSystem;

public enum CppBinary {
    moduleFilename(operatingSystem.makeModuleFilenameSo("jobscheduler-engine")),  // jobscheduler-engine.dll oder libjobscheduler-engine.so
    exeFilename(operatingSystem.makeExecutableFilename("scheduler"));           // scheduler.exe oder scheduler

    private final String filename;

    CppBinary(String filename) {
        this.filename = filename;
    }

    public String filename() {
        return filename;
    }
}
