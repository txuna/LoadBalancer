import socket

# UDP 소켓 생성
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 서버 주소 및 포트
server_address = ('localhost', 20000)

while True:
    message = input("전송할 메시지 입력 (종료를 원할 시 'exit' 입력): ")
    if message.lower() == 'exit':
        break

    # 메시지를 서버로 전송
    client_socket.sendto(message.encode(), server_address)

    # 서버로부터 응답 받기
    data, address = client_socket.recvfrom(4096)
    print(f"서버 응답: {data.decode()}")

# 소켓 닫기
client_socket.close()