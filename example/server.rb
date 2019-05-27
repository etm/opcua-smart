#!/usr/bin/ruby
require_relative '../lib/opcua/server'
#require 'opcua/server'

Daemonite.new do
  on startup do
    @opts['server'] = OPCUA::Server.new
    @opts['server'].add_namespace "https://centurio.work/kelch"

    mt = @opts['server'].types.add_object_type(:MeasurementType).tap{ |t|
      t.add_variable :SollWertX
      t.add_variable :SollWertY
      t.add_variable :SollWertZ
    }
    tt = @opts['server'].types.add_object_type(:ToolType).tap{ |t|
      t.add_variable :SollWertX
      t.add_variable :SollWertY
      t.add_variable :SollWertZ
      t.add_variable :ToolNumber
      t.add_variable :DuploNumber
      t.add_variable :testValue1
      t.add_method :testMethod, test1: OPCUA::TYPES::STRING, test2: OPCUA::TYPES::DATETIME do |node, test1, test2, test4|
        ns, nid = node.id
        p nid
        p test1
        p test2
        p test4
        puts 'me'
        # do something
      end
      t.add_object(:Measurements, @opts['server'].types.folder).tap{ |u|
        u.add_object :M, mt, OPCUA::OPTIONAL
      }
    }
    pt = @opts['server'].types.add_object_type(:PresetterType).tap{ |t|
      t.add_variable :ManufacturerName
      t.add_object(:Tools, @opts['server'].types.folder).tap{ |u|
        u.add_object :Tool, tt, OPCUA::OPTIONAL
      }
    }

    tools = @opts['server'].objects.manifest(:KalimatC34, pt).find(:Tools)

    t1 = tools.manifest(:Tool1,tt)
    t2 = tools.manifest(:Tool2,tt)
    t3 = tools.manifest(:Tool3,tt)

    @opts[:tn] = t1.find(:ToolNumber)

    measurments_t1 = t1.find(:Measurements)
    measurments_t1.manifest(:M1,mt)
    measurments_t1.manifest(:M2,mt)

  rescue => e
    puts e.message
  end

  run do
    GC.start
    sleep @opts['server'].run
    @opts[:tn].value = Time.now
  rescue => e
    puts e.message
  end

  on exit do
    # we could disconnect here, but OPCUA::Server does not have an explicit disconnect
    puts 'bye.'
  end
end.loop!
