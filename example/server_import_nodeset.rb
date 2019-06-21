#!/usr/bin/ruby
#require 'opcua/server'
require_relative '../lib/opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new

    srv = opts['server']
    # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    srv.add_nodeset File.read('Opc.Ua.Di.1.2.NodeSet2.xml'), :DI                               # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    srv.add_nodeset File.read('Opc.Ua.AutoID.1.0.NodeSet2.xml'), :AutoId, :DI                  # https://opcfoundation.org/UA/schemas/AutoID/1.0/Opc.Ua.AutoID.NodeSet2.xml
    srv.add_nodeset File.read('Example.Reference.1.0.NodeSet2.xml'), :Testing, :DI, :Robotics  # Really weird local testing nodeset with references to DI and Robotics

    # TODO: currently add your current namespace as the last or it will be overridden
    srv.add_namespace 'http://example.org/'

    puts "\n======================="
    puts UA::HasProperty.methods(false).map{ |name| "#{name}:   \t#{UA::HasProperty.send(name)}" }
    
    puts "\n======================="
    puts srv.find(0, 13, OPCUA::NodeIdType::Numeric)
    puts srv.find_node(UA::HasProperty)

    tt = opts['server'].types.add_object_type(:TestType).tap{ |t|
      t.add_variable :TestValue
    }
    puts srv.find(0, 2253, 0).manifest :TestObject, tt

    find = [2253, 27, 15, 7777, 34, 355, 654, 42, 12, 324, 44, 67]
    for i in find
      if srv.find(0, i, 0).nil?
        puts "#{i} is nil"
      end
      if srv.find(0, i, 0)
        puts "#{i} exists"
      end
    end
    








    
  rescue => e
    puts e.message
  end

  counter = 0
  run do |opts|
    GC.start
    sleep opts['server'].run
    if counter > 20
      raise "exit"
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