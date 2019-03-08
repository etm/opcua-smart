#!/usr/bin/ruby
require_relative '../lib/opcua/server'

Daemonite.new do
  server = OPCUA::Server.new
  run do
    sleep server.run
  end
end.loop!
