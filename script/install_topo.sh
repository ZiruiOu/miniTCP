cd 3rdparty/vnetUtils
sudo dos2unix ./examples/* ./helper/*
sudo chmod +x ./examples/makeVNet ./examples/removeVNet ./helper/*
cd ./examples
sudo bash ./makeVNet < $1