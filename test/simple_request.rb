#!/usr/bin/env ruby
# 
# Copyright (c) 2015 Pagoda Box Inc
# 
# This Source Code Form is subject to the terms of the Mozilla Public License, v.
# 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
# at http://mozilla.org/MPL/2.0/.
# 
 
require 'socket'
require 'msgpack'
require 'pp'
 
class Connection
 
  MAX_ATTEMPTS = 30
 
  attr_accessor :socket
  
  def initialize(host='127.0.0.1', port=4000, autoconnect=true)
    @host     = host
    @port     = port
    @attempts = 0
    connect if autoconnect
    @request = 0
  end
 
  def deliver(message)
    socket.print [message.bytes.count].pack('L') # 32-bit unsigned, native endian (uint32_t)
    socket.print message
    socket.flush
  end
 
  def receive
    len = socket.recv(4).unpack('L')[0] # 32-bit unsigned, native endian (uint32_t)
    data = ''
    until data.bytes.count == len do
      data += socket.recv (len - data.bytes.count)
    end
    data
  end
 
  def socket
    @socket ||= establish_connection
  end
 
  alias :connect :socket
 
  def establish_connection
    TCPSocket.open @host, @port
  end

  def send_request(command, data)
    id = "#{@request}"
    @request += 1
    deliver({command: command, data: data.to_msgpack, id: id}.to_msgpack)
    receive_responses()
  end

  def receive_response()
    message = receive
    unpacked = MessagePack.unpack(message)
    puts "id: #{unpacked["id"]}, data: #{MessagePack.unpack(unpacked["data"])}, status: #{unpacked["status"]}"
    {"id" => unpacked["id"], "data" => MessagePack.unpack(unpacked["data"]), "status" => unpacked["status"] }
  end

  def receive_responses()
    responses = []
    begin
      responses << receive_response
    end while responses.last['status'] != 'complete'
    responses
  end
end
 
host = 'red.local'
port = '4470'

connection = Connection.new host, port, false

connection.send_request("ping", {foo: "bar"})

connection.send_request("device.add", {vni: "0", mac: "00:00:00:00:00:00", device: 'something'})

connection.send_request("device.add", {vni: "1", mac: "01:23:45:67:89:ab", device: 'simnet0'})
connection.send_request("device.add", {vni: "1", mac: "01:23:45:67:89:cd", device: 'simnet1'})
connection.send_request("device.add", {vni: "2", mac: "01:23:45:67:89:ef", device: 'simnet2'})

connection.send_request("vxlan.list", {})

connection.send_request("device.list", {})

connection.send_request("device.list", {vni: "1"})

connection.send_request("device.list", {vni: "2"})

connection.send_request("device.remove", {device: 'simnet0'})

connection.send_request("device.remove", {device: 'simnet0'})

connection.send_request("device.remove", {device: 'simnet2'})

connection.send_request("vxlan.list", {})

connection.send_request("device.list", {})

connection.send_request("device.list", {vni: "1"})

connection.send_request("device.list", {vni: "2"})

connection.send_request("member.add", {vni: "1", mac: "12:12:12:23:23:23", host: '1.2.3.4'})

connection.send_request("member.add", {vni: "2", mac: "12:12:12:23:23:23", host: '2.3.4.5'})
connection.send_request("device.add", {vni: "2", mac: "01:23:45:67:89:ef", device: 'simnet2'})
connection.send_request("member.add", {vni: "2", mac: "12:12:12:23:23:23", host: '2.3.4.5'})
connection.send_request("member.add", {vni: "2", mac: "45:45:45:23:23:23", host: '3.4.5.6'})
connection.send_request("member.list", {vni: "1"})
connection.send_request("member.list", {vni: "2"})
connection.send_request("member.remove", {vni: "2", mac: "45:45:45:23:23:23"})
connection.send_request("member.list", {vni: "2"})