#!/usr/bin/ruby
require_relative '../lib/opcua/client'

# p OPCUA::Client.new("opc.tcp://localhost:4840",nil,nil)
client = OPCUA::Client.new("opc.tcp://localhost:4840","OpcUaClient","SUNRISE")
node = client.find(2,"/Channel/ProgramInfo/progName")

p node.value
