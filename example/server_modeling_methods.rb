#!/usr/bin/ruby
require 'opcua/server'

Daemonite.new do
  on startup do |opts|
    opts['server'] = OPCUA::Server.new

     # read nodesets from xml and load into server
    # also create classes for objects, types, variables and put nodeid in class variables
    opts['server'].add_nodeset :DI, File.read('Opc.Ua.Di.1.2.NodeSet2.xml')                       # https://opcfoundation.org/UA/schemas/DI/1.2/Opc.Ua.Di.NodeSet2.xml
    opts['server'].add_nodeset :Robotics, File.read('Opc.Ua.Robotics.1.0.NodeSet2.xml')           # https://opcfoundation.org/UA/schemas/Robotics/1.0/Opc.Ua.Robotics.NodeSet2.xml

    opts['server'].add_namespace :Example 'http://example.org/'

    # partial address space after loading the nodesets
=begin
    -> UA:Root
      -> UA:Objects::UA:FolderType
        -> DI:DeviceSet::UA:BaseObjectType
        -> UA:Server::UA:ServerType
      -> UA:Types::UA:FolderType
        -> UA:DataTypes::UA:FolderType
        -> UA:EventTypes::UA:FolderType
        -> UA:ReferenceTypes::UA:FolderType
        -> UA:VariableTypes::UA:FolderType
        -> UA:ObjectTypes::UA:FolderType
          -> UA:BaseObjectType
            +> DI:TopologyElementType
              +> DI:ComponentType
                +> DI:DeviceType
                +> Robotics:MotionDeviceSystemType
                  -> Robotics:Controllers::UA:FolderType
                    -> Robotics:<ControllerIdentifier>::Robotics:ControllerType
                  -> Robotics:MotionDevices::UA:FolderType
                    -> Robotics:<MotionDeviceIdentifier>::Robotics:MotionDeviceType
                +> Robotics:MotionDeviceType
                  -> Robotics:Axes::UA:FolderType
                  -> Robotics:FlangeLoad::Robotics:LoadType
                  - Robotics:Manufacturer::UA:PropertyType
                +> Robotics:ControllerType
                  -> Robotics:Components::UA:FolderType
                  - Robotics:Manufacturer::UA:PropertyType
=end


    # add new referencetype with add_type(new, parent, referencetype): HasExampleSubType < HasSubType
    example_hasexamplesubtype = opts['server'].types.add_type(:HasExampleSubType, UA::HasSubType)


    # add a new type with that reference (referencetype defaults to UA::HasSubType, but we want Examples::HasExampleSubType)
    example_somedevice_type = opts['server'].types.add_type(:SomeDeviceType, DI::DeviceType, example_hasexamplesubtype).tap{ |t|
      t.add_object :SomeComponent
      t.add DI::ComponentType.new(:SomeComponent2)
      t.add_variable :SomeProperty
      t.add_variable UA::PropertyType.new(:SomeProperty2)  # add custom variabletype
      
      # add new node with custom reference
      t.add_reference(UA::HasChild).modify(UA::PropertyType.new(:SomeProperty3)) # with curstom reference
      # is equivalent to:
      blank_node = t.add_reference(UA::HasChild) # initiates a blank node (perhaps not the best idea, because it could be unnecessary)
      component = blank_node.modify(UA::PropertyType.new(:SomeProperty4)) # instantiate a Property Node
      # modify first calls .delete_recursive() which deletes all references and recursively all forward hierarchical referenced nodes,
      # then creates a new node
    }


    # representation in types folder
=begin
    -> UA:Root
      -> UA:Types::UA:FolderType
        -> UA:ObjectTypes::UA:FolderType
          -> UA:BaseObjectType
            +> DI:TopologyElementType
              +> DI:ComponentType
                +> DI:DeviceType
                  +> Example:SomeDeviceType
                    -> Example:SomeComponent::UA:BaseObjectType
                    -> Example:SomeComponent2::DI:ComponentType
                    - Example:SomeProperty::UA:BaseVariableType
                    - Example:SomeProperty2::UA:PropertyType
                    - Example:SomeProperty3::UA:PropertyType
                    - Example:SomeProperty4::UA:PropertyType
=end


    # find DeviceSet from Objects in DI Companion Specification (root->objects->DeviceSet)
    deviceset = pts['server'].objects.find(:DeviceSet)

    
    # add 'SomeDevice' as instance of 'SomeDeviceType' to Server.Objects.DeviceSet
    example_somedevice = deviceset.manifest(:SomeDevice, example_device_type)
    # or as everything could become a class at runtime
    example_somedevice2 = deviceset.manifest(Example::SomeDeviceType.new(:SomeDevice2))
    # is somehow equivalent to (but perhaps it doen't make sense to create an unnecessary blank node):
    example_somedevice3 = deviceset.add_reference(UA::HasComponent).modify(Example::SomeDeviceType.new(:SomeDevice3))
    # better
    example_somedevice4 = deviceset.link(UA::HasComponent, Example::SomeDeviceType.new(:SomeDevice3))

    # set property value
    example_somedevice.find(:Manufacturer).value = 'ExampleManufacturer'
    # or if example_device isn't just a simple node, but the instance of Example::SomeDeviceType
    # but I'm not sure if it makes sense at this point
    example_somedevice.Manufacturer.value = 'ExampleManufacturer'
    # but you could instead create instances of these classes like:
    example_somedevice5 = Example::SomeDeviceType.new(:SomeDevice5)
    example_somedevice5.Manufacturer.value = 'ExampleManufacturer'
    deviceset.manifest(example_someDevice5)

    # or instead of a create function use bind
    # example_somedevice3 = deviceset.manifest(:SomeDevice3).bind(somedevice3)
    # somedevice3_component = DI::ComponentType.new
    # example_somedevice3_component = deviceset.find(:SomeComponent).bind(somedevice3_component)

    # create a link (reference) with a custom reference between two nodes
    deviceset.link(example_somedevice) # creates a UA::HasComponent Reference from deviceset to example_somedevice
    # use link_inverse() to connect create a non-forward reference
    deviceset.link(UA::HasChild, example_somedevice)
    example_somedevice.link(UA::HasTypeDefinition, Example::SomeDeviceType)








    # instead of UA::HasChild
    # ReferenceTypes::UA::HasChild could also make sense

    # it is a question of grouping: should we start with the nodeset namespace or the nodeclass:
    # ObjectTypes::Example::SomeDeviceType
    # ObjectTypes::UA::BaseObjectType
    # or
    # Example::ObjectTypes::SomeDeviceType
    # UA::ObjectTypes::BaseObjectType
    # or simply just
    # Example::SomeDeviceType
    # UA::BaseObjectType
    # what is better?






    
  rescue => e
    puts e.message
  end

  counter = 0
  run do |opts|
    GC.start
    sleep opts['server'].run
    if counter % 100 == 0
      p 'changed'
    end
    counter += 1
  rescue => e
    puts e.message
  end

  on exit do
    # we could disconnect here, but UA::Server does not have an explicit disconnect
    puts 'bye.'
  end
end.loop!
