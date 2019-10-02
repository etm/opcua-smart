#!/usr/bin/ruby
require_relative '../lib/opcua/client'
#require 'opcua/client'

### username & pass in url (e.g. siemens)
# client = OPCUA::Client.new("opc.tcp://OpcUaClient:SUNRISE@localhost:4840")

client = OPCUA::Client.new("opc.tcp://localhost:4840")
client.debug = false
client.default_ns = 2

# node = client.get '/KalimatC34/Tools/Tool3/testMethod'
# p node.call 'abcde', Time.now
# client.disconnect

node = client.get 1, 62541
p node.to_s
p node.call 'a'
