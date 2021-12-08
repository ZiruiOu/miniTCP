./bypassKernel 
tc qdisc dev veth1-7 add netem rate 5kbps loss 2% delay 500ms
cd ../../../build
sudo ./cong_server