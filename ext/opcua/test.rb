require_relative 'smart'

client = OPCUA::Smart.new("opc.tcp://localhost:4840")
p client
