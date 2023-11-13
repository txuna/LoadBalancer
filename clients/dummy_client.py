import threading 
import requests
import time
import sys

def dummy(ip, port):
    res = requests.get("http://{0}:{1}".format(ip, port))
    #print(res)


if len(sys.argv) != 3:
    print("[USAGE] python3 dummy_client.py [Server IP] [Server Port]")
    exit(1)

task = []

start = time.time()

for i in range(0, 10000):
    r_thr = threading.Thread(target=dummy, args=(sys.argv[1], sys.argv[2], ))
    task.append(r_thr)
    r_thr.start() 
    
    
for t in task:
    t.join()
    
end = time.time()

print(f"FIN: {end - start:.5f} sec")
