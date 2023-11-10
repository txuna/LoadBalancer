#ifndef __COMPONENT_H_
#define __COMPONENT_H_

#include <vector>
#include <string>
#include <iostream>

#include "sock.hpp"
#include "common.hpp"

#include "epoll_event.hpp"

/**
 * NOTE! 
 * 전체 컴포넌트 리스트와 컴포넌트별로 사용중인 binding port들에 대한 관리 필요
 * binding port는 참조하는 레퍼런스의 갯수 체크 필요 0이 될 시 해당 포트 close
*/

class BindComponent
{
    private:
        /* Control Channel은 TCP 연결이기 때문에 연결유지되어 있다면 언제든지 getpeername으로 주소정보 가지고 올 수 있음*/
        std::vector<socket_t> fds;   
        int port;
        std::string protocol;
        Net::Socket *bind_socket = nullptr;

    public:
        BindComponent();
        ~BindComponent();

        Net::TcpSocket *BindTcpSocket(int port, socket_t fd);
        Net::UdpSocket *BindUdpSocket(int port, socket_t fd);
        bool HasFd(socket_t fd);
        bool Compare(std::string protocol, int port);
        void AppendFD(socket_t fd);
        void DeleteFD(socket_t fd);
        int LenFDS();
        Net::Socket *GetSocket();
};

class BindManager
{
    private:
        std::vector<BindComponent*> binds;

    public:
        BindManager();
        ~BindManager();

        std::tuple<ErrorCode, Net::Socket*> AddBind(std::string protocol, int port, socket_t fd);
        std::tuple<ErrorCode, Net::Socket*> DeleteBind(socket_t fd); 
        BindComponent *LoadBindComponent(std::string protocol, int port);
};

#endif 