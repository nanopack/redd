#!/usr/bin/env ruby
 
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
  end
 
  def deliver(message)
    socket.print [message.bytes.count].pack('L') # 32-bit unsigned, native endian (uint32_t)
    socket.print message
    socket.flush
  end
 
  def receive
    len = socket.recv(4).unpack('L')[0] # 32-bit unsigned, native endian (uint32_t)
    puts len
    data = ''
    until data.bytes.count == len do
      puts "receiving"
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

  def send_request(command, data, id)
    deliver({command: command, data: data.to_msgpack, id: id}.to_msgpack)
    receive_responses()
  end

  def receive_response()
    message = receive
    unpacked = MessagePack.unpack(message)
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
 
host = '127.0.0.1'
port = '4000'

10.times do |loop_num|
  connection = Connection.new host, port, false

  100.times do |num|
    puts "sending ping (loop: #{loop_num} num: #{num})"
    pp connection.send_request("ping", {ping: "ping"}, "#{num}")
  end

  100.times do |num|
    puts "sending run (loop: #{loop_num} num: #{num})"
    pp connection.send_request("run", {hook: "echo", payload: "bye" }, "#{1000 + num}")
  end

  100.times do |num|
    puts "sending run (loop: #{loop_num} num: #{num})"
    pp connection.send_request("run", {hook: "ps", payload: "aux" }, "#{2000 + num}")
  end

  100.times do |num|
    puts "sending run with bad hook (loop: #{loop_num} num: #{num})"
    pp connection.send_request("run", {hook: "george", payload: "bye" }, "#{3000 + num}")
  end

  100.times do |num|
    puts "sending garbage (loop: #{loop_num} num: #{num})"
    pp connection.send_request("blah", {blah: "blah"}, "#{4000 + num}")
  end

  connection.socket.close
end

connection = Connection.new host, port, false

puts "sending stop"
pp connection.send_request("stop", {stop: "stop"}, "400")
