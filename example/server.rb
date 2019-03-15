#!/usr/bin/ruby
require_relative '../lib/opcua/server'

Daemonite.new do
  server = OPCUA::Server.new
  server.add_namespace "https://centurio.work/kelch"

  mt = server.types.add_object_type(:MeasurementType).tap{ |t|
    t.add_variable :SollWertX, OPCUA::OPTIONAL
    t.add_variable :SollWertY
    t.add_variable :SollWertZ
  }
  tt = server.types.add_object_type(:ToolType).tap{ |t|
    t.add_variable :ToolNumber
    t.add_variable :DuploNumber
    t.add_variable :testValue1
    t.add_object(:Measurements, server.types.folder).tap{ |u|
      u.add_object :M1, mt
    }
  }
  pt = server.types.add_object_type(:PresetterType).tap{ |t|
    t.add_variable :ManufacturerName
    t.add_object(:Tools, server.types.folder).tap{ |u|
      u.add_object :Tool2, tt
    }
  }
  server.objects.add_object :KalimatC34, pt

  run do
    sleep server.run
  end
end.loop!
