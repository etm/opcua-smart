#!/usr/bin/ruby
require_relative '../lib/opcua/server'

Daemonite.new do
  server = OPCUA::Server.new
  server.add_namespace "https://centurio.work/kelch"

  mt = server.types.add_object_type(:MeasurementType).tap{ |t|
    t.add_variable :SollWertX, OPCUA::MANDATORY
    t.add_variable :SollWertY, 1
    t.add_variable :SollWertZ, 1
  }
  tt = server.types.add_object_type(:ToolType).tap{ |t|
    t.add_variable :ToolNumber, 2
    t.add_variable :DuploNumber, 2
    t.add_variable :testValue1, 2
    t.add_object(:Measurements, server.types.folder, 2).tap{ |u|
      u.add_object :M1, mt, 2
    }
  }
  pt = server.types.add_object_type(:PresetterType).tap{ |t|
    t.add_variable :ManufacturerName, 2
    t.add_object(:Tools, server.types.folder, 17).tap{ |u|
      u.add_object :Tool2, tt, 2
    }
  }
  server.objects.add_object :KalimatC34, pt, 1

  run do
    sleep server.run
  end
end.loop!
