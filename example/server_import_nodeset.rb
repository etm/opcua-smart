#!/usr/bin/ruby
#require 'opcua/server'
require_relative '../lib/opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new

    # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    opts['server'].add_nodeset File.read('Opc.Ua.Di.1.2.NodeSet2.xml'), :DI                               # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    opts['server'].add_nodeset File.read('Opc.Ua.AutoID.1.0.NodeSet2.xml'), :AutoId, :DI                  # https://opcfoundation.org/UA/schemas/Robotics/1.0/Opc.Ua.Robotics.NodeSet2.xml
    opts['server'].add_nodeset File.read('Example.Reference.1.0.NodeSet2.xml'), :Testing, :DI, :Robotics  # Really weird local testing nodeset

    # TODO: currently add your current namespace as the last or it will be overridden
    opts['server'].add_namespace 'http://example.org/'

    puts "========================"
    nss = opts['server'].namespaces
    for i in 0..nss.length - 1
      puts "ns#{i}=#{nss[i]}"
    end

    puts "\n\n==========================String=========================="
    puts UA::HasProperty.methods(false).map{ |name| "#{name}:   \t#{UA::HasProperty.send(name)}" }
    puts "\n\n=======================HasProperty======================="
    puts UA::HasProperty.methods(false).map{ |name| "#{name}:   \t#{UA::HasProperty.send(name)}" }
    puts "\n\n=====================AutoIdDeviceType====================="
    puts AutoId::AutoIdDeviceType.methods(false).map{ |name| "#{name}:   \t#{AutoId::AutoIdDeviceType.send(name)}" }
    puts "\n\n========================DeviceType========================"
    puts DI::DeviceType.methods(false).map{ |name| "#{name}:   \t#{DI::DeviceType.send(name)}" }
    puts "\n\n========================ConnectsTo========================"
    puts DI::ConnectsTo.methods(false).map{ |name| "#{name}:   \t#{DI::ConnectsTo.send(name)}" }
    puts "\n\n=====================TestVariableType====================="
    puts Testing::TestVariableType.methods(false).map{ |name| "#{name}:   \t#{Testing::TestVariableType.send(name)}" }
    










    
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