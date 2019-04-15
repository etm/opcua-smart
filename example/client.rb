#!/usr/bin/ruby
require_relative '../lib/opcua/client'

p OPCUA::Client.new("opc.tcp://localhost:4840","OpcUaClient","SUNRISE")
