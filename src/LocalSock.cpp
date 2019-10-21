/* 
 * This file is part of wslbridge2 project
 * Licensed under the GNU General Public License version 3
 * Copyright (C) 2019 Biswapriyo Nath
 */

#include <assert.h>
#include <winsock.h>
#include <stdio.h>
#include "LocalSock.hpp"

#include "../hvsocket/hvsocket.h"

#define DYNAMIC_PORT_LOW 49152
#define DYNAMIC_PORT_HIGH 65535
#define BIND_MAX_RETRIES 10

#define RANDOMPORT() \
rand() % (DYNAMIC_PORT_HIGH - DYNAMIC_PORT_LOW) + DYNAMIC_PORT_LOW

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

typedef int socklen_t;
LocalSock::LocalSock(GUID *VmId)
{
    ws = WinsockModule::getInstance();

    isHv = VmId ? 1 : 0;

    if (isHv)
    {
	mSockFd = ws->socket(AF_HYPERV, SOCK_STREAM, HV_PROTOCOL_RAW);
    } else
    {
	mSockFd = ws->socket(AF_INET, SOCK_STREAM, 0);
    }
    assert(mSockFd >= 0);

    if (isHv)
    {
	const int suspend = true;
	const int suspendRet = ws->setsockopt(mSockFd, HV_PROTOCOL_RAW, HVSOCKET_CONNECTED_SUSPEND, &suspend, sizeof(suspend));
	struct SOCKADDR_HV addr = {};
	addr.Family = AF_HYPERV;
	memcpy(&addr.VmId, VmId, sizeof addr.VmId);
	memcpy(&addr.ServiceId, &HV_GUID_VSOCK_TEMPLATE, sizeof addr.ServiceId);

	/* Try to bind to a dynamic port */
	int nretries = 0;
	int port;

	while (nretries < BIND_MAX_RETRIES)
	{
	    port = RANDOMPORT();
	    addr.ServiceId.Data1 = port;
	    const int bindRet = ws->bind(mSockFd, &addr, sizeof addr);
	    if (bindRet == 0)
		break;
	    
	    nretries++;
	}

	const int listenRet = ws->listen(mSockFd, 1);
	assert(listenRet == 0);

	/* Return port number to caller */
	mPort = port;	
    } else
    {
	const int flag = 1;
	const int nodelayRet = ws->setsockopt(mSockFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof flag);
	assert(nodelayRet == 0);

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	const int bindRet = ws->bind(mSockFd, &addr, sizeof addr);
	assert(bindRet == 0);

	const int listenRet = ws->listen(mSockFd, 1);
	assert(listenRet == 0);

	socklen_t addrLen = sizeof(addr);
	const int getRet = ws->getsockname(mSockFd, reinterpret_cast<sockaddr*>(&addr), &addrLen);
	assert(getRet == 0);

	mPort = ntohs(addr.sin_port);

    }
    printf("%ld %d\n", mSockFd, mPort);
}

SOCKET LocalSock::accept()
{
    const SOCKET cs = ws->accept(mSockFd, nullptr, nullptr);
    assert(cs >= 0);

    if (!isHv)
    {
	const int flag = 1;
	const int nodelayRet = ws->setsockopt(cs, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof flag);
	assert(nodelayRet == 0);
    }
    return cs;
}

void LocalSock::close()
{
    if (mSockFd != -1)
    {
        ws->closesocket(mSockFd);
        mSockFd = -1;
    }
}
