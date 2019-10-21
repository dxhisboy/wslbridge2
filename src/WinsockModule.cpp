#include <assert.h>
#include <winsock.h>
#include <stdio.h>
#include "WinsockModule.hpp"

WinsockModule* WinsockModule::p_Instance = nullptr;
WinsockModule::WinsockModule()
{
    m_hModule = LoadLibraryExW(
                L"ws2_32.dll",
                nullptr,
                LOAD_LIBRARY_SEARCH_SYSTEM32);
    assert(m_hModule != nullptr);

    pfnAccept = (ACCEPTPROC)GetProcAddress(m_hModule, "accept");
    pfnBind = (BINDPROC)GetProcAddress(m_hModule, "bind");
    pfnCloseSocket = (CLOSESOCKETPROC)GetProcAddress(m_hModule, "closesocket");
    pfnConnect = (CONNETCPROC)GetProcAddress(m_hModule, "connect");
    pfnListen = (LISTENPROC)GetProcAddress(m_hModule, "listen");
    pfnRecv = (RECVPROC)GetProcAddress(m_hModule, "recv");
    pfnSend = (SENDPROC)GetProcAddress(m_hModule, "send");
    pfnSocket = (SOCKETPROC)GetProcAddress(m_hModule, "socket");
    pfnSetSockOpt = (SETSOCKOPTPROC)GetProcAddress(m_hModule, "setsockopt");
    pfnGetSockName = (GETSOCKNAMEPROC)GetProcAddress(m_hModule, "getsockname");
    pfnWSAStartup = (WSASTARTUPPROC)GetProcAddress(m_hModule, "WSAStartup");
    pfnWSACleanup = (WSACLEANUPPROC)GetProcAddress(m_hModule, "WSACleanup");

    struct WSAData wdata;
    const int wsaRet = pfnWSAStartup(MAKEWORD(2,2), &wdata);
    assert(wsaRet == 0);
}

WinsockModule::~WinsockModule()
{
    pfnWSACleanup();
    if (m_hModule)
        FreeLibrary(m_hModule);
}

WinsockModule *WinsockModule::getInstance()
{
    if (!p_Instance)
	p_Instance = new WinsockModule();
    return p_Instance;
}

SOCKET WinsockModule::accept(SOCKET socket, void *addr, int *addrlen)
{
    return pfnAccept(socket, addr, addrlen);
}

int WinsockModule::bind(SOCKET s, void *name, int namelen)
{
    return pfnBind(s, name, namelen);
}

int WinsockModule::closesocket(SOCKET s)
{
    return pfnCloseSocket(s);
}

int WinsockModule::connect(SOCKET s, void *name, int namelen)
{
    return pfnConnect(s, name, namelen);
}

int WinsockModule::listen(SOCKET s, int backlog)
{
    return pfnListen(s, backlog);
}
int WinsockModule::recv(SOCKET s, void *buf, int len, int flags)
{
    return pfnRecv(s, buf, len, flags);
}

int WinsockModule::send(SOCKET s, const void *buf, int len, int flags)
{
    return pfnSend(s, buf, len, flags);
}

SOCKET WinsockModule::socket(int af, int type, int protocol)
{
    return pfnSocket(af, type, protocol);
}

int WinsockModule::setsockopt(SOCKET s, int level, int optname, const void *optval, int optlen)
{
    return pfnSetSockOpt(s, level, optname, optval, optlen);
}

int WinsockModule::getsockname(SOCKET s, void *name, int *namelen) {
    return pfnGetSockName(s, name, namelen);
}
