#include "balancer.hpp"
#include "common.hpp"
#include <iostream>

BalancerProxy::BalancerProxy()
{

}

BalancerProxy::~BalancerProxy()
{

}

ErrorCode BalancerProxy::Verify(const json &req)
{
    if(req.contains("cmd") == false
    || req.contains("protocol") == false
    || req.contains("port") == false)
    {
        return ErrorCode::InvalidRequest;
    }

    if(req["cmd"].type() != json::value_t::string
    || req["protocol"].type() != json::value_t::string
    || req["port"].type() != json::value_t::number_unsigned)
    {
        return ErrorCode::InvalidRequest;
    }

    return ErrorCode::None;
}

json BalancerProxy::Controller(const json &req)
{
    json response = {
        {"error", ErrorCode::None},
    }; 

    /* 유효성 검사 */
    ErrorCode result = Verify(req);
    if(result != ErrorCode::None)
    {
        response["error"] = result;
        return response;
    }
    
    std::string cmd = req["cmd"];
    std::string protocol = req["protocol"];
    int port = req["port"];

    if(cmd == "register")
    {
        response["error"] = RegisterComponent(protocol, port);
    }

    else if(cmd == "unregister")
    {
        response["error"] = UnRegisterComponent(protocol, port);
    }
    
    else
    {
        response["error"] = ErrorCode::InvalidCmd;
        return response;
    }

    return response;
}

/**
 * {"cmd": "register", "protocol", "tcp", "port": 80}
*/
/**
 * 넘겨받은 port, 넘겨받은 주소 쌍 만들기 == (port, IPEndpoint)
*/
ErrorCode BalancerProxy::RegisterComponent(std::string protocol, int port)
{

    return ErrorCode::None;
}

/**
 * {"ack": "successful"} or {"ack": "failed", "msg": "…"}
*/
ErrorCode BalancerProxy::UnRegisterComponent(std::string protocol, int port)
{
    return ErrorCode::None;
}

json BalancerProxy::HealthCheckComponent()
{
    json response = {

    };

    return response;
}