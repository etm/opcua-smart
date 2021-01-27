#!/usr/bin/ruby
require_relative '../lib/opcua/server'
#require 'opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new
    opts['server'].add_namespace "https://centurio.work/kelch"

    t = opts['server'].types.add_object_type(:Test).tap{ |u|
      u.add_object(:Measurements, opts['server'].types.folder).tap{ |v|
        v.add_object(:BitRegister, opts['server'].types.folder).tap{ |w|
          w.add_variable_rw :Wert do |node,value,external|
            if external
              p node.id
              p value
            end
          end
        }
      }
    }

    v = opts['server'].objects.manifest(:Tester, t)

    opts['mm'] = v.find :Measurements
    opts['br'] = opts['mm'].find :BitRegister
    opts['value'] = opts['br'].find :Wert
  rescue => e
    puts e.message
  end

  run do |opts|
    opts['server'].run
    opts['value'].value = rand
  rescue => e
    puts e.message
  end

  on exit do
    # we could disconnect here, but OPCUA::Server does not have an explicit disconnect
    puts 'bye.'
  end
end.loop!
