#include <psp2/net/net.h>
#include <psp2/kernel/threadmgr.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int sceClibPrintf(const char *fmt, ...);


#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

typedef int socklen_t;

#define sockaddr_in SceNetSockaddrIn
#define msghdr SceNetMsghdr
#define iovec SceNetIovec
#define sockaddr SceNetSockaddr
#define in_addr SceNetInAddr

#define AF_INET SCE_NET_AF_INET
#define PF_INET SCE_NET_AF_INET
#define SOL_SOCKET SCE_NET_SOL_SOCKET
#define SO_SNDTIMEO SCE_NET_SO_SNDTIMEO 
#define IPPROTO_IP SCE_NET_IPPROTO_IP
#define IPPROTO_TCP SCE_NET_IPPROTO_TCP
#define TCP_NODELAY SCE_NET_TCP_NODELAY
#define SO_RCVBUF SCE_NET_SO_RCVBUF
#define SO_SNDBUF SCE_NET_SO_SNDBUF
#define SO_BROADCAST SCE_NET_SO_BROADCAST
#define SO_REUSEADDR SCE_NET_SO_REUSEADDR
#define SO_RCVTIMEO SCE_NET_SO_RCVTIMEO
#define SOCK_STREAM SCE_NET_SOCK_STREAM
#define SOCK_DGRAM SCE_NET_SOCK_DGRAM
#define INADDR_ANY SCE_NET_INADDR_ANY
#define SO_ERROR SCE_NET_SO_ERROR

#define recvmsg sceNetRecvmsg
#define sendmsg sceNetSendmsg
#define connect sceNetConnect
#define accept sceNetAccept
#define shutdown sceNetShutdown
#define setsockopt sceNetSetsockopt
#define getsockopt sceNetGetsockopt
#define listen sceNetListen
#define bind sceNetBind
#define getsockname sceNetGetsockname
#define socket sceNetSocket

#define SOMAXCONN 128
#define MSG_NOSIGNAL 0

static enet_uint32 timeBase = 0;

int
enet_initialize (void)
{
    return 0;
}

void
enet_deinitialize (void)
{
}

enet_uint32
enet_host_random_seed (void)
{
    return (enet_uint32) time (NULL);
}

enet_uint32
enet_time_get (void)
{
    struct timeval timeVal;

    gettimeofday (& timeVal, NULL);

    return timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000 - timeBase;
}

void
enet_time_set (enet_uint32 newTimeBase)
{
    struct timeval timeVal;

    gettimeofday (& timeVal, NULL);
    
    timeBase = timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000 - newTimeBase;
}

struct hostent{
  char  *h_name;         /* official (cannonical) name of host               */
  char **h_aliases;      /* pointer to array of pointers of alias names      */
  int    h_addrtype;     /* host address type: AF_INET                       */
  int    h_length;       /* length of address: 4                             */
  char **h_addr_list;    /* pointer to array of pointers with IPv4 addresses */
};
#define h_addr h_addr_list[0]

#define MAX_NAME 512
static struct hostent *gethostbyname(const char *name)
{
    static struct hostent ent;
    static char sname[MAX_NAME] = "";
    static struct SceNetInAddr saddr = { 0 };
    static char *addrlist[2] = { (char *) &saddr, NULL };

    int rid;
    int err;
    rid = sceNetResolverCreate("resolver", NULL, 0);
    if(rid < 0) {
        return NULL;
    }

    err = sceNetResolverStartNtoa(rid, name, &saddr, 0, 0, 0);
    sceNetResolverDestroy(rid);
    if(err < 0) {
        return NULL;
    }

    ent.h_name = sname;
    ent.h_aliases = 0;
    ent.h_addrtype = SCE_NET_AF_INET;
    ent.h_length = sizeof(struct SceNetInAddr);
    ent.h_addr_list = addrlist;
    ent.h_addr = addrlist[0];

    return &ent;
}

int
enet_address_set_host (ENetAddress * address, const char * name)
{
    struct hostent * hostEntry = NULL;
    hostEntry = gethostbyname (name);

    if (hostEntry != NULL && hostEntry -> h_addrtype == AF_INET)
    {
        address -> host = * (enet_uint32 *) hostEntry -> h_addr_list [0];

        return 0;
    }
    if (sceNetInetPton (AF_INET, name, & address -> host) < 0) {
        return -1;
    }

    return 0;
}

int
enet_address_get_host_ip (const ENetAddress * address, char * name, size_t nameLength)
{
    sceClibPrintf("enet_address_get_host_ip: not implemented\n");
    return -1;
}

int
enet_address_get_host (const ENetAddress * address, char * name, size_t nameLength)
{
    sceClibPrintf("enet_address_get_host: not implemented\n");
    return 0;
}

int
enet_socket_bind (ENetSocket socket, const ENetAddress * address)
{
    struct sockaddr_in sin;

    memset (& sin, 0, sizeof (struct sockaddr_in));

    sin.sin_family = AF_INET;

    if (address != NULL)
    {
       sin.sin_port = ENET_HOST_TO_NET_16 (address -> port);
       sin.sin_addr.s_addr = address -> host;
    }
    else
    {
       sin.sin_port = 0;
       sin.sin_addr.s_addr = INADDR_ANY;
    }

    return bind (socket,
                 (struct sockaddr *) & sin,
                 sizeof (struct sockaddr_in)); 
}

int
enet_socket_get_address (ENetSocket socket, ENetAddress * address)
{
    struct sockaddr_in sin;
    socklen_t sinLength = sizeof (struct sockaddr_in);

    if (getsockname (socket, (struct sockaddr *) & sin, & sinLength) == -1)
      return -1;

    address -> host = (enet_uint32) sin.sin_addr.s_addr;
    address -> port = ENET_NET_TO_HOST_16 (sin.sin_port);

    return 0;
}

int 
enet_socket_listen (ENetSocket socket, int backlog)
{
    return listen (socket, backlog < 0 ? SOMAXCONN : backlog);
}

ENetSocket
enet_socket_create (ENetSocketType type)
{
    return socket ("", PF_INET, type == ENET_SOCKET_TYPE_DATAGRAM ? SOCK_DGRAM : SOCK_STREAM, 0);
}

int
enet_socket_set_option (ENetSocket socket, ENetSocketOption option, int value)
{
    int result = -1;
    int _true = 1;
    switch (option)
    {
        case ENET_SOCKOPT_NONBLOCK:
            result = sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, (char *)&_true, sizeof(_true));
            if (result < 0)
                result = -1;
            break;

        case ENET_SOCKOPT_BROADCAST:
            result = setsockopt (socket, SOL_SOCKET, SO_BROADCAST, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_REUSEADDR:
            result = setsockopt (socket, SOL_SOCKET, SO_REUSEADDR, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_RCVBUF:
            result = setsockopt (socket, SOL_SOCKET, SO_RCVBUF, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_SNDBUF:
            result = setsockopt (socket, SOL_SOCKET, SO_SNDBUF, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_RCVTIMEO:
        {
            struct timeval timeVal;
            timeVal.tv_sec = value / 1000;
            timeVal.tv_usec = (value % 1000) * 1000;
            result = setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *) & timeVal, sizeof (struct timeval));
            break;
        }

        case ENET_SOCKOPT_SNDTIMEO:
        {
            struct timeval timeVal;
            timeVal.tv_sec = value / 1000;
            timeVal.tv_usec = (value % 1000) * 1000;
            result = setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *) & timeVal, sizeof (struct timeval));
            break;
        }

        case ENET_SOCKOPT_NODELAY:
            result = setsockopt (socket, IPPROTO_TCP, TCP_NODELAY, (char *) & value, sizeof (int));
            break;

        default:
            break;
    }
    return result == -1 ? -1 : 0;
}

int
enet_socket_get_option (ENetSocket socket, ENetSocketOption option, int * value)
{
    int result = -1;
    socklen_t len;
    switch (option)
    {
        case ENET_SOCKOPT_ERROR:
            len = sizeof (int);
            result = getsockopt (socket, SOL_SOCKET, SO_ERROR, value, & len);
            break;

        default:
            break;
    }
    return result == -1 ? -1 : 0;
}

int
enet_socket_connect (ENetSocket socket, const ENetAddress * address)
{
    struct sockaddr_in sin;
    int result;

    memset (& sin, 0, sizeof (struct sockaddr_in));

    sin.sin_family = AF_INET;
    sin.sin_port = ENET_HOST_TO_NET_16 (address -> port);
    sin.sin_addr.s_addr = address -> host;

    result = connect (socket, (struct sockaddr *) & sin, sizeof (struct sockaddr_in));
    if (result < 0) {
        if (result == SCE_NET_ERROR_EINPROGRESS)
            return 0;
        result = -1;
    }

    return result;
}

ENetSocket
enet_socket_accept (ENetSocket socket, ENetAddress * address)
{
    int result;
    struct sockaddr_in sin;
    socklen_t sinLength = sizeof (struct sockaddr_in);

    result = accept (socket, 
                     address != NULL ? (struct sockaddr *) & sin : NULL, 
                     address != NULL ? & sinLength : NULL);
    
    if (result == -1)
      return ENET_SOCKET_NULL;

    if (address != NULL)
    {
        address -> host = (enet_uint32) sin.sin_addr.s_addr;
        address -> port = ENET_NET_TO_HOST_16 (sin.sin_port);
    }

    return result;
} 
    
int
enet_socket_shutdown (ENetSocket socket, ENetSocketShutdown how)
{
    return shutdown (socket, (int) how);
}

void
enet_socket_destroy (ENetSocket socket)
{
    if (socket != -1)
      close (socket);
}

