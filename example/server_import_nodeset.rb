#!/usr/bin/ruby
# require 'opcua/server'
require_relative '../lib/opcua/server'
require 'xml/smart' # TEST

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new

    srv = opts['server']

=begin
    # TEST
    ex = opts['server'].add_namespace 'http://test.org/'
    xml = XML::Smart.string(File.read("../lib/opcua/Opc.Ua.1.04.NodeSet2.xml"))
    nodeids = xml.find("//*[name()='UAObject']/@NodeId").map{ |x| x.to_s }
    sampled = Hash.new
    not_found = Hash.new
    not_same = Hash.new
    for x in 0..1000000
      rnd_nid = nodeids.sample
      found_node = opts['server'].find_nodeid(rnd_nid)
      sampled[rnd_nid] = found_node;
      if found_node.nil?
        not_found[rnd_nid] = found_node;
      elsif(!"ns=0;#{rnd_nid}".eql?(found_node.to_s))
        not_same[rnd_nid] = found_node;
      else
        #puts found_node;
        #found_node.description = "#{found_node.description}:#{x}"
      end
    end

    puts "Sampled\t: #{sampled.length} of #{nodeids.length}"
    puts "Not found\t: #{not_found.length}"
    puts "Not the same\t: #{not_same.length}"
    not_same.each do |id|
      puts id
    end
=end

    # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    srv.import_ua # imports standard OPC UA Namespace under UA module -> but takes some time to parse
    # puts "UA::HasSubtype NodeId: #{UA::HasSubtype}"
    # puts "Found UA::HasSubtype = #{!srv.find_nodeid(UA::HasSubtype).nil?}";

    srv.add_nodeset File.read('Opc.Ua.Di.1.2.NodeSet2.xml'), :DI                               # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    srv.add_nodeset File.read('Opc.Ua.AutoID.1.0.NodeSet2.xml'), :AutoId, :DI                  # https://opcfoundation.org/UA/schemas/AutoID/1.0/Opc.Ua.AutoID.NodeSet2.xml
    srv.add_nodeset File.read('Example.Reference.1.0.NodeSet2.xml'), :Testing, :DI, :Robotics  # Really weird local testing nodeset with references to DI and Robotics

=begin
    # TODO: currently add your current namespace as the last or it will be overridden
    srv.add_namespace 'http://example.org/'

    ex = 6

    puts "TestType.nil? = #{srv.find_nodeid("ns=#{ex};i=77777").nil?}";
    srv.add_type("TestType","ns=#{ex};i=77777", UA::BaseObjectType, UA::HasSubtype, NodeClass::ObjectType, "");
    puts "TestType.nil? = #{srv.find_nodeid("ns=#{ex};i=77777").nil?}";
=end








    
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