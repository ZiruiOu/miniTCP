import os
import sys
from typing import List, Optional

def bypass_kernel_ip():
    os.system("../3rdparty/vnetUtils/helper/bypassKernel")

def cancell_bypass_ip():
    os.system("../3rdparty/vnetUtils/helper/undoBypass")

def install_ns(script_path: str, topology_name : Optional[str] = None):
    install_cmd: str = "bash " + script_path
    if topology_name is not None:
        install_cmd += " " + topology_name
    os.system(install_cmd)

def lauch_terminals(script_path: str, num_terminals: int, exec_path: str):
    for i in range(num_terminals):
        os.system("gnome-terminal -e 'bash -c \" bash %s %d %s; exec bash\"'"%(script_path, i+1, exec_path))

def lauch_terminals_by_scripts(script_path: str, num_terminals: int, exec_paths: List[str]):
    for i in range(num_terminals):
        os.system("gnome-terminal -e 'bash -c \" bash %s %d %s; exec bash\"'"%(script_path, i+1, exec_paths[i]))

def delete_ns(script_path: str, num_terminals: int):
    for i in range(num_terminals):
        os.system("bash %s %d"%(script_path, i + 1))