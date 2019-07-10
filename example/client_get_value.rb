#!/usr/bin/ruby
require_relative '../lib/opcua/client'
#require 'opcua/client'

### username & pass in url (e.g. siemens)
# client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")

client = OPCUA::Client.new("opc.tcp://localhost:4840")
client.subscription_interval = 100 # default 500

if (node = client.get 2, '/KalimatC34/Tools/Tool1/ToolNumber')  # get node from nodeid
  p node
  p node.value
else
  p 'invalid nodeid'
end
