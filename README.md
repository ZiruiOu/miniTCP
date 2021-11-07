# miniTCP
A mini network protocol stack built upon libpcap.



## Part A

To install the demo program, please first enter the miniTCP directory and run the following command.

```shell
mkdir build
cd build
cmake ..
make
```

##### Note:

The network topology we use in checkpoint 1 and checkpoint 2 is exactly **the same as the example** given in the vnetUtils.

![topology](./demo/topology.png)



#### Checkpoint 1:  Show that your implementation can detect network interfaces on the host.

To run the demo, first use the following command to activate the NS environment.

```shell
bash script/install_ckpt1.sh
```

Run following command to enter ns#3.

```shell
bash script/enter_ns3.sh
```



To go back to the **build directory in miniTCP** after executing the bash script , please run the following command in the terminal.

```
cd ../../../build
```

To see the demo, please enter the build director in the root of miniTCP and run the following command.

```shell
./link_app
```



You can see the result of devices like this, which implies that the ethernet kernel successfully find all devices.

![checkpoint1](./demo/checkpoint1.jpg)



#### Checkpoint 2:  Show that your implementation can capture frames from a device and inject frames to a device using libpcap.

To run the demo, use the following command to activate the NS environment.

```shell
bash script/install_ckpt2.sh
```

Run following command to enter ns#1 in one terminal (namely, terminal#1).

```shell
bash script/enter_ns1.sh
```

Run following command on another terminal (namely, terminal#2) to enter ns#2.

```shell
bash script/etner_ns2.sh
```



To show that we can inject frames to a device as well as capture frames from a device, our demo is sending a greeting message from one terminal to another.



For example,  If you want to send a greeting message from veth 1-2 (in terminal#1) to veth 2-1 (in terminal#2).  



First start a listening application on terminal#2 by running

```shell
./link_recv
```



Suppose the mac address of destination (veth 2-1) is 52:ee:51:6f:18:ff, you can send the message by running on terminal#1.

```shell
./link_send veth1-2 52:ee:51:6f:18:ff
```



And the device veth 2-1 will receive this message and use a callback function to print out the message received.

The result of the checkpoint 2.

![checkpoint2](./demo/checkpoint2.jpg)






## Part B

### Writing task1: Explain how you addressed this problem when implementing IP protocol.

​	I implement an arp protocol above the link layer [in the files ethernet/arp.h and ethernet/arp_impl.h]. If a device needs to find out the corresponding MAC address of an IP address when trying to send the IP datagram,  it can lookup the MAC address from the arp table.

​	To be more specific, the arp table on each host (terminals) is a singleton, the devices belong to this host would send out an ARP request as an announcement every 8 seconds. If a device receives an ARP request, it would first match whether the receiver IP matches its onw IP address, if it matches or the receiver IP is an broadcast IP, it would send back an ARP reply. An ARP entry will be removed from the ARP table if it is not updated within 48 seconds. 



### Writing task2: Describe your routing algorithm.

​	I implement RIP protocol above the link layer.

   To be more specific, every host would maintain a local routing table, recording the destination, netmask of the destination, the corresponding nexthop ip and the metric (also known as the distance). We use the **hop number** from this host to the destination as the metric.

   A **timer will be maintained for each routing entry**. If a routing entry isn't updated by the host's neighbors within 18 seconds, the distance of the entry will be mark as inreachable  by setting it as 16 (the inreachable threshold) and the status of the entry will be marked as GARBAGE, a garbage collection will set set for this entry. If this entry is still not updated by the corresponding neighbor    within 12 seconds, this entry will be remove from the routing entry immediately. By using timeout, we can automatically adjust our routing table when the network condition changes.

  The hosts in the network will **send its own distance vector to its neighbors every 3 seconds**. The distance vector is an serialized object. It is consisted of the sender ip (4 bytes), number of entries (4 bytes) and multiple distance vector entries.

  A distance vector entry is defined as the following C struct. It mainly maintains the destination ip, the netmask and the distance from the sender host to the destination network.

  When the host receives a distance vector from its neighbour, the host first deserielize the distance vector, figure out the sender ip (the nexthop ip), the number of entries that the distance vector contains and the content of the distance vector. Then the host updates its own routing table according to the information it receives.

  To accelerate the covergence of the algorithm, the **poison reversing** technique is also applied to the construction of the distance vector. Upon constructing the distance vector for a specific neighbor, we will mark the distance of the entries that learnt from this neighbor as inreachable.

  Since I haven't finish the transport layer yet, the only layer I can relay on to send the distance vector would be the **link layer**, which means the current algorithm is **sending/receiving raw ethernet frame**, which seems to be quiet strange. I will use a formal RIP protocol packet to construct my RIP protocol when UDP is built. 