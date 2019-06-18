# OPC-UA Ruby Bindings (open62541)

The development of OPC UA applications takes currently a lot of effort. This is caused by the large possibilities of the OPC UA specification. With this implemtation we want to define some conventions, which shoud make the technology more useable.

## Table of Contents

1. [Modelling Style](#Modelling-Style)
2. [Installation](#Installation)
3. [Examples](#Examples)
    1. [Server](#Server)
        1. [Create Server and Namespace](#Create-Server-and-Namespace)
        2. [Create ObjectTypes](#Create-ObjectTypes)
        3. [Manifest Objects](#Manifest-Objects)
        4. [Find Nodes in the Addressspace](#Find-Nodes-in-the-Addressspace)
        5. [Loop for getting Real Life Data](#Loop-for-getting-Real-Life-Data)
    2. [Client](#Client)

## Modelling Style

The idea of the opcua-smart library is to simplify the OPC UA application generation. Since OPC UA has more than 1500 pages of basic specifications, and the number is still growing, we decided to make some simplification.

This is done by some constraints regarding the modeling functionality of OPC UA. This library deliberately does not offer all functions of OPC UA to simplify the creation of applications.

## COPYING

Copyright (C) 2019-* Jürgen "eTM" Mangler <juergen.mangler@gmail.com>. opcua-smart is freely distributable according to the terms of the GNU Lesser General Public License 3.0 (see the file 'COPYING'). This code is distributed without any warranty. See the file 'COPYING' for details.

## Installation

Dependency: https://github.com/open62541/open62541 > 0.4 (master branch as of 2019-04-26)

```sh
git clone https://github.com/open62541/open62541.git
cd open62541
mkdir build
cd build
cmake ..
ccmake ..
# Configuration, see picture below
make
sudo make install
gem install opcua
```

![ccmake Config](config.png)

If the installation works correctly, but examples are still complaining about missing lib62541.so, try this:

```sh
sudo echo "/usr/local/lib" > /etc/ld.so.conf.d/local.conf # add to libs path
sudo ldconfig # update libs
sudo ldconfig -p | grep libopen62541 # check if its there
```

## EXAMPLES

### Server

The server has following steps:
* Create the server and add_namespace
* Create ObjectTypes
* Manifest ObjectTypes
* Find nodes in the adress space
* Loop for getting real life data

#### Create Server and Namespace

```ruby
server = OPCUA::Server.new
server.add_namespace "https://yourdomain/testserver"
```


#### Create ObjectTypes 

Basically all new created types are subtypes of the _BaseObjectType_. With ```server.types.add_object_type(:TestObjectType)``` a new type is defined in the information model. All nodes of the new created type are defined in the ```tap{}``` region.

```ruby
to = server.types.add_object_type(:TestObjectType).tap{ |t|
  t.add_variable :TestVariable
  t.add_object(:TestObject, server.types.folder).tap{ |u|
    u.add_object :M, mt, OPCUA::OPTIONAL
  }
  t.add_method :TestMethod, inputarg1: OPCUA::TYPES::STRING, inputarg2: OPCUA::TYPES::DATETIME do |node, inputarg1, inputarg2|
    #do some stuff here
  end
}
```
In this example the _TestObjectType_ is defined. It consits of _TestVariable_ of the _BaseVariableType_ an _TestObject_ of the _FolderType_ and a _TestMethod_.

The ``` .add_variable :TestVariable ``` command adds a variable with the name _TestVariable_.

With ```.add_object(:TestObject)``` a new object named _TestObject_ is added. The second parameter is optional and definies of which type the new object is. Default the object is from _BaseObjectType_. In this example the created object is from _FolderType_. All child nodes of the object can be definded in the ```tap{}``` area.

Methods are added with the ```.add_method(:TestMethod)``` function. Per default the method has no input and output arguments. By adding additional arguments you can define input arguments. The code for defining a method with input arguments looks like 
```ruby
 t.add_method :TestMethod, inputarg1: OPCUA::TYPES::STRING, inputarg2: OPCUA::TYPES::DATETIME do |node, inputarg1, inputarg2|
    #do some stuff here
   end
```
Input arguments can have a name and a type.
in the ```do...end```section you write the code which should be executed by calling the method.

#### Manifest Objects

ObjectTypes can be instiantiated with the ```.manifest``` method. 

```ruby
    testobject =server.objects.manifest(:TestObjectType, to)
```

#### Find Nodes in the Addressspace

To get a specific node u should use th ```.find``` method. 
```ruby
tv = to.find(:TestVariable)
```
_tv_ is now the _TestVariable_ node.

#### Loop for getting Real Life Data
The server loop looks like follows:
```ruby
 run do 
    sleep server.run
    to.value = 'Testvariable1'
    p to.value
 rescue => e
    puts e.message
 end
```

The loop starts with ```sleep server.run```. This is recommended by the open62541 developer. With the ```.value``` function you can write or get the value of a node. 

### Client
TBD. See examples subdirectory.
