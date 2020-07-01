#!/usr/bin/ruby
require_relative '../lib/opcua/client'

client = OPCUA::Client.new("opc.tcp://128.130.57.76:4840")
client.subscription_interval = 100 # default 500

if (node = client.get 1, 'DISPLAY_VOLTAGE')  # get node from nodeid
  p node
  p node.value
else
  p 'invalid nodeid'
end
