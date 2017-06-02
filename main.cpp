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

#define DEFAULT_PORT 80

void listenerNewConnection_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
void listnerError_cb(struct evconnlistener *, void *);

void connectionWrite_cb(struct bufferevent *, void *);
void connectionRead_cb(struct bufferevent *, void *);
void connectionEvent_cb(struct bufferevent *, short, void *);


int main(int argc, char *argv[])
{
    struct event_base* pBase = 0;
    struct evconnlistener* pListener = 0;
    struct sockaddr_in Address;
    int nProcessessAmount = std::thread::hardware_concurrency() - 1;//2;

    /// init libevent base
    pBase = event_base_new();
    if (pBase == 0)
    {
        std::cerr << "ERROR: init libevent" << std::endl;
        return 1;
    }

    /// get specified port
    unsigned short nPort = DEFAULT_PORT;
    if(argc > 1)
    {
        long int nDesiredPort = strtol(argv[1], NULL, 0);
        if(nDesiredPort > 0 && nDesiredPort <= USHRT_MAX)
        {
            nPort = nDesiredPort;
            std::cout << "INFO: specified port " << nPort << std::endl;
        }
        else
        {
            std::cerr << "WARNING: port must be in 1 - " << USHRT_MAX << " range, 80 will be used instead" << std::endl;
        }

    }
    else
    {
        std::cout << "INFO: no port specified, 80 will be used" << std::endl;
    }

    /// init address
    memset(&Address, 0, sizeof(Address));
    Address.sin_port = htons(nPort);
    Address.sin_family = AF_INET;

    /// bind port
    pListener = evconnlistener_new_bind(pBase, listenerNewConnection_cb, NULL,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&Address, sizeof(Address));
    if (pListener == 0)
    {
        std::cerr << "ERROR: creating a listener, Check if port " << nPort << " is not used by another process" << std::endl;
        return 1;
    }

    /// bind error callback
    evconnlistener_set_error_cb(pListener, listnerError_cb);

    /// fork
    pid_t Pid;
    std::cout << "parent process " << getpid() << std::endl;
    for(int i = 1; i <= nProcessessAmount; ++i) {
        Pid = fork();
        if(Pid == 0)
        {
            std::cout << "fork " << getpid() << " started" << std::endl;
            break;
        }
        else if(Pid == -1)
        {
            std::cerr << "ERROR: forking\n" << std::endl;
            break;
        }

    }

    /// reinit is required after forking
    event_reinit(pBase);

    /// start loop
    event_base_dispatch(pBase);

    /// loop ended, free resourses
    evconnlistener_free(pListener);
    event_base_free(pBase);

    std::cout << "exit" << std::endl;
    return 0;
}



void listenerNewConnection_cb(struct evconnlistener *pListener, evutil_socket_t nFileDescriptor,
                 struct sockaddr *SourceAddress, int nAddressLength, void *pData)
{
    struct event_base* pBase = evconnlistener_get_base(pListener);
    struct bufferevent* pBufferEvent = 0;

    /// create new bufferevent for a new socket
    pBufferEvent = bufferevent_socket_new(pBase, nFileDescriptor, BEV_OPT_CLOSE_ON_FREE);
    if (!pBufferEvent) {
        std::cerr << "ERROR: creating buffer" << std::endl;
        event_base_loopbreak(pBase);
        return;
    }

    /// bind callbacks to a new bufferevent
    bufferevent_setcb(pBufferEvent, connectionRead_cb, connectionWrite_cb, connectionEvent_cb, NULL);
    bufferevent_enable(pBufferEvent, EV_READ);
    bufferevent_disable(pBufferEvent, EV_WRITE);
}


void connectionWrite_cb(struct bufferevent *pBufferEvent, void *pData)
{
    struct evbuffer *pOutBuffer = bufferevent_get_output(pBufferEvent);

    /// check if all data is sended, then close bufferevent
    if (evbuffer_get_length(pOutBuffer) == 0 && BEV_EVENT_EOF ) {
        bufferevent_free(pBufferEvent);
    }
}

void connectionRead_cb(struct bufferevent *pBufferEvent, void *pData)
{
    /// read and process request
    bufferevent_enable(pBufferEvent, EV_WRITE);
    bufferevent_disable(pBufferEvent, EV_READ);
    createResponse(pBufferEvent);
}

void connectionEvent_cb(struct bufferevent *pBufferEvent, short nEventFlags, void *pData)
{
    /// close buffervent on socket error
    if (nEventFlags & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(pBufferEvent);
    }
}


void listnerError_cb(struct evconnlistener *pListener, void *pData)
{
    /// stop server on socket error
    struct event_base *pBase = evconnlistener_get_base(pListener);
    event_base_loopexit(pBase, NULL);
}

