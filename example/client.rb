#!/usr/bin/ruby
require_relative '../lib/opcua/client'

client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")
node = client.find(2,"/Channel/ProgramInfo/progName")

p node.value
