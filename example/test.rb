#!/usr/bin/ruby
require_relative '../lib/opcua/server'

Daemonite.new do
  server = OPCUA::Server.new
  server.add_object_type :PresetterType
  server.add_variable :PresetterType, :ManufacturerName
  server
  run do
    sleep server.run
  end
end.loop!
