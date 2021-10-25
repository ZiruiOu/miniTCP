# miniTCP
A mini network protocol stack built upon libpcap.



### Usage

#### Part A

To install the demo program, please first enter the miniTCP directory and run the following command.

```shell
mkdir build
cd build
cmake ..
make
```

Checkpoint 1:



Checkpoint 2:

To run the demo, please first use

```shell
bash script/install_ckpt2.sh
```

to install the NS environment.



You can run 

```shell
bash script/enter_ns1.sh
```

to enter ns#1 in one terminal. And run

```shell
bash script/etner_ns2.sh
```

to enter ns#2 in another terminal.



The topology of ns#1 and ns#2 follows the example in vnetUtils.

If you want to send message from one veth to another, first enter the **build directory in miniTCP**, then you can run 

```shell
./link_app
```

to run the applications on both of the terminals.



To send a message from veth 1-2 to veth 2-1, you can input the message in the form as the user prompt goes.

```
sender_device_name receiver_mac_address message
```

the message ends when you press the 'enter' key and will be sent to the receiver.



For example, if you want to send a message "Hello, How are you?" from veth 1-2 (in terminal#1) to veth 2-1 (in terminal#2), you can enter something like this on terminal#1.

```shell
veth1-2 52:ee:51:6f:18:ff Hello, How are you?
```

the device veth 2-1 will receive this message and use a callback function to print out the message received.
