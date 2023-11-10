#include "proxy.hpp"
#include "balancer.hpp"
#include "tcp_proxy.hpp"
#include "udp_proxy.hpp"

Proxy::Proxy()
{
    bm = new BindManager();

    if(el.CreateEventLoop() == C_ERR)
    {
        std::cerr<<"Failed CreateEventLoop for Balancer"<<std::endl;
        return;
    }
}

// bind목록 컴포넌트 목록등 확인하여 제거 필요
int Proxy::DeleteSocket(Net::Socket *socket)
{
    // bind 목록에서 먼저 제거
    if(socket->sock_type == SockType::BalancerProxyClient)
    {
        // call bm->DeleteBind();
        ErrorCode err; 
        Net::Socket *bind_socket = nullptr;
        std::tie(err, bind_socket) = bm->DeleteBind(socket->fd);
        if(err != ErrorCode::None)
        {
            std::cout<<"[DEBUG] isn't bind socket"<<std::endl;
        }
        else
        {
            if(bind_socket != nullptr)
            {
                el.DelEvent(bind_socket);
                std::cout<<"[DEBUG] remove bind socket"<<std::endl;
            }
            else
            {
                std::cout<<"[DEBUG] reduce bind reference"<<std::endl;
            }
        }
    }

    el.DelEvent(socket);
    return C_OK;
}

Message *Proxy::ParseMessage(Net::Socket *socket)
{
    if(socket->querybuf == nullptr)
    {
        return nullptr;
    }

    Message *message = new Message(socket->querybuf);
    if(message->Parse(socket->querylen) == C_ERR)
    {
        delete message;
        return nullptr;
    }

    return message;
}

void Proxy::Run(int port)
{
    if(BindTcpSocket(port, SockType::BalancerProxyServer) == C_ERR)
    {
        std::cerr<<"Failed Bind Balancer Tcp Socket"<<std::endl;
        return;
    }
    
    while(true)
    {
        int retval = el.FetchEvent();
        if(retval <= 0 )
        {
            continue;
        }

        ProcessEvent(retval);
    }
}

void Proxy::ProcessEvent(int retval)
{
    for(int i=0; i<retval; i++)
    {
        Epoll::ev_t e = el.fired[i];
        Net::Socket *socket = el.LoadSocket(e.fd);

        /* EPOLLERR */
        /**
         * NOTE! 
         * 연결이 끊어졌을 경우 컴포넌트 리스트에서 삭제
         * 현재 바인드하고 있는 포트 레퍼런스 제거
        */
        if(e.mask != EPOLLIN)
        {
            std::cout<<"[DEBUG] Client disconnected"<<std::endl;
            DeleteSocket(socket);
            continue;
        }

        /* EPOLLIN */
        /* 이벤트가 발생한 socket에서 어떤 이벤트가 발생했는지 확인 필요 */
        switch(socket->sock_type)
        {
            /* 컴포넌트 소켓 추가 */
            case SockType::BalancerProxyServer:
            {
                if(ProcessAccept((Net::TcpSocket*)socket, e.mask, SockType::BalancerProxyClient) == C_ERR)
                {
                    continue;
                }

                std::cout<<"[DEBUG] Accept Load Balance Component"<<std::endl;
                break;
            }

            /* 컴포넌트의 메시지 처리 */
            case SockType::BalancerProxyClient:
            {
                int err; 
                json res; 
                std::tie(err, res) = ProcessControlChannel(socket);
                if(err == C_ERR)
                {
                    DeleteSocket(socket);
                    continue; 
                }

                std::cout<<res<<std::endl;
                break;
            }

            /* 연결된 TCP 서버로 릴레이 */
            /**
             * 1. 서버로 접근하려는 클라이언트와 ACCEPT을 하고 해당 소켓을 TcpProxyClient로 설정한다. 
             * 2. 요청한 포트번호로 연결된 컴포넌트의 fd들 중 하나를 getpeername으로 ip주소와 포트를 알아냄
             * 3. 거기로 connect해서 데이터 전송 및 수신
             * 4. 수신 된 데이터 Client에게 전송
             * 
             * 5. 어떻게 포트랑 IP알것인가
            */
            case SockType::TcpProxyServer:
            {
                std::cout<<"HELLO TCP"<<std::endl;
                int host_port = ntohs(socket->sock_addr->adr.sin_port);
                //std::cout<<"Server Port: "<<socket->sock_addr->adr.sin_port<<std::endl;
                BindComponent *bc = bm->LoadBindComponent("tcp", host_port);
                
                std::vector<Component*> *comps = bc->GetComps();
                for(auto i: *comps)
                {
                    std::cout<<ntohs(i->addr.sin_port)<<std::endl;
                }
                //TcpProxy tproxy = TcpProxy();
                break;
            }

            /* 컴포넌트에 접근하기를 원하는 외부 요청 */
            case SockType::TcpProxyClient:
            {
                TcpProxy tproxy = TcpProxy();
                break;
            }

            /* 연결된 UDP 서버로 릴레이 */
            case SockType::UdpProxyServer:
            {
                std::cout<<"Hello UDP"<<std::endl;
                UdpProxy uproxy = UdpProxy();
                break;
            }

            /* 컴포넌트에 접근하기를 원하는 외부 요청 */
            case SockType::UdpProxyClient:
            {
                UdpProxy uproxy = UdpProxy();
                break;
            }

            default:
                break;
        }
    }

    delete el.fired;
}

int Proxy::ProcessAccept(Net::TcpSocket *socket, int mask, int sock_type)
{
    Net::TcpSocket *c_socket = socket->AcceptSocket(sock_type);
    if(c_socket != nullptr)
    {
        if(el.AddEvent(c_socket) == C_ERR)
        {
            return C_ERR;
        }
    }

    return C_ERR;
}

/**
 * Create Load Balancer Socket
*/
int Proxy::BindTcpSocket(int port, int sock_type)
{
    Net::SockAddr *addr = new Net::SockAddr(port);
    Net::TcpSocket *socket = new Net::TcpSocket(addr, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);

    if(socket->CreateSocket(sock_type, SOCK_STREAM) == C_ERR)
    {
        delete socket;
        return C_ERR;
    } 

    if(socket->BindSocket() == C_ERR)
    {
        delete socket;
        return C_ERR;
    }

    if(socket->ListenSocket() == C_ERR)
    {
        delete socket;
        return C_ERR;
    }

    /* Add Load Balancer Socket in epoll */
    if(el.AddEvent(socket) == C_ERR)
    {
        delete socket;
        return C_ERR;
    }

    return C_OK;
}

int Proxy::BindUdpSocket(int port, int sock_type)
{
    Net::SockAddr *addr = new Net::SockAddr(port);
    Net::UdpSocket *socket = new Net::UdpSocket(addr, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);

    if(socket->CreateSocket(sock_type, SOCK_DGRAM) == C_ERR)
    {
        delete socket;
        return C_ERR;
    } 

    if(socket->BindSocket() == C_ERR)
    {
        delete socket;
        return C_ERR;
    }

    return C_OK;
}

std::tuple<int, json> Proxy::ProcessControlChannel(Net::Socket *socket)
{
    if(socket->ReadMsgPackFromSocket() == C_ERR)
    {
        std::cout<<"[DEBUG] Client read Error"<<std::endl;
        return std::make_tuple(C_ERR, json());
    }

    Message *message = ParseMessage(socket);
    if(message == nullptr)
    {
        std::cout<<"[DEBUG] Client Parse Error"<<std::endl;
        return std::make_tuple(C_ERR, json()); 
    }

    std::cout<<message->msg<<std::endl;
    BalancerProxy bproxy = BalancerProxy(&el, bm, socket); 
    json res = bproxy.Controller(message->msg);

    delete message;
    return std::make_tuple(C_OK, res);
}

Proxy::~Proxy()
{
    delete bm;
}