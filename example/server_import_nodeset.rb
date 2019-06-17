#!/usr/bin/ruby
#require 'opcua/server'
require_relative '../lib/opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new

    # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    opts['server'].add_nodeset :DI, File.read('Opc.Ua.Di.1.2.NodeSet2.xml')                       # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    opts['server'].add_nodeset :Robotics, File.read('Opc.Ua.Robotics.1.0.NodeSet2.xml')           # https://opcfoundation.org/UA/schemas/Robotics/1.0/Opc.Ua.Robotics.NodeSet2.xml
    opts['server'].add_nodeset :Testing, File.read('Example.Reference.1.0.NodeSet2.xml')          # Really weird local testing nodeset
    
    # TODO: currently add your current namespace as the last or it will be overridden
    opts['server'].add_namespace 'http://example.org/'

    nss = opts['server'].namespaces[0]
    for i in 0..nss.length - 1
      puts "ns#{i}=#{nss[i]}"
    end
    # TODO: check ns1=urn:unconfigured:application

    # TEST:
    # puts DI::ObjectTypes::DeviceType.new

    # SAMPLE:
    axis1 = Robotics::AxisType.new(:Axis1)
    puts "An instance #{axis1} of type #{axis1.type_nodeid}"
    puts "An instance #{Testing::TestComponent.new(:Component1)}"
    puts "An instance #{DI::DeviceType.new(:Device1)}"

    # class method get nodeid of type node:
    puts "A type node #{DI::DeviceType} of type #{DI::DeviceType.nodeid}"
    puts "A type node #{Robotics::AxisType} of type #{Robotics::AxisType.nodeid}"











    
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
      puts "exit"
      break
    end
  end
end.loop!

exit