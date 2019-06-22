#!/usr/bin/ruby
#require 'opcua/server'
require_relative '../lib/opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new(true)

    srv = opts['server']
    # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    srv.add_nodeset File.read('Opc.Ua.Di.1.2.NodeSet2.xml'), :DI                               # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    srv.add_nodeset File.read('Opc.Ua.AutoID.1.0.NodeSet2.xml'), :AutoId, :DI                  # https://opcfoundation.org/UA/schemas/AutoID/1.0/Opc.Ua.AutoID.NodeSet2.xml
    srv.add_nodeset File.read('Example.Reference.1.0.NodeSet2.xml'), :Testing, :DI, :Robotics  # Really weird local testing nodeset with references to DI and Robotics

    # TODO: currently add your current namespace as the last or it will be overridden
    srv.add_namespace 'http://example.org/'

    puts srv.find(2, 1001, 0).nil?;
    srv.add_type("TopologyElementType","ns=2;i=1001", "i=58", "ObjectType");
    puts srv.find(2, 1001, 0).nil?;
    








    
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