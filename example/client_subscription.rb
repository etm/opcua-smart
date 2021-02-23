#!/usr/bin/ruby
require_relative '../lib/opcua/client'
#require 'opcua/client'

### username & pass in url (e.g. siemens)
# client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")

client = OPCUA::Client.new("opc.tcp://localhost:4840")
client.subscription_interval = 100 # default 500

node = client.get 2, '/KalimatC34/Tools/Tool1/ToolNumber'  # get node from nodeid
n2 = client.get 2, '/KalimatC34/Tools/Tool2/ToolNumber'  # get node from nodeid
p node.unsubscribe
node.on_change do |value,timestamp|
  p value
  if value == 2
    p 'unsub'
    p node.unsubscribe
  end
end

while true
  client.check_subscription
  sleep client.subscription_interval / 1000
end
