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
#include "SConfig.h"


void listenerNewConnection_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
void listnerError_cb(struct evconnlistener *, void *);

void connectionWrite_cb(struct bufferevent *, void *);
void connectionRead_cb(struct bufferevent *, void *);
void connectionEvent_cb(struct bufferevent *, short, void *);

#define MIN_PORT_NUMBER_WITHOUT_ROOT_PERMISSIONS 1024

SConfig getConfigFromArgs(int argc, char *argv[])
{
    SConfig Config;

    /// select processes amount based on hardware
    Config.nProcesses = std::thread::hardware_concurrency() - 1;//2;
    std::cout << "INFO: selected processes amount " << Config.nProcesses << std::endl;

    if(argc > 1)
    {
        /// get specified port from args
        long int nDesiredPort = strtol(argv[1], NULL, 0);
        if(nDesiredPort > 0 && nDesiredPort <= USHRT_MAX)
        {
            Config.nPort = nDesiredPort;
            std::cout << "INFO: specified port " << Config.nPort << std::endl;
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

    if(Config.nPort < MIN_PORT_NUMBER_WITHOUT_ROOT_PERMISSIONS)
    {
        std::cout << "WARNING: ports less then " << MIN_PORT_NUMBER_WITHOUT_ROOT_PERMISSIONS << " require root permissions" << std::endl;
    }
    return Config;
}

void forkServer(unsigned short nPocesses)
{
    /// fork
    pid_t Pid = 0;
    std::cout << "INFO: parent process id: " << getpid() << std::endl;
    for(int i = 1; i <= nPocesses; ++i) {
        Pid = fork();
        if(Pid == 0) // child process
        {
            std::cout << "INFO: child process id: " << getpid() << " started" << std::endl;
            break;
        }
        else if(Pid == -1) // fork error
        {
            std::cerr << "ERROR: forking failed" << std::endl;
            throw std::exception();
        }
    }
}

struct event_base* initBase()
{
    struct event_base* pBase = event_base_new();
    if (pBase == 0)
    {
        std::cerr << "ERROR: init libevent base failed" << std::endl;
        throw std::exception();
    }
    return pBase;
}

struct sockaddr_in initAddress(unsigned short nPort)
{
    struct sockaddr_in Address;
    memset(&Address, 0, sizeof(Address));

    Address.sin_port = htons(nPort);
    Address.sin_family = AF_INET;

    return Address;
}

struct evconnlistener* initListner(struct event_base* pBase, struct sockaddr_in Address)
{
    struct evconnlistener* pListener = pListener = evconnlistener_new_bind(
                pBase,
                listenerNewConnection_cb,
                NULL,
                LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
                -1,
                (struct sockaddr*)&Address,
                sizeof(Address));
    if (pListener == 0)
    {
        std::cerr << "ERROR: aquiring port failed" << std::endl;
        throw std::exception();
    }

    /// bind error callback
    evconnlistener_set_error_cb(pListener, listnerError_cb);

    return pListener;
}

short startForkServer(SConfig Config)
{
    struct event_base* pBase = initBase();
    struct sockaddr_in Address = initAddress(Config.nPort);
    struct evconnlistener* pListener = initListner(pBase, Address);

    forkServer(Config.nProcesses);

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

int main(int argc, char* argv[])
{
    SConfig Config = getConfigFromArgs(argc, argv);
    try
    {
        startForkServer(Config);
    }
    catch(...)
    {
        return 1;
    }

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

