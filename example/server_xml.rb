#!/usr/bin/ruby
require_relative '../lib/opcua/server'

Daemonite.new do
  on startup do |opts|
    srv = opts['server'] = OPCUA::Server.new
    srv.import_UA # FIX
    srv.import_nodeset File::read('nodeset1.xml')
    srv.import_nodeset File::read('nodeset1.xml')
    srv.import_nodeset File::read('nodeset1.xml')
    DI  = srv.imports['http://xxxxx/DI']
    AID = srv.imports['http://xxxxx/AutoID']

    srv.objects

    # ab 106 im nodesets.rb
    x = srv.data_types.add(qualifiedname,referencetype=UA::HasSubtype,nodeid=rand())
    x.displayname['en-US'] = 'rrr'
    x.displayname['de'] = 'rrr'
    x.displayname.delete('de')

    srv.data_types.bla.blurg.add(wieheissts')
    srv.data_types.bla.blurg.push(wieheissts')
    srv.data_types.bla.blurg.<<('wieheissts')
    x = srv.data_types.bla.find name: 'bla', type: SUBXXXX
    [x] = srv.data_types.bla.find_all name: 'bla', type: SUBXXXX
    [x] = srv.data_types.bla.select name: 'bla', type: SUBXXXX
    srv.data_types.bla.parent

    # srv.event_types
    # srv.interfaces_types

    srv.reference_types.add(qualifiedname,symmetric,referencetype,nodeid)
    srv.reference_types.add_symmetric(qualifiedname,referencetype,nodeid)

    srv.variable_types
    srv.object_types

    p server.add_namespace "https://centurio.work/kelch"
    p server.active_namespace
    server.active_namespace = 0
    p server.active_namespace
    p server.namespaces


    tools = opts['server'].objects.manifest(:KalimatC34, pt).find(:Tools)

    t1 = tools.manifest(:Tool1,tt)
    t2 = tools.manifest(:Tool2,tt)
    t3 = tools.manifest(:Tool3,tt)

    opts[:tn] = t1.find(:ToolNumber)
    opts[:tn].description = 'test test'
    opts[:tn].value = [0,1]
    p opts[:tn].description
    p opts[:tn].to_s
    p opts[:tn].name

    measurments_t1 = t1.find(:Measurements)
    measurments_t1.manifest(:M1,mt)
    m2 = measurments_t1.manifest(:M2,mt)
  rescue => e
    puts e.message
  end


  counter = 0
  run do |opts|
    GC.start
    sleep opts['server'].run
    # if counter % 100 == 0
    #   opts[:tn].value = [counter, counter]
    #   # opts[:tn].value = 1
    #   p opts[:tn].value
    # end
    # counter += 1
  rescue => e
    puts e.message
  end

  on exit do
    # we could disconnect here, but OPCUA::Server does not have an explicit disconnect
    puts 'bye.'
  end
end.loop!
