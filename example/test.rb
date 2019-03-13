#!/usr/bin/ruby
require_relative '../lib/opcua/server'

server = OPCUA::Server.new
p server.types
p (pt = server.types.add_object_type :PresetterType)
p pt.add_variable :ManufacturerName
