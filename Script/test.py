import os
import time
import threading

num_of_clients = 3


def start_client():
    os.system("../Client/client")
    pass

if __name__ == "__main__":
    
    for _ in range(num_of_clients):
        thread = threading.Thread(target=start_client)
        thread.start()
        pass

    pass
