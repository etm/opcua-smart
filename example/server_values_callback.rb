#!/usr/bin/ruby
require_relative '../lib/opcua/server'
#require 'opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new
    opts['server'].add_namespace "https://centurio.work/kelch"

    t = opts['server'].types.add_object_type(:Test).tap{ |t|
      t.add_variable_rw :Wert do |node,value,external|
        p node.id
        p value
        p external
      end
    }

    v = opts['server'].objects.manifest(:Tester, t)

    opts['value'] = v.find :Wert
  rescue => e
    puts e.message
  end

  run do |opts|
    opts['server'].run
    opts['value'].value = rand
    sleep 1
  rescue => e
    puts e.message
  end

  on exit do
    # we could disconnect here, but OPCUA::Server does not have an explicit disconnect
    puts 'bye.'
  end
end.loop!
