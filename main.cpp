#include <QCoreApplication>

#include <thread>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <iostream>
#include <QDebug>

#include <unistd.h>

#include "httplogic.h"

#define PORT 80

void listenerNewConnection_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
void listnerError_cb(struct evconnlistener *, void *);

void connectionWrite_cb(struct bufferevent *, void *);
void connectionRead_cb(struct bufferevent *, void *);
void connectionEvent_cb(struct bufferevent *, short, void *);


int main(int argc, char *argv[])
{
    struct event_base *pBase;
    struct evconnlistener *pListener;
    struct sockaddr_in Address;
    int nProcessessAmount = std::thread::hardware_concurrency() - 1;//2;

    pBase = event_base_new();
    if (!pBase) {
        fprintf(stderr, "ERROR: init libevent\n");
        return 1;
    }

    memset(&Address, 0, sizeof(Address));
    Address.sin_port = htons(PORT);
    Address.sin_family = AF_INET;

    pListener = evconnlistener_new_bind(pBase, listenerNewConnection_cb, NULL,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&Address,
        sizeof(Address));

    if (!pListener) {
        fprintf(stderr, "ERROR: creating a listener\n");
        return 1;
    }

    evconnlistener_set_error_cb(pListener, listnerError_cb);

    pid_t Pid;
    std::cout << "parent process " << getpid() << "\n";
    for(int i = 1; i <= nProcessessAmount; ++i) {
        Pid = fork();
        if(Pid == 0)
        {
            std::cout << "fork " << getpid() << " started\n";
            break;
        }
        else if(Pid == -1)
        {
            fprintf(stderr, "ERROR: forking\n");
            break;
        }

    }
    event_reinit(pBase);
    event_base_dispatch(pBase);
    evconnlistener_free(pListener);
    event_base_free(pBase);

    printf("exit\n");
    return 0;
}



void listenerNewConnection_cb(struct evconnlistener *pListener, evutil_socket_t nFileDescriptor,
                 struct sockaddr *SourceAddress, int nAddressLength, void *pData)
{
    struct event_base *pBase = evconnlistener_get_base(pListener);
    struct bufferevent *pBufferEvent;

    pBufferEvent = bufferevent_socket_new(pBase, nFileDescriptor, BEV_OPT_CLOSE_ON_FREE);
    if (!pBufferEvent) {
        fprintf(stderr, "ERROR: creating buffer\n");
        event_base_loopbreak(pBase);
        return;
    }
    bufferevent_setcb(pBufferEvent, connectionRead_cb, connectionWrite_cb, connectionEvent_cb, NULL);
    bufferevent_enable(pBufferEvent, EV_READ);
    bufferevent_disable(pBufferEvent, EV_WRITE);
}


void connectionWrite_cb(struct bufferevent *pBufferEvent, void *pData)
{
    struct evbuffer *pOutBuffer = bufferevent_get_output(pBufferEvent);
    if (evbuffer_get_length(pOutBuffer) == 0 && BEV_EVENT_EOF ) {
        bufferevent_free(pBufferEvent);
    }
}

void connectionRead_cb(struct bufferevent *pBufferEvent, void *pData)
{
    bufferevent_enable(pBufferEvent, EV_WRITE);
    bufferevent_disable(pBufferEvent, EV_READ);
    createResponse(pBufferEvent);
}

void connectionEvent_cb(struct bufferevent *pBufferEvent, short nEventFlags, void *pData)
{
    if (nEventFlags & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(pBufferEvent);
    }
}


void listnerError_cb(struct evconnlistener *pListener, void *pData)
{
    struct event_base *pBase = evconnlistener_get_base(pListener);
    event_base_loopexit(pBase, NULL);
}

