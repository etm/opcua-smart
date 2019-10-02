#!/usr/bin/ruby
require_relative '../lib/opcua/server'
#require 'opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['count'] = 500
    opts['server'] = OPCUA::Server.new
    opts['server'].add_namespace "https://centurio.work/kelch"

    t = opts['server'].types.add_object_type(:Test).tap{ |t|
      0.upto opts['count'] do |e|
        t.add_variable :"Wert#{e}"
      end
    }

    opts['values'] = []
    v = opts['server'].objects.manifest(:Tester, t)
    0.upto opts['count'] do |e|
      opts['values'][e] = v.find :"Wert#{e}"
      opts['values'][e].value = rand
    end
  rescue => e
    puts e.message
  end

  run do |opts|
    opts['server'].run
    opts['values'].each do |v|
      v.value = rand
    end
  rescue => e
    puts e.message
  end

  on exit do
    # we could disconnect here, but OPCUA::Server does not have an explicit disconnect
    puts 'bye.'
  end
end.loop!
