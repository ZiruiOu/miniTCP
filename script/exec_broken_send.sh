./bypassKernel
cd ../../../build
tc  qdisc  add  dev  veth1-2  root  netem  loss 20%
sudo ./broken_send 10.102.1.2