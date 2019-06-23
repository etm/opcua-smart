#!/usr/bin/ruby
# require 'opcua/server'
require_relative '../lib/opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new

    srv = opts['server']

    # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    srv.import_ua # imports standard OPC UA Namespace under UA module -> but takes some ms time to parse
    # this is equal to:
    # add_nodeset File.read("../lib/opcua/Opc.Ua.1.04.NodeSet2.xml")
    # in this way you could also load a newer version of the opc ua standard nodeset


    puts "UA::HasSubtype NodeId: #{UA::HasSubtype}"
    puts "Found UA::HasSubtype = #{!srv.find_nodeid(UA::HasSubtype).nil?}";

    srv.add_nodeset File.read('Opc.Ua.Di.1.2.NodeSet2.xml'), :DI                               # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    srv.add_nodeset File.read('Opc.Ua.AutoID.1.0.NodeSet2.xml'), :AutoId, :DI                  # https://opcfoundation.org/UA/schemas/AutoID/1.0/Opc.Ua.AutoID.NodeSet2.xml
    srv.add_nodeset File.read('Example.Reference.1.0.NodeSet2.xml'), :Testing, :DI, :Robotics  # Really weird local testing nodeset with references to DI and Robotics


    # TODO: currently add your current namespace as the last or it will be overridden
    ex = srv.add_namespace 'http://example.org/' # TODO: add_namespace should return namespace index
    ex = 6

    puts "TestType.nil? = #{srv.find_nodeid("ns=#{ex};i=77777").nil?}";
    # just needed when adding nodeset
    srv.add_type("TestType","ns=#{ex};i=77777", UA::BaseObjectType, UA::HasSubtype, NodeClass::ObjectType, "");
    puts "TestType.nil? = #{srv.find_nodeid("ns=#{ex};i=77777").nil?}";









    
  rescue => e
    puts e.message
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