#!/usr/bin/ruby
require_relative '../lib/opcua/server'
#require 'opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new
    opts['server'].add_namespace "https://centurio.work/kelch"

    mt = opts['server'].types.add_object_type(:MeasurementType).tap{ |t|
      t.add_variable :SollWertX
      t.add_variable :SollWertY
      t.add_variable :SollWertZ
    }
    tt = opts['server'].types.add_object_type(:ToolType).tap{ |t|
      t.add_variable :SollWertX
      t.add_variable :SollWertY
      t.add_variable :SollWertZ
      t.add_variable_rw :ToolNumber
      t.add_variable :DuploNumber
      t.add_property :testValue1
      t.add_method :testMethod, test1: OPCUA::TYPES::STRING, test2: OPCUA::TYPES::DATETIME do |node, test1, test2|
        ns, nid = node.id
        puts '-' * 10
        p nid
        p test1
        p test2
        puts '-' * 10
      end
      t.add_object(:Measurements, opts['server'].types.folder).tap{ |u|
        u.add_object :M, mt, OPCUA::OPTIONAL
      }
    }
    pt = opts['server'].types.add_object_type(:PresetterType).tap{ |t|
      t.add_variable :ManufacturerName
      t.add_object(:Tools, opts['server'].types.folder).tap{ |u|
        u.add_object :Tool, tt, OPCUA::OPTIONAL
      }
    }

    tools = opts['server'].objects.manifest(:KalimatC34, pt).find(:Tools)

    t1 = tools.manifest(:Tool1,tt)
    t2 = tools.manifest(:Tool2,tt)
    t3 = tools.manifest(:Tool3,tt)

    opts[:tn] = t1.find(:ToolNumber)
    opts[:tn].description = 'test test'
    opts[:tn].value = [0,1]
    p opts[:tn].description

    measurments_t1 = t1.find(:Measurements)
    measurments_t1.manifest(:M1,mt)
    measurments_t1.manifest(:M2,mt)

    p opts['server'].namespaces
  rescue => e
    puts e.message
  end


  counter = 0
  run do |opts|
    GC.start
    sleep opts['server'].run
    # if counter % 100 == 0
    #   opts[:tn].value = [counter, counter]
    #   # opts[:tn].value = 1
    #   p opts[:tn].value
    # end
    # counter += 1
  rescue => e
    puts e.message
  end

  on exit do
    # we could disconnect here, but OPCUA::Server does not have an explicit disconnect
    puts 'bye.'
  end
end.loop!
