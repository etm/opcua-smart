#!/usr/bin/ruby
# require 'opcua/server'
require_relative '../lib/opcua/server'

Daemonite.new do
  on startup do |opts|
    srv = opts['server'] = OPCUA::Server.new

    # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    srv.import_ua # imports standard OPC UA Namespace under UA module -> but takes some ms time to parse
    # this is equal to:
    # add_nodeset File.read("../lib/opcua/Opc.Ua.1.04.NodeSet2.xml")
    # in this way you could also load a newer version of the opc ua standard nodeset

    raise "UA::HasSubtype not found" unless UA::HasSubtype.to_s == "ns=0;i=45"
    raise "UA::LocalizedText not found" unless UA::LocalizedText.to_s == "ns=0;i=21"

    srv.add_nodeset File.read('Opc.Ua.Di.1.2.NodeSet2.xml'), :DI                               # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    srv.add_nodeset File.read('Opc.Ua.AutoID.1.0.NodeSet2.xml'), :AutoId, :DI                  # https://opcfoundation.org/UA/schemas/AutoID/1.0/Opc.Ua.AutoID.NodeSet2.xml
    srv.add_nodeset File.read('Example.Reference.1.0.NodeSet2.xml'), :Testing, :DI, :Robotics  # Really weird local testing nodeset with references to DI and Robotics


    # TODO: currently add your current namespace as the last or it will be overridden
    ex = srv.add_namespace 'http://example.org/' # TODO: add_namespace should return namespace index
    ex = 6
    
    raise "add_object_type Error" unless srv.add_object_type("TestType","ns=#{ex};i=77777", UA::BaseObjectType, UA::HasSubtype).to_s == "ns=#{ex};i=77777"
    raise "add_data_type Error" unless srv.add_data_type("TestDataType","ns=#{ex};i=77778", UA::Structure, UA::HasSubtype).to_s == "ns=#{ex};i=77778"
    raise "add_object Error" unless srv.add_object("TestDevice", "ns=#{ex};i=31243", srv.objects, UA::Organizes, AutoId::OpticalReaderDeviceType).to_s == "ns=#{ex};i=31243"

    tt = srv.add_object_type(:TestComponentType, "ns=#{ex};i=77900", DI::ComponentType, UA::HasSubtype).tap{ |t|
      t.add_variable :TestVar1
    }

    tt2 = srv.add_object_type(:TestComponent2Type, "ns=#{ex};i=77901", tt, UA::HasSubtype).tap{ |t|
      t.add_variable :TestVar2
      t.add_object :TestObject2, tt
    }

    srv.objects.manifest(:Test1, tt)
    srv.objects.manifest(:Test2, tt2)

    raise "DI import Error" unless DI::ComponentType.follow_inverse(UA::HasSubtype).first.name == "TopologyElementType"
    raise "DI DeviceType not found" unless DI::ComponentType.follow(UA::HasSubtype).select{ |n| n.name == "DeviceType" }.length == 1
    raise "UA HasChild references missing" unless UA::HasChild.follow_all(UA::HasSubtype).select{ |n| n.name == "Aggregates" || n.name == "HasSubtype" }.length == 2

    node = UA::HasComponent
    path = "Find SuperTypes of #{node}"
    until node.nil?
      path += "->#{node.name}"
      node = node.follow_inverse(UA::HasSubtype).first
    end
    puts path












    
  rescue => e
    puts "=====================ERROR====================="
    puts e.message
    puts e.backtrace
    puts "======================END======================"
  end

  counter = 0
  run do |opts|
    GC.start
    sleep opts['server'].run
    if counter % 40 == 0
      #raise "exit"
    end
    counter += 1
  rescue => e
    if e.message == "exit"
      puts "\n\n\-->exit"
      break
    end
  end
end.loop!

exit