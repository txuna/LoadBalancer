#include "component.hpp"
#include <errno.h>

BindComponent::BindComponent()
{

}

BindComponent::~BindComponent()
{

}

void BindComponent::AppendFD(socket_t fd)
{
    fds.push_back(fd);
    return;
}

bool BindComponent::Compare(std::string protocol, int port)
{
    if(this->port == port && this->protocol == protocol)
    {
        return true;
    }

    return false;
}

bool BindComponent::HasFd(socket_t fd)
{
    for(socket_t rfd: fds)
    {
        if(fd == rfd)
        {
            return true;
        }
    }
    return false;
}


Net::TcpSocket *BindComponent::BindTcpSocket(int port, socket_t fd)
{
    Net::SockAddr *addr = new Net::SockAddr(port);
    Net::TcpSocket *socket = new Net::TcpSocket(addr, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);

    this->port = port; 
    this->protocol = "tcp";

    if(socket->CreateSocket(SockType::TcpProxyServer, SOCK_STREAM) == C_ERR)
    {
        delete socket;
        return nullptr;
    } 

    if(socket->BindSocket() == C_ERR)
    {
        delete socket;
        return nullptr;
    }

    if(socket->ListenSocket() == C_ERR)
    {
        delete socket;
        return nullptr;
    }

    AppendFD(fd);
    bind_socket = socket;

    return (Net::TcpSocket*)bind_socket;
}

Net::UdpSocket *BindComponent::BindUdpSocket(int port, socket_t fd)
{
    Net::SockAddr *addr = new Net::SockAddr(port);
    Net::UdpSocket *socket = new Net::UdpSocket(addr, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);

    this->port = port; 
    this->protocol = "udp";

    if(socket->CreateSocket(SockType::UdpProxyServer, SOCK_DGRAM) == C_ERR)
    {
        delete socket;
        return nullptr;
    } 

    if(socket->BindSocket() == C_ERR)
    {
        delete socket;
        return nullptr;
    }

    AppendFD(fd);
    bind_socket = socket;

    return (Net::UdpSocket*)bind_socket;
}

void BindComponent::DeleteFD(socket_t fd)
{
    auto it = std::find_if(fds.begin(), fds.end(), 
                        [fd](socket_t rfd){
                            return rfd == fd;
                        });

    if(it != fds.end())
    {
        fds.erase(it);
    }
}

int BindComponent::LenFDS()
{
    return fds.size();
}

Net::Socket *BindComponent::GetSocket()
{
    return bind_socket;
}

BindManager::BindManager()
{

}

BindManager::~BindManager()
{
    /* 모든 bind 요소 delete */
    for(BindComponent *bc: binds)
    {
        // need to unbind 
        delete bc; 
    }

    binds.clear();
}

std::tuple<ErrorCode, Net::Socket*> BindManager::AddBind(std::string protocol, int port, socket_t fd)
{
    // 이미 동일한 fd로 바인드되고 있는지 확인
    for(BindComponent *bc: binds)
    {
        if(bc->HasFd(fd) == true)
        {
            return std::make_tuple(ErrorCode::AlreadyRegistered, nullptr);
        }
    }

    BindComponent *bc = LoadBindComponent(protocol, port);
    // 이미 해당 포트와 프로토콜로 바인딩 되고 있다면 
    if(bc != nullptr)
    {
        bc->AppendFD(fd);
        return std::make_tuple(ErrorCode::None, nullptr);
    }

    BindComponent *new_bc = new BindComponent();
    Net::Socket *bind_socket = nullptr;

    if(protocol == "tcp")
    {
        bind_socket = new_bc->BindTcpSocket(port, fd);
    }

    else if(protocol == "udp")
    {
        bind_socket = new_bc->BindUdpSocket(port, fd);
    }

    /* socket bind실패시 바인드 목록 삭제 */
    if(bind_socket == nullptr)
    {
        delete new_bc;
        return std::make_tuple(ErrorCode::BindError, nullptr);
    }

    /* 바인드 목록에 추가 */
    binds.push_back(new_bc);

    return std::make_tuple(ErrorCode::None, bind_socket);
}

/* 인자로 주어진 fd를 지우고 bindcomponent의 fds의 길이가 0이라면 바인딩 제거 */
std::tuple<ErrorCode, Net::Socket *> BindManager::DeleteBind(socket_t fd)
{
    /* fds의 사이즈가 0이 된다면 해당 바인드 삭제 */
    for(BindComponent *bc: binds)
    {
        if(bc->HasFd(fd) == true)
        {
            bc->DeleteFD(fd);
            if(bc->LenFDS() == 0)
            {
                auto it = std::find(binds.begin(), binds.end(), bc); 
                if(it != binds.end())
                {
                    binds.erase(it);
                    Net::Socket *rs = bc->GetSocket();
                    delete bc;
                    return std::make_tuple(ErrorCode::None, rs);
                }
            }

            return std::make_tuple(ErrorCode::None, nullptr);
        }
    }

    return std::make_tuple(ErrorCode::AlreadyUnRegistered, nullptr);
}

BindComponent *BindManager::LoadBindComponent(std::string protocol, int port)
{
    for(BindComponent *bc: binds)
    {
        if(bc->Compare(protocol, port) == true)
        {
            return bc; 
        }
    }

    return nullptr;
}