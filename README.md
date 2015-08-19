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
Path to file where a pidfile is stored.
#### daemonize
Start up as a daemonized background process. Options are "yes" and "no".
#### loglevel
How much output to the log file. Options are "debug", "verbose", "notice", and "warning".
#### logfile
Path to file where to output logs.
#### syslog-enabled
Send logs to syslog. Options are "yes" and "no".
#### syslog-ident
Identifier string for syslog.
#### syslog-facility
Syslog facility to use. Options are "USER", "LOCAL0", "LOCAL1", "LOCAL2", "LOCAL3", "LOCAL4", "LOCAL5", "LOCAL6", and "LOCAL7".
#### port
Port for the API to listen to.
#### bind
IP address for the API to bind to.
#### timeout
API timeout.
#### routing-enabled
Enable the routing of packets. Options are "yes" and "no".
#### udp-listen-address
IP address for unicast packets to send / receive from.
#### udp-recv-buf
Receive buffer size.
#### udp-send-buf
Send buffer size.
#### save-path
Path to directory to save data for IPs and host nodes.
#### vxlan-name
Name of the vxlan interface.
#### vxlan-vni
Virtual network identifier for the vxlan interface.
#### vxlan-group
Multicast address for the vxlan interface.
#### vxlan-port
Port to configure the vxlan driver to use.
#### vxlan-interface
Physical interface to send vxlan traffic over.