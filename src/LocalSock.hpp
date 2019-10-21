/* 
 * This file is part of wslbridge2 project
 * Licensed under the GNU General Public License version 3
 * Copyright (C) 2019 Biswapriyo Nath
 */

#ifndef LOCALSOCK_HPP
#define LOCALSOCK_HPP
#include <windows.h>
#ifndef SOCKET
#define SOCKET size_t
#endif
/* Windows socket functions pointers from ws2_32.dll file */
typedef SOCKET (WINAPI *ACCEPTPROC)(SOCKET, void*, int*);
typedef int (WINAPI *BINDPROC)(SOCKET, const void*, int);
typedef int (WINAPI *CLOSESOCKETPROC)(SOCKET);
typedef int (WINAPI *CONNETCPROC)(SOCKET, const void*, int);
typedef int (WINAPI *LISTENPROC)(SOCKET, int);
typedef int (WINAPI *RECVPROC)(SOCKET, void*, int, int);
typedef int (WINAPI *SENDPROC)(SOCKET, const void*, int, int);
typedef SOCKET (WINAPI *SOCKETPROC)(int, int, int);
typedef int (WINAPI *SETSOCKOPTPROC)(SOCKET, int, int, const void*, int);
typedef int (WINAPI *GETSOCKNAMEPROC)(SOCKET, void *, int *);
typedef int (WINAPI *WSASTARTUPPROC)(WORD, void*);
typedef int (WINAPI *WSACLEANUPPROC)(void);

class WinsockModule {
public:
    static WinsockModule *getInstance();
    SOCKET accept(SOCKET socket, void *addr, int *addrlen);
    int bind(SOCKET s, void *name, int namelen);
    int closesocket(SOCKET s);
    int connect(SOCKET s, void *name, int namelen);
    int listen(SOCKET s, int backlog);
    int recv(SOCKET s, void *buf, int len, int flags);
    int send(SOCKET s, const void *buf, int len, int flags);
    SOCKET socket(int af, int type, int protocol);
    int setsockopt(SOCKET s, int level, int optname, const void *optval, int optlen);
    int getsockname(SOCKET s, void *addr, int *namelen);
private:
    static WinsockModule *p_Instance;
    WinsockModule();
    ~WinsockModule();
    HMODULE m_hModule;
    ACCEPTPROC pfnAccept;
    BINDPROC pfnBind;
    CLOSESOCKETPROC pfnCloseSocket;
    CONNETCPROC pfnConnect;
    LISTENPROC pfnListen;
    RECVPROC pfnRecv;
    SENDPROC pfnSend;
    SOCKETPROC pfnSocket;
    SETSOCKOPTPROC pfnSetSockOpt;
    GETSOCKNAMEPROC pfnGetSockName;
    WSASTARTUPPROC pfnWSAStartup;
    WSACLEANUPPROC pfnWSACleanup;
};
class LocalSock
{
public:
    LocalSock(GUID *VmId = nullptr);
    ~LocalSock() { close(); }
    int port() { return mPort; }
    SOCKET accept();
    void close();

private:
    SOCKET mSockFd;
    int mPort;
    int isHv;
    WinsockModule *ws;
};

#endif /* LOCALSOCK_HPP */
