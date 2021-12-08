./bypassKernel 
tc qdisc add dev veth1-7 root netem rate 20kbps
cd ../../../build
sudo ./router