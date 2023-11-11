#ifndef __TCP_PROXY_H_
#define __TCP_PROXY_H_

#include "main.hpp"
#include "component.hpp"
#include "sock.hpp"

class TcpProxy
{
    public:
        TcpProxy();
        ~TcpProxy();

        void Run();
};

#endif 