#!/usr/bin/ruby
require_relative '../../lib/opcua/server'
# require 'opcua/server'

Daemonite.new do
  server = OPCUA::Server.new
  server.add_namespace "https://centurio.work/kelch"

  mt = server.types.add_object_type(:MeasurementType).tap{ |t|
    t.add_variable :SollWertX
    t.add_variable :SollWertY
    t.add_variable :SollWertZ
  }
  tt = server.types.add_object_type(:ToolType).tap{ |t|
    t.add_object :M, mt, OPCUA::MANDATORY
  }

  tools = server.objects.manifest(:KalimatC34, tt)

  run do
    sleep server.run
  end
end.loop!
