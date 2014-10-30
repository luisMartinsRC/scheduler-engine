package com.sos.scheduler.engine.eventbus.annotated;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSourceEvent;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/** Eine {@link EventSubscription} für eine mit @{@link com.sos.scheduler.engine.eventbus.EventHandler} oder
 * @{@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode mit nur einem Parameter für das {@link Event}. */
public class SimpleMethodEventSubscription extends MethodEventSubscription {
    public SimpleMethodEventSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        super(annotatedObject, method);
        checkMethodParameterCount(method, 1, 1);
    }

    @Override protected final void invokeHandler(Event event) throws InvocationTargetException, IllegalAccessException {
        Event e = event instanceof EventSourceEvent<?>? ((EventSourceEvent<?>)event).event() : event;
        getMethod().invoke(getAnnotatedObject(), e);
    }

    @Override public String toString() {
        return getMethod().getDeclaringClass().getSimpleName() +"."+ getMethod().getName() +
                "("+ eventClass().getSimpleName() +")";
    }
}
