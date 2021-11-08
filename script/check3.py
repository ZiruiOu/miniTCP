import argparse
import os
import sys
from utils import install_ns, lauch_terminals, delete_ns, bypass_kernel_ip, cancell_bypass_ip

CHECKPOINT5_INSTALL_PATH: str = "script/install_topo1.sh"
CHECKPOINT5_SCRIPT_PATH: str = "script/enter_ns.sh"
CHECKPOINT5_DELETE_PATH: str = "script/exit_ns.sh"
NUM_NETWORK: int = 2

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--install", help="install checkpoint topology")
parser.add_argument("-d", "--delete", help="delete installed NSs")
args = parser.parse_args()

if __name__ == "__main__":
    if args.install:
        install_ns(CHECKPOINT5_INSTALL_PATH)
        lauch_terminals(CHECKPOINT5_SCRIPT_PATH, NUM_NETWORK)
    elif args.delete:
        delete_ns(CHECKPOINT5_DELETE_PATH, NUM_NETWORK)