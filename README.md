# Load Balancer for C/C++ 
바인딩된 포트로의 요청시 연결된 컴포넌트에 적절하게 패킷을 분배하는 로드밸런서입니다. 

# Architecture
blah blah ~  

# Control Channel
- Register 
```json
{
    "cmd" : "register", 
    "protocol" : "tcp", 
    "port" : 30000, 
    "relay_port" : 50000 
}
```
동일한 API서버가 다른 머신에서 돌아간다면 port와 relay_port는 동일하지만 동일 머신에서 테스트하기 위해 다른값으로 세팅 

port : 실제 API서버가 오픈한 포트 
relay_port : 로드밸런서가 동작하는 머신에서 바인딩할 포트 

클라이언트는 relay_port로 접근을 시도한다. 로드밸런서는 relay_port를 실제 API서버의 port로 패킷 릴레이  

- UnRegister
```json 
{
    "cmd" : "unregister", 
    "protocol" : "tcp", 
    "port" : 30000, 
    "relay_port" : 50000 
}
```

- Health Check 
```json 
{
    "cmd" : "healthcheck" 
}
```
5초마다 연결된 서버들에게 보내는 상태 메시지 

# Dependency
blah blah ~

# Usage
```
git clone https://github.com/txuna/LoadBalancer.git
cd LoadBalancer 
make 
```

# Example 
blah blah ~