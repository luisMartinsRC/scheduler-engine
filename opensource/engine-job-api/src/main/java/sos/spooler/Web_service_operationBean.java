package sos.spooler;

import static sos.spooler.Beans.toBean;

/** Ein Proxy von {@link Web_service_operation}, mit Gettern und Settern für Skriptsprachen. */
public class Web_service_operationBean implements Bean<Web_service_operation> {
    private Web_service_operation delegate;

    Web_service_operationBean(Web_service_operation web_service_operation) {
        delegate = web_service_operation;
    }

    public Web_serviceBean getWeb_service() {
        return toBean(delegate.web_service());
    }

    public Web_service_requestBean getRequest() {
        return toBean(delegate.request());
    }

    public Web_service_responseBean getResponse() {
        return toBean(delegate.response());
    }

    @Override
    public Web_service_operation getDelegate() {
        return delegate;
    }
}