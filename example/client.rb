#!/usr/bin/ruby
require_relative '../lib/opcua/client'

### username & pass in url
# client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")

client = OPCUA::Client.new("opc.tcp://localhost:4840")

node = client.get 2, 2117628832
node.on_change do |value,timestamp|
  p value
end

client.publishing_interval = 100
while true
  client.check_subscription
  sleep client.publishing_interval / 1000
end
