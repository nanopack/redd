#!/usr/bin/env ruby
 
require 'socket'
require 'msgpack'
 
class Connection
 
  MAX_ATTEMPTS = 30
 
  attr_accessor :socket
  
  def initialize(host='127.0.0.1', port=4000, autoconnect=true)
    @host     = host
    @port     = port
    @attempts = 0
    connect if autoconnect
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
end
 
host = '127.0.0.1'
port = '4000'
 
connection = Connection.new host, port, false
 
connection.deliver ["hi", 5, "what all can go in here?", [{"key" => "value"}, {"another" => "hash", "with" => {"nested" => "values"}}]].to_msgpack

data = connection.receive

puts data

puts MessagePack.unpack(data)

data = connection.receive

puts data

puts MessagePack.unpack(data)

data = connection.receive

puts data

puts MessagePack.unpack(data)