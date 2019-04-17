#!/usr/bin/ruby
require_relative '../lib/opcua/client'

### username & pass in url
# client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")

client = OPCUA::Client.new("opc.tcp://localhost:4840")

node = client.get 2, 2117628832
node.on_change do |value|
  p value
end

client.publishing_interval = 2000
while true
  client.check_subscription
  sleep 0.1
end
