package com.sos.scheduler.engine.kernel.folder.events;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.folder.FileBased;

@ForCpp
public class FileBasedActivatedEvent extends ObjectEvent {
    private final FileBased fileBased;

    @ForCpp public FileBasedActivatedEvent(FileBased fileBased) {
        this.fileBased = fileBased;
    }

    @Override public FileBased getObject() {
        return fileBased;
    }
}