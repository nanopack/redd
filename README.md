# VTEPD

## What is VTEPD?
Vtepd is a management plain for the Linux vxlan module. The Linux vxlan module can use multicast to auto-discover peers, and send broadcast packets (like arp requests). Many datacenters don't handle multicast traffic very well when hosts are on different network segments. The other method is to seed the forwarding database with the location to find the IPs. This doesn't work very well if you want to have IPs switch between machines. Vtepd is a compromise between the two. Vtepd listens for multicast packets and sends a copy of it out to each node in a list allowing hosts that can't multicast to each other to talk to each other. This allows arp requests and other broadcast packets to be delivered to each host that needs them.

## How to use VTEPD:

### Usage
    Usage: vtep [/path/to/vtep.conf] [options]
           vtep - (read config from stdin)
           vtep -v or --version
           vtep -h or --help
    Examples:
           vtep (run the server with default conf)
           vtep /etc/vtep/4000.conf
           vtep --port 7777
           vtep /etc/myvtep.conf --loglevel verbose
    
### Configuration options

The following things can be set in the vtepd configuration file:

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
Path to file where a pidfile is stored. Defaults to "/var/run/vtep.pid".
#### daemonize
Start up as a daemonized background process. Options are "yes" and "no". Defaults to "no".
#### loglevel
How much output to the log file. Options are "debug", "verbose", "notice", and "warning". Defaults to "notice".
#### logfile
Path to file where to output logs. Outputs to stdout if empty. Defaults to stdout.
#### syslog-enabled
Send logs to syslog. Options are "yes" and "no". Defaults to "no".
#### syslog-ident
Identifier string for syslog. Defaults to "vtep".
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
Path to directory to save data for IPs and host nodes. Defaults to "/var/db/vtep/".
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