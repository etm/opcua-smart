#!/usr/bin/ruby
require_relative '../lib/opcua/server'
#require 'opcua/server'

Daemonite.new do

  server = OPCUA::Server.new
  server.add_namespace "https://centurio.work/kelch"

  mt = server.types.add_object_type(:MeasurementType).tap{ |t|
    t.add_variable :SollWertX
    t.add_variable :SollWertY
    t.add_variable :SollWertZ
  }
  tt = server.types.add_object_type(:ToolType).tap{ |t|
    t.add_variable :ToolNumber
    t.add_variable :DuploNumber
    t.add_variable :testValue1
    t.add_method :testMethod, test1: OPCUA::TYPES::STRING, test2: OPCUA::TYPES::DATETIME do |node, test1, test2|
      # do something
    end
    t.add_object(:Measurements, server.types.folder).tap{ |u|
      u.add_object :M, mt, OPCUA::OPTIONAL
    }
  }
  pt = server.types.add_object_type(:PresetterType).tap{ |t|
    t.add_variable :ManufacturerName
    t.add_object(:Tools, server.types.folder).tap{ |u|
      u.add_object :Tool, tt, OPCUA::OPTIONAL
    }
  }

  tools = server.objects.manifest(:KalimatC34, pt).find(:Tools)

  t1 = tools.manifest(:Tool1,tt)
  t2 = tools.manifest(:Tool2,tt)
  t3 = tools.manifest(:Tool3,tt)

  tn = t1.find(:ToolNumber)

  measurments_t1 = t1.find(:Measurements)
  measurments_t1.manifest(:M1,mt)
  measurments_t1.manifest(:M2,mt)

  p tn.id

  run do
    sleep server.run
    tn.value = Time.now
    p tn.value

  end
end.loop!
