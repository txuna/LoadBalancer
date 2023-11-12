import socket
import time
import threading
import sys

task = []


def dummpy(client_socket):
    message = "hello"
    client_socket.sendto(message.encode(), server_address)
    data, address = client_socket.recvfrom(4096)
    print(f"서버 응답: {data.decode()}")

if len(sys.argv) != 3:
    print("[USAGE] python3 udp_dummpy_client.py [Server IP] [Server Port]")
    exit(1)

# UDP 소켓 생성
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 서버 주소 및 포트
server_address = (sys.argv[1], int(sys.argv[2]))

start = time.time()

for i in range(0, 10):
    r_thr = threading.Thread(target=dummpy, args=(client_socket, ))
    task.append(r_thr)
    r_thr.start()

for t in task:
    t.join()

end = time.time()
print(f"FIN: {end - start:.5f} sec")

# 소켓 닫기
client_socket.close()