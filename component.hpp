#ifndef __COMPONENT_H_
#define __COMPONENT_H_

#include <vector>
#include <string>
#include <iostream>

#include "sock.hpp"

/**
 * NOTE! 
 * 전체 컴포넌트 리스트와 컴포넌트별로 사용중인 binding port들에 대한 관리 필요
 * binding port는 참조하는 레퍼런스의 갯수 체크 필요 0이 될 시 해당 포트 close
*/

class IPEndpoint
{
    public:
        int port; 
        int addr;
        std::string protocol;
};

class Bind
{
    public:
        int port; 
        int count; 

        Bind();
        virtual ~Bind();
        virtual void Open() = 0;
        virtual void Close() = 0;

        void Dereference();
        void Reference();
};

class TcpBind : public Bind
{
    public:
        Net::TcpSocket *socket;

        TcpBind();
        virtual ~TcpBind();
        virtual void Open();
        virtual void Close();
};

class UdpBind : public Bind
{
    public:
        Net::UdpSocket *socket;

        UdpBind();
        virtual ~UdpBind();
        virtual void Open();
        virtual void Close();
};

class Component
{
    public: 
        IPEndpoint ip_endpoint;

        Component();
        ~Component();
};


/* NOTE! This class will be accessed by many proxy threads. SO, NEED LOCK! */
class ComponentManager
{
    private:
        std::vector<Component> components;
        std::vector<Bind*> binds;

    public:
        ComponentManager();
        ~ComponentManager();

        int AppendComponent();
        int DeleteComponent();
        int FindComponent();
        std::vector<Component> &LoadComponents();
};

#endif 