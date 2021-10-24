cd 3rdparty/vnetUtils/helper
sudo ./addNS vr
sudo bash ./execNS vr bash ./addVethPair vr-1 vr-2
sudo bash ./execNS vr bash ./giveAddr vr-1 11.22.33.44
sudo bash ./execNS vr bash ./giveAddr vr-2 22.33.44.55
sudo bash ./execNS vr bash
cd ../../..
