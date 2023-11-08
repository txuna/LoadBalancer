#ifndef __BALANCER_PROXY_H_
#define __BALANCER_PROXY_H_

#include "main.hpp"
#include "component.hpp"



/* 연결된 컴포넌트는 tcp_proxy나 udp_proxy에서 접근할 수 있어야 함 */

/**
 * 연결된 컴포넌트와의 register, unregister, health check 기능 수행
*/
class BalancerProxy
{
    public:
        BalancerProxy();
        ~BalancerProxy();
        
        ErrorCode RegisterComponent(std::string protocol, int port);
        ErrorCode UnRegisterComponent(std::string protocol, int port);
        json Controller(const json &req);
        json HealthCheckComponent();
        ErrorCode Verify(const json &req);
};



#endif