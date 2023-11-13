import threading 
import requests
import time
import sys

def dummy(ip, port, index):
    try:
        res = requests.get("http://{0}:{1}".format(ip, port))
    except:
        print("[Error] disconnected from server: "+ str(index)+" thread")
        return 
    
    print("[LOG] Get Data from server: "+ str(index)+" thread")


if len(sys.argv) != 4:
    print("[USAGE] python3 dummy_client.py [Server IP] [Server Port] [num of request]")
    exit(1)

task = []

start = time.time()

round = int(sys.argv[3])

for i in range(0, round):
    r_thr = threading.Thread(target=dummy, args=(sys.argv[1], sys.argv[2], i, ))
    task.append(r_thr)
    r_thr.start() 
    
    
for t in task:
    t.join()
    
end = time.time()

print(f"FIN: {end - start:.5f} sec")
