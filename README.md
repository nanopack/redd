# REDD
[![Build Status](https://travis-ci.org/nanopack/redd.svg)](https://travis-ci.org/nanopack/redd)

## What is REDD?
Redd is a management plain for the Linux vxlan module. The Linux vxlan module can use multicast to auto-discover peers, and send broadcast packets (like arp requests). Many datacenters don't handle multicast traffic very well when hosts are on different network segments. The other method is to seed the forwarding database with the location to find the IPs. This doesn't work very well if you want to have IPs switch between machines. Redd is a compromise between the two. Redd listens for multicast packets and sends a copy of it out to each node in a list allowing hosts that can't multicast to each other to talk to each other. This allows arp requests and other broadcast packets to be delivered to each host that needs them.

## How to use REDD:

### Usage
    Usage: red [/path/to/red.conf] [options]
           red - (read config from stdin)
           red -v or --version
           red -h or --help
    Examples:
           red (run the server with default conf)
           red /etc/red/4000.conf
           red --port 7777
           red /etc/myred.conf --loglevel verbose
    
### Configuration options

The following things can be set in the redd configuration file:

- pidfile
- daemonize
- loglevel
- logfile
- syslog-enabled
- syslog-ident
- syslog-facility
- port
- bind
- timeout
- routing-enabled
- udp-listen-address
- udp-recv-buf
- udp-send-buf
- save-path
- vxlan-name
- vxlan-vni
- vxlan-group
- vxlan-port
- vxlan-interface

#### pidfile
Path to file where a pidfile is stored. Defaults to "/var/run/red.pid".
#### daemonize
Start up as a daemonized background process. Options are "yes" and "no". Defaults to "no".
#### loglevel
How much output to the log file. Options are "debug", "verbose", "notice", and "warning". Defaults to "notice".
#### logfile
Path to file where to output logs. Outputs to stdout if empty. Defaults to stdout.
#### syslog-enabled
Send logs to syslog. Options are "yes" and "no". Defaults to "no".
#### syslog-ident
Identifier string for syslog. Defaults to "red".
#### syslog-facility
Syslog facility to use. Options are "USER", "LOCAL0", "LOCAL1", "LOCAL2", "LOCAL3", "LOCAL4", "LOCAL5", "LOCAL6", and "LOCAL7". Defaults to "LOCAL0".
#### port
Port for the API to listen to. Defaults to "4470".
#### bind
IP address for the API to bind to. Defaults to "0.0.0.0".
#### timeout
API timeout. Defaults to infinite.
#### routing-enabled
Enable the routing of packets. Options are "yes" and "no". Defaults to "yes".
#### udp-listen-address
IP address for unicast packets to send / receive from. Defaults to "localhost".
#### udp-recv-buf
Receive buffer size. Defaults to "114688".
#### udp-send-buf
Send buffer size. Defaults to "114688".
#### save-path
Path to directory to save data for IPs and host nodes. Defaults to "/var/db/red/".
#### vxlan-name
Name of the vxlan interface. Defaults to "vxlan0".
#### vxlan-vni
Virtual network identifier for the vxlan interface. Defaults to "1".
#### vxlan-group
Multicast address for the vxlan interface. Defaults to "239.0.0.1".
#### vxlan-port
Port to configure the vxlan driver to use. Defaults to "8472".
#### vxlan-interface
Physical interface to send vxlan traffic over. Defaults to "eth0".

[![open source](http://nano-assets.gopagoda.io/open-src/nanobox-open-src.png)](http://nanobox.io/open-source)