int
enet_socket_send (ENetSocket socket,
                  const ENetAddress * address,
                  const ENetBuffer * buffers,
                  size_t bufferCount)
{
    struct msghdr msgHdr;
    struct sockaddr_in sin;
    int sentLength;

    memset (& msgHdr, 0, sizeof (struct msghdr));

    if (address != NULL)
    {
        memset (& sin, 0, sizeof (struct sockaddr_in));

        sin.sin_family = AF_INET;
        sin.sin_port = ENET_HOST_TO_NET_16 (address -> port);
        sin.sin_addr.s_addr = address -> host;

        msgHdr.msg_name = & sin;
        msgHdr.msg_namelen = sizeof (struct sockaddr_in);
    }

    msgHdr.msg_iov = (struct iovec *) buffers;
    msgHdr.msg_iovlen = bufferCount;

    sentLength = sendmsg (socket, & msgHdr, MSG_NOSIGNAL);
    
    if (sentLength < 0) {
        if (sentLength == SCE_NET_ERROR_EWOULDBLOCK)
            return 0;

        return -1;
    }

    return sentLength;
}

int
enet_socket_receive (ENetSocket socket,
                     ENetAddress * address,
                     ENetBuffer * buffers,
                     size_t bufferCount)
{
    struct msghdr msgHdr;
    struct sockaddr_in sin;
    int recvLength;

    memset (& msgHdr, 0, sizeof (struct msghdr));

    if (address != NULL)
    {
        msgHdr.msg_name = & sin;
        msgHdr.msg_namelen = sizeof (struct sockaddr_in);
    }

    msgHdr.msg_iov = (struct iovec *) buffers;
    msgHdr.msg_iovlen = bufferCount;

    recvLength = recvmsg (socket, & msgHdr, MSG_NOSIGNAL);

    if (recvLength < 0) {
        if (recvLength == SCE_NET_ERROR_EWOULDBLOCK)
            return 0;

        return -1;
    }

#ifdef HAS_MSGHDR_FLAGS
    if (msgHdr.msg_flags & MSG_TRUNC)
      return -1;
#endif

    if (address != NULL)
    {
        address -> host = (enet_uint32) sin.sin_addr.s_addr;
        address -> port = ENET_NET_TO_HOST_16 (sin.sin_port);
    }

    return recvLength;
}

#define MAX_EVENTS 256
static int vita_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    if (nfds == 0) {
        sceKernelDelayThread(timeout->tv_sec * 1000000 + timeout->tv_usec);
        return 0;
    } else {
        int i;
        int epoll = sceNetEpollCreate("", 0);
        int ret = 0;
        for (i = 0; i < nfds; ++i) {
            SceNetEpollEvent ev = {0};
            if (readfds && FD_ISSET(i, readfds))
                ev.events |= SCE_NET_EPOLLIN;
            else if (writefds && FD_ISSET(i, writefds))
                ev.events |= SCE_NET_EPOLLOUT;
            else if (exceptfds && FD_ISSET(i, exceptfds))
                ev.events |= SCE_NET_EPOLLERR;
            if (ev.events) {
                ev.data.fd = i;
                ret = sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_ADD, i, &ev);
            }
        }
        SceNetEpollEvent events[MAX_EVENTS] = {0};
        ret = sceNetEpollWait(epoll, events, MAX_EVENTS, timeout->tv_sec * 1000000 + timeout->tv_usec);
        sceNetEpollDestroy(epoll);

        if (readfds)
            FD_ZERO(readfds);
        if (writefds)
            FD_ZERO(writefds);
        if (exceptfds)
            FD_ZERO(exceptfds);
        int cnt = 0;
        for (i = 0; i < ret; ++i) {
            if (events[i].events & SCE_NET_EPOLLIN && readfds) {
                FD_SET(events[i].data.fd, readfds);
                ++cnt;
            }
            if (events[i].events & SCE_NET_EPOLLOUT && writefds) {
                FD_SET(events[i].data.fd, writefds);
                ++cnt;
            }
            if (events[i].events & SCE_NET_EPOLLERR && exceptfds) {
                FD_SET(events[i].data.fd, exceptfds);
                ++cnt;
            }
        }
        return cnt;
    }
}

int
enet_socket_wait (ENetSocket socket, enet_uint32 * condition, enet_uint32 timeout)
{
    fd_set readSet, writeSet;
    struct timeval timeVal;
    int selectCount;

    timeVal.tv_sec = timeout / 1000;
    timeVal.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO (& readSet);
    FD_ZERO (& writeSet);

    if (* condition & ENET_SOCKET_WAIT_SEND)
      FD_SET (socket, & writeSet);

    if (* condition & ENET_SOCKET_WAIT_RECEIVE)
      FD_SET (socket, & readSet);

    selectCount = vita_select (socket + 1, & readSet, & writeSet, NULL, & timeVal);

    if (selectCount < 0)
    {      
        return -1;
    }

    * condition = ENET_SOCKET_WAIT_NONE;

    if (selectCount == 0)
      return 0;

    if (FD_ISSET (socket, & writeSet))
      * condition |= ENET_SOCKET_WAIT_SEND;

    if (FD_ISSET (socket, & readSet))
      * condition |= ENET_SOCKET_WAIT_RECEIVE;

    return 0;
}
