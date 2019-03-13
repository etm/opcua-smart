#!/usr/bin/ruby
require_relative '../lib/opcua/server'

Daemonite.new do
  server = OPCUA::Server.new
  server.types.add_object_type(:PresetterType).tap do |pt|
    pt.add_variable :ManufacturerName
  end
  run do
    sleep server.run
  end
end.loop!
