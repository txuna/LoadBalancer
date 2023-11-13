import socket
import time
import threading
import sys

task = []


def dummpy(client_socket, index):
    try:
        message = "hello"
        client_socket.sendto(message.encode(), server_address)
        data, address = client_socket.recvfrom(4096)
        print("[Log] get data from server: "+str(index)+" thread")
    except:
        print("[Error] disconnected from server: "+str(index)+" thread")


if len(sys.argv) != 4:
    print("[USAGE] python3 udp_dummpy_client.py [Server IP] [Server Port] [num of request]")
    exit(1)

# UDP 소켓 생성
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 서버 주소 및 포트
server_address = (sys.argv[1], int(sys.argv[2]))

start = time.time()

round = int(sys.argv[3])

for i in range(0, round):
    r_thr = threading.Thread(target=dummpy, args=(client_socket, i, ))
    task.append(r_thr)
    r_thr.start()

for t in task:
    t.join()

end = time.time()
print(f"FIN: {end - start:.5f} sec")

# 소켓 닫기
client_socket.close()