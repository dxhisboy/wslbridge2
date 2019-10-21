/* 
 * This file is part of wslbridge2 project
 * Licensed under the GNU General Public License version 3
 * Copyright (C) 2019 Biswapriyo Nath
 */

#ifndef LOCALSOCK_HPP
#define LOCALSOCK_HPP
#include "WinsockModule.hpp"
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
