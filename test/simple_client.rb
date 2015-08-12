#!/usr/bin/env ruby

require 'socket'

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
    # 100.times do
      socket.print [message.bytes.count].pack('L') # 32-bit unsigned, native endian (uint32_t)
      socket.print message
    # end
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

loop do

  print "type message> "
  connection.deliver gets.chomp

end

# connection.deliver File.read(File.expand_path("../ipsum/four.txt", __FILE__))
