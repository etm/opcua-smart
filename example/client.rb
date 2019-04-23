#!/usr/bin/ruby
require_relative '../lib/opcua/client'

### username & pass in url (e.g. siemens)
# client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")

client = OPCUA::Client.new("opc.tcp://localhost:4840")
client.subscription_interval = 100 # default 500

exit

node = client.get 2, 874565491 # get node from nodeid
node.on_change do |value,timestamp|
  p value
end

while true
  client.check_subscription
  sleep client.publishing_interval / 1000
end
