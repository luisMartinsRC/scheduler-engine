package com.sos.scheduler.engine.data.order;

/**
 * \file OrderStateChangeEvent.java
 * \brief This event fired if the order of a job chain changed the state (next state starts) 
 *  
 * \class OrderStateChangeEvent
 * \brief This event fired if the order of a job chain changed the state (next state starts) 
 * 
 * \details
 * Beside the order object this event provides the state of the previously executed state.
 * 
 * \see
 * Order::set_state2 in Order.cxx
 *
 * \code
  \endcode
 *
 * \version 1.0 - 12.04.2011 12:04:02
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class OrderStateChangedEvent extends OrderEvent {
    private final OrderState previousState;

    public OrderStateChangedEvent(OrderKey key, OrderState previousState) {
        super(key);
        this.previousState = previousState;
    }

    public final OrderState getPreviousState() { 
        return previousState;
    }

    @Override public final String toString() {
        return super.toString() + ", previousState=" + previousState;
    }

    public static OrderStateChangedEvent of(String jobChainPath, String orderId, String state) {
        return new OrderStateChangedEvent(OrderKey.of(jobChainPath, orderId), new OrderState(state));
    }
}