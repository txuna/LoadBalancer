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
        time_clock::time_point start = time_clock::now();
        int retval = el.FetchEvent();
        if(retval > 0 )
        {
            ProcessEvent(retval);
        }

        time_clock::time_point end = time_clock::now();
        std::chrono::milliseconds ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start); 

        tick += ms_duration.count();
        if(tick >= 5000)
        {
            SendHealthCheck();
            tick = 0;
        }
    }
}

void Proxy::SendHealthCheck()
{
    json req = {
        {"cmd", "healthcheck"},
    };

    std::vector<BindComponent*> *binds = bm->GetBinds();

    for(BindComponent *bc: *binds)
    {
        std::vector<Component*> *comps = bc->GetComps();
        for(Component *comp: *comps)
        {
            Net::Socket *socket = el.LoadSocket(comp->fd); 
            if(socket == nullptr)
            {
                continue;
            }

            //std::cout<<ntohs(socket->sock_addr->adr.sin_port)<<std::endl;
            if(socket->SendMsgPackToSocket(req) == C_ERR)
            {
                continue;
            }
        }
    }

    return;
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

                //std::cout<<res<<std::endl;
                /* res 전송 */
                if(socket->SendMsgPackToSocket(res) == C_ERR)
                {
                    DeleteSocket(socket);
                    continue;
                }
                
                break;
            }

            /* 연결된 TCP 서버로 릴레이 */
            case SockType::TcpProxyServer:
            {
                if(ProcessAccept((Net::TcpSocket*)socket, e.mask, SockType::TcpProxyClient) == C_ERR)
                {
                    DeleteSocket(socket);
                    continue;
                }

                break;
            }

            /* 컴포넌트에 접근하기를 원하는 외부 요청 */
            case SockType::TcpProxyClient:
            {
                if(ProcessTcpProxy(socket) == C_ERR)
                {
                    DeleteSocket(socket);
                    continue;
                }

                break;
            }

            /* 처음 요청한 클라이언트에게 전달 */
            /* 이때 socket은 API 서버에 연결한 socket */
            /* connection_pair_fd는 로드밸런서에 연결 시도한 사용자 */
            case SockType::TcpRelayClient:
            {
                int client_fd = ((Net::TcpSocket*)socket)->connection_pair_fd;
                Net::Socket *client_socket = el.LoadSocket(client_fd);
                if(client_socket == nullptr)
                {
                    std::cout<<"tcp relay client is null"<<std::endl;
                    continue;
                }

                if(socket->ReadSocket() == C_ERR)
                {
                    std::cout<<"tcp relay client failed read"<<std::endl;
                    continue;
                }

                if(client_socket->SendSocket(socket->querybuf, socket->querylen) == C_ERR)
                {
                    std::cout<<"tcp relay client failed send"<<std::endl;
                    continue;
                }

                std::cout<<"SEND!"<<std::endl;

                delete []socket->querybuf;
                break;
            }

            /* 연결된 UDP 서버로 릴레이 */
            /* UDP는 클라이언트없이 여기서 바로 하는 듯 */
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
            delete c_socket;
            return C_ERR;
        }
    }

    c_socket->connection_port = socket->connection_port = GetBindPortFromSocket(socket);

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

int Proxy::GetBindPortFromSocket(Net::Socket *socket)
{
    return ntohs(socket->sock_addr->adr.sin_port);
}

/**
 * 어떠한 바인딩된 포트와 왔는지 확인 필요
 * 그냥 편하게 스레드 사용?
*/
int Proxy::ProcessTcpProxy(Net::Socket *socket)
{   
    /* ReadSocket 후 버퍼 비우기 필수 */
    if(socket->ReadSocket() == C_ERR)
    {
        std::cout<<"In TcpProxyClient Failed Read Socket"<<std::endl;
        return C_ERR;
    }

    std::cout<<"Connection Port: "<<socket->connection_port<<std::endl;
    /* 클라이언트가 접속한 포트의 relay_port에 쏴줘야 함 새로운 커넥션 생성 */

    BindComponent *bc = bm->LoadBindComponent("tcp", socket->connection_port);
    Component *comp = bc->GetRoundRobinComponent();

    // connect 해야하는데 시간이 걸리지 않는감 
    // connect하고 생기는 소켓은 TcpRelayClient 
    Net::SockAddr *addr = new Net::SockAddr(comp->addr); /* 서버의 주소 정보 */
    Net::TcpSocket *relay_socket = new Net::TcpSocket(addr, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);
    if(relay_socket->CreateSocket(SockType::TcpRelayClient, SOCK_STREAM) == C_ERR)
    {
        delete relay_socket;
        delete []socket->querybuf;

        std::cout<<"1"<<std::endl;
        return C_ERR;
    }
    
    if(relay_socket->ConnectSocket() == C_ERR)
    {
        delete relay_socket;
        delete []socket->querybuf;
        std::cout<<"12"<<std::endl;
        return C_ERR;
    }

    if(el.AddEvent(relay_socket) == C_ERR)
    {
        delete relay_socket;
        delete []socket->querybuf;

        std::cout<<"123"<<std::endl;
        return C_ERR;
    }

    /* 보내고 epoll_wait을 통해 데이터를 받았을 때 어디로 보내야 하는가 */
    /* 인자로 주어진 socket에게 줘야하는데 어떻게? */

    /* relay socket에 연결된 socket에게 줘야 함 */
    if(relay_socket->SendSocket(socket->querybuf, socket->querylen) == C_ERR)
    {
        delete []socket->querybuf;
        std::cout<<"1234"<<std::endl;
        return C_ERR;
    }

    delete []socket->querybuf;
    relay_socket->connection_pair_fd = socket->fd;

    return C_OK;
}

Proxy::~Proxy()
{
    for(auto &t: proxy_threads)
    {
        t.join();
    }

    delete bm;
}