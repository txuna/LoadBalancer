import threading 
import requests
import time

def dummy():
    res = requests.get("http://127.0.0.1:50000")
    print(res)

task = []

start = time.time()

for i in range(0, 10):
    r_thr = threading.Thread(target=dummy)
    task.append(r_thr)
    r_thr.start() 
    
    
for t in task:
    t.join()
    
end = time.time()

print(f"FIN: {end - start:.5f} sec")
