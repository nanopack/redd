#!/bin/bash
wget -O /tmp/libbframe_1.0.0-1_amd64.deb https://s3.amazonaws.com/nanopack.nanobox.io/deb/libbframe_1.0.0-1_amd64.deb
wget -O /tmp/libmsgxchng_1.0.0-1_amd64.deb https://s3.amazonaws.com/nanopack.nanobox.io/deb/libmsgxchng_1.0.0-1_amd64.deb
wget -O /tmp/red_1.0.0-1_amd64.deb https://s3.amazonaws.com/nanopack.nanobox.io/deb/red_1.0.0-1_amd64.deb
wget -O /tmp/redd_1.0.0-1_amd64.deb https://s3.amazonaws.com/nanopack.nanobox.io/deb/redd_1.0.0-1_amd64.deb

sudo apt-get -qq update
sudo apt-get -y install libmsgpack3 libuv0.10

sudo dpkg -i /tmp/libbframe_1.0.0-1_amd64.deb
sudo dpkg -i /tmp/libmsgxchng_1.0.0-1_amd64.deb
sudo dpkg -i /tmp/red_1.0.0-1_amd64.deb
sudo dpkg -i /tmp/redd_1.0.0-1_amd64.deb

sudo wget -O /etc/redd.conf https://raw.githubusercontent.com/nanopack/redd/master/redd.conf
sudo mkdir -p /var/db/redd
sudo redd /etc/redd.conf
while [[ $(red ping) == "Error: Unable to connect to RED\n" ]]; do
	sleep 1
done
