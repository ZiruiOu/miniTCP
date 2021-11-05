import os

CHECKPOINT3_SCRIPT_PATH: str = "script/enter_ns.sh"
NUM_NETWORK: int = 6

def lauch_terminals(script_path: str, num_terminals: int):
    for i in range(num_terminals):
        os.system("gnome-terminal -e 'bash -c \" bash script/enter_ns.sh %d; exec bash\"'"%(i+1))

if __name__ == "__main__":
    lauch_terminals(CHECKPOINT3_SCRIPT_PATH, NUM_NETWORK)