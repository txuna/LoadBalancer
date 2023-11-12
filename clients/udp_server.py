import threading
from socket import *
import msgpack 
import struct 
import json
import time

LoadBalancerIP = "127.0.0.1"
LoadBalancerPORT = 9988 
ApiPORT = 15000
bindPORT = 20000

def create_msgpack(value):
    byte = msgpack.packb(value)
    length = struct.pack('i', len(byte))
    msg = length + byte
    return msg 

def register():
    msg = create_msgpack({
        "cmd" : "register", 
        "protocol" : "udp", 
        "port" : bindPORT, 
        "relay_port" : ApiPORT
    })
    
    return msg

def healthcheck():
    msg = create_msgpack({
        "cmd" : "healthcheck",
    })
    
    return msg


def unregister():
    msg = create_msgpack({
        "cmd" : "unregister", 
        "protocol" : "udp", 
        "port" : bindPORT, 
        "relay_port" : ApiPORT
    })
    
    return msg

# Control Channel
def send_handler(client_socket, msg):
    client_socket.send(msg)


def recv_handler(client_socket):
    while True:
        data = client_socket.recv(4096)
        if len(data) < 4:
            return 
        
        while True:
            offset = 0
            unpack_data = struct.unpack("i", data[offset:4])
            length_r = unpack_data 
            decoded_data = msgpack.unpackb(data[offset+4:length_r[0]+4], raw=False)
            json_data = json.dumps(decoded_data, indent=2)
            
            response = json.loads(json_data)

            if "cmd" in response:
                if response["cmd"] == "healthcheck":
                    print("Health Check!")
                    s_thr = threading.Thread(target=send_handler, args=(client_socket, healthcheck(), ))
                    s_thr.start()
                    
                else:
                    print("DDDD")
                    
            else:
                print(response)
                
            
            offset = 4 + length_r[0]
            if offset >= len(data):
                break; 


def connect_server():
    client_socket = socket(AF_INET, SOCK_STREAM)
    client_socket.connect((LoadBalancerIP, LoadBalancerPORT))
    return client_socket

def echo_server():
    server_socket = socket(AF_INET, SOCK_DGRAM)
    address = ('localhost', ApiPORT)
    server_socket.bind(address)
    
    while True:
        data, address = server_socket.recvfrom(4096)
        print(f"수신한 데이터: {data.decode()} from {address}")
        server_socket.sendto(data, address)
        print("전송 완료")

if __name__ == '__main__':
    
    # Control Channel run 
    client_socket = connect_server()
    
    r_thr = threading.Thread(target=recv_handler, args=(client_socket, ))
    s_thr = threading.Thread(target=send_handler, args=(client_socket, register(), ))
    
    r_thr.start()
    s_thr.start()
    
    time.sleep(1)
    # Echo Server Run 
    echo_server()