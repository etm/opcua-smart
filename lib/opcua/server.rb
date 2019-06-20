if File.exists?(File.join(__dir__,'../../ext/opcua/server/server.so'))
  require_relative  '../../ext/opcua/server/server.so'
else
  require_relative 'server.so'
end
require 'daemonite'
require 'xml/smart'

module OPCUA
  class Server
    #private :get_server_nodeid

    class ObjectNode
      alias_method :find_one, :find

      def find(*what)
        if what.length == 0
          nil
        elsif what.length == 1
          find_one what[0]
        else
          what.map{|e| find_one e}
        end
      end
    end

    class TypeSubNode
      def add_variables(*item)
        item.each { |e| add_variable e }
      end
      def add_variables_rw(*item)
        item.each { |e| add_variable_rw e }
      end
    end

    def add_nodeset(nodeset, *namespace_indices)
      doc = XML::Smart.string(nodeset)

      # get all used namespaces from nodeset nss[0] is always UA, nss[1] is mostly the the own ns and nss[2 + n] is all the required other nss
      namespace_indices.unshift("http://opcfoundation.org/UA/")
      local_nss = ["http://opcfoundation.org/UA/"]
      for i in doc.find("//*[name()='NamespaceUris']/*[name()='Uri']") do
        ns = i.find("text()").first.to_s
        local_nss.push(ns)
        if !namespaces[0].include? ns
          add_namespace ns
        end
      end
      
      aliases = Hash.new # get Aliases from Nodeset and use like: aliases['HasSubtype'] ... = i=45
      doc.find("//*[name()='Aliases']/*[name()='Alias']").each do |x|
        aliases[x.find("@Alias").first.to_s] = x.find("text()").first.to_s
      end

      doc.find("//*[name()='UAReferenceType']").each do |x|
        # c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
        symmetric = x.find("@Symmetric").first || false
        # TODO: Create ReferenceTypes on the Server
      end

      doc.find("//*[name()='UADataType']").each do |x|
        # c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
        # TODO: Find Structures and add to server
      end

      # TODO: Find all VariableTypes and create them on the Server
      # TODO: Find all Objects and create them on the Server

      # Find all ObjectTypes and create them on the Server
      doc.find("//*[name()='UAObjectType']").each do |x|
        c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
      end

      # TODO: create all References of ReferenceTypes
      # TODO: create all References of DataTypes
      # TODO: create all References of VariableTypes
      # TODO: create all References of Objects

      # TODO: create all References of ObjectTypes
      doc.find("//*[name()='UAObjectType']").each do |x|
        x.find("*[name()='References']/*[name()='Reference']").each do |r|
          reference_type = r.find("@ReferenceType").first
          is_forward = r.find("@IsForward").first
          reference_nodeid = NodeId.from_string(r.find("text()").first.to_s)
        end
      end
    end
  end

  class BaseNode
    def self.from_xml(server, xml, namespace_indices, local_namespaces)
      local_nodeid = NodeId.from_string(xml.find("@NodeId").first.to_s)
      namespace_index = namespace_indices[local_nodeid.ns]
      namespace = local_namespaces[local_nodeid.ns]
      nodeid = NodeId.new(server.namespaces[0].index(local_namespaces[local_nodeid.ns]), local_nodeid.id, local_nodeid.type)
      local_browsename = QualifiedName.from_string(xml.find("@BrowseName").first.to_s)
      browsename = QualifiedName.new(server.namespaces[0].index(local_namespaces[local_browsename.ns]), local_browsename.name)
      displayname = LocalizedText.parse xml.find("*[name()='DisplayName']").first
      description = LocalizedText.parse xml.find("*[name()='Description']").first

      # TODO: check NodeClass
      nodeclass = NodeClass::ObjectType

      if namespace_index != ""
        if(!Object.const_defined?(namespace_index))
          Object.const_set(namespace_index, Module.new)
        end
        basenode = Class.new(BaseNode)
        Object.const_set(browsename.name, basenode)
        m = Object.const_get(namespace_index)
        # basenode.include(m)
        m.const_set(browsename.name, basenode)
      else
        basenode = Class.new
        Object.const_set(browsename.name, basenode)
      end
      basenode.define_singleton_method(:nodeid, -> { return nodeid })
      basenode.define_singleton_method(:browsename, -> { return browsename })
      basenode.define_singleton_method(:displayname, -> { return displayname })
      basenode.define_singleton_method(:description, -> { return description })
      basenode.define_singleton_method(:nodeclass, -> { return nodeclass })
      basenode.define_singleton_method(:namespace, -> { return namespace })

      # TODO: provide NodeClass-specific methods next to create_class
      # TODO: read NodeClass-specific properties

      basenode
    end
    def self.nodeid() @@nodeid end
    def self.browsename() @@browsename end
    def self.displayname() @@displayname end
    def self.description() @@description end
    def self.nodeclass() @@nodeclass end
    def self.namespace() @@namespace end
  end

  class NodeId
    def ns() @ns end
    def id() @id end
    def type() @type end
    def to_s() "ns=#{ns};#{type}=#{id}" end
    def initialize(namespaceindex, identifier, identifiertype='s') 
      if !namespaceindex.is_a?(Integer) || namespaceindex < 0
        raise "Bad namespaceindex #{namespaceindex}" 
      end
      if !!(identifier =~ /\A[-+]?[0-9]+\z/) && identifier.to_i > 0
        identifier = identifier.to_i
        identifiertype = 'i'
      end
      @ns = namespaceindex
      @id = identifier
      @type = identifiertype
    end
    def self.from_string(nodeid)
      if nodeid.match /ns=(.*?);/
        ns = nodeid.match(/ns=(.*?);/)[1].to_i
        type = nodeid.match(/;(.)=/)[1]
        id = nodeid.match(/;.=(.*)/)[1]
      else
        ns = 0
        type = nodeid.match(/(.)=/)[1]
        id = nodeid.match(/.=(.*)/)[1]
      end
      NodeId.new(ns, id, type)
    end
  end

  class QualifiedName
    def ns() @ns end
    def name() @name end
    def to_s() "#{ns}:#{name}" end
    def initialize(namespaceindex, name) 
      if !namespaceindex.is_a?(Integer) || namespaceindex < 0
        raise "Bad namespaceindex #{namespaceindex}" 
      end
      @ns = namespaceindex
      @name = name
    end
    def self.from_string(qualifiedname)
      ns = qualifiedname.match(/^([^:]+):/)[1].to_i
      name = qualifiedname.match(/:(.*)/)[1]
      QualifiedName.new(ns, name)
    end
  end

  class LocalizedText
    def locale() @locale end
    def text() @text end
    def to_s()
      if locale == ""
        return text
      else
        return "#{locale}:#{text}"
      end
    end
    def initialize(text, locale="") 
      if !text.is_a?(String) && !text.is_a?(Symbol) || !locale.is_a?(String)
        raise "Bad LocalizedText #{text} - #{locale}" 
      end
      if text == ""
        return nil
      end
      @locale = locale
      @text = text
    end
    def self.from_string(localizedtext)
      locale = localizedtext.match(/^([^:]+):/)[1]
      text = localizedtext.match(/:(.*)/)[1]
      LocalizedText.new(text, locale)
    end
    def self.parse(xml_element)
      if(xml_element.nil?)
        return nil
      end
      name = xml_element.to_s || ""
      locale = xml_element.find("@Locale").first.to_s || ""
      LocalizedText.new(name, locale)
    end
  end

  ##
  # NodeClasses see https://documentation.unified-automation.com/uasdkhp/1.0.0/html/_l2_ua_node_classes.html
  class NodeClass
    Unspecified = 0
    Object = 1
    Variable = 2
    Method = 4
    ObjectType = 8
    VariableType = 16
    ReferenceType = 32
    DataType = 64
    View = 128
  end
end
