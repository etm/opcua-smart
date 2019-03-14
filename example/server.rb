#!/usr/bin/ruby
require_relative '../lib/opcua/server'

Daemonite.new do
  server = OPCUA::Server.new
  pt = server.types.add_object_type(:PresetterType).tap{ |pt|
    pt.add_variable :ManufacturerName
  }
  server.objects.add_object :KalimatC34, pt
  run do
    sleep server.run
  end
end.loop!
