#!/usr/bin/ruby
require_relative '../../lib/opcua/client'
#require 'opcua/client'

### username & pass in url (e.g. siemens)
# client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")

client = OPCUA::Client.new("opc.tcp://localhost:4840")
client.default_ns = 2

if (node = client.get '/KalimatC34/Tools/Tool1/ToolNumber')
  node.value = [:a, :b]
else
  p 'invalid nodeid'
end
