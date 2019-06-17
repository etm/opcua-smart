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

    # REMOVE:
=begin
    class NodeIdType
      NUMERIC='i'
      STRING='s'
      GUID='g'
      BYTESTRING='b'
    end
=end

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
      ##
      # Creates a +NodeId+ from a string
      #
      # +nodeid+:: String containing the NodeId
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
      ##
      # Creates a +QualifiedName+ from a string
      #
      # +qualifiedname+:: String containing the QualifiedName
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
      ##
      # Creates a +LocalizedText+ from a string
      #
      # +localizedtext+:: String containing the +LocalizedText+
      def self.from_string(localizedtext)
        locale = localizedtext.match(/^([^:]+):/)[1]
        text = localizedtext.match(/:(.*)/)[1]
        LocalizedText.new(text, locale)
      end
      ##
      # Parses a +LocalizedText+ from an xml element
      #
      # +xml_element+:: Xml element containing the +LocalizedText+
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
    # Transform  a local +NodeId+ from  a nodeset to a +NodeId+ on the server.
    # 
    # +nodeid+:: Local +NodeId+ in the nodeset
    # +local_nss+:: String array of the nodesets local namespaces.
    def get_server_nodeid(nodeid, local_nss) 
      NodeId.new(namespaces[0].index(local_nss[nodeid.ns]), nodeid.id, nodeid.type) # CHECK: also caching the server_nss could be faster and not always calling "namespaces" method
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

    ##
    # Create a class in a module dynamically.
    # 
    # +name+:: String BrowseName without namespaceindex.
    # +nodeid+:: +NodeId+ of the type node.
    # +mod+:: Optional string of the parent module
    def create_class(name, nodeid, mod = "")
      if mod != ""
        if(!Object.const_defined?(mod))
          Object.const_set(mod, Module.new)
        end
        basenode = Class.new(BaseNode)
        Object.const_set(name, basenode)
        m = Object.const_get(mod)
        # basenode.include(m)
        m.const_set(name, basenode)
      else
        basenode = Class.new
        Object.const_set(name, basenode)
      end
      basenode.define_singleton_method(:nodeid, -> { return nodeid })
      basenode
    end

    class BaseNode
      def self.nodeid() @@nodeid end
      def self.browsename() @@browsename end
      def self.displayname() @@displayname end
      def self.description() @@description end
      def self.nodeclass() @@nodeclass end
      def type_nodeid() self.class.nodeid end
      def type_browsename() self.class.browsename end
      def type_displayname() self.class.displayname end
      def type_description() self.class.description end
      def type_nodeclass() self.class.nodeclass end
      def nodeid() @nodeid end
      def nodeclass() @nodeclass end
      def browsename
        # TODO: if nil take from self.class.browsename and remove 'Type'
        @browsename
      end
      def displayname
        # TODO: if nil take from self.class.displayname and remove 'Type'
        @displayname
      end
      def description
        if(@description == "")
          return self.class.description
        end
        @description
      end
      def initialize(name, description = "")
        # TODO: create QualifiedName from name in the current namespace, not sure whats the best strategy right now
        # @browsename = QualifiedName.new(2, name)
        @displayname = LocalizedText.new(name) # no Locale seems better than 'en' right now
        @description = description
        # TODO: create nodeid within the server.objects.add(BaseNode) function (same problem as with QualifiedName)
        # TODO: add HasTypeDefinition to typeid -> can be done with server.add
      end
    end

    ##
    # Parse a nodeset, load it to the server and create classes dynamically.
    # 
    # +namespace+:: Symbol containing the module name for the nodeset classes.
    # +nodeset+:: String containing the nodeset.
    def add_nodeset(namespace, nodeset)
      doc = XML::Smart.string(nodeset)
      namespace_module = Object.const_set(namespace, Module.new)

      # get all used namespaces from nodeset nss[0] is always UA nss[1] is mostly the the own ns and nss[2 + n] is all the required other nss
      local_nss = ["http://opcfoundation.org/UA/"]
      for i in doc.find("//*[name()='NamespaceUris']/*[name()='Uri']") do
        ns = i.find("text()").first.to_s
        local_nss.push(ns)
        if !namespaces[0].include? ns
          add_namespace ns
        end
      end

      # get Aliases from Nodeset and use like: aliases['HasSubtype'] ... = i=45
      aliases = Hash.new
      doc.find("//*[name()='Aliases']/*[name()='Alias']").each do |x|
        aliases[x.find("@Alias").first.to_s] = x.find("text()").first.to_s
      end

      doc.find("//*[name()='UAReferenceType']").each do |x|
        nodeid = get_server_nodeid(NodeId.from_string(x.find("@NodeId").first.to_s), local_nss)
        browsename = QualifiedName.from_string(x.find("@BrowseName").first.to_s)
        symmetric = x.find("@Symmetric").first || false
        displayname = LocalizedText.parse x.find("*[name()='DisplayName']").first
        description = LocalizedText.parse x.find("*[name()='Description']").first
        # TODO: Create ReferenceTypes on the Server
        # EXAMPLE: 
        # node = add_node(nodeid, browsename.text, NodeClass::ReferenceType)
        # node.add_attribute(displayname, Attributes::DisplayName)
        # node.add_attribute(description, Attributes::Description)
        # ...
      end

      doc.find("//*[name()='UADataType']").each do |x|
        nodeid = get_server_nodeid(NodeId.from_string(x.find("@NodeId").first.to_s), local_nss)
        browsename = QualifiedName.from_string(x.find("@BrowseName").first.to_s)
        displayname = LocalizedText.parse x.find("*[name()='DisplayName']").first
        description = LocalizedText.parse x.find("*[name()='Description']").first
        # TODO: Find Structures and add to server
      end

      # TODO: Find all DataTypes and create them on the Server
      # TODO: Find all VariableTypes and create them on the Server
      # TODO: Find all Objects and create them on the Server

      # Find all ObjectTypes and create them on the Server
      doc.find("//*[name()='UAObjectType']").each do |x|
        nodeid = get_server_nodeid(NodeId.from_string(x.find("@NodeId").first.to_s), local_nss)
        browsename = QualifiedName.from_string(x.find("@BrowseName").first.to_s)
        displayname = LocalizedText.parse(x.find("*[name()='DisplayName']").first)
        description = LocalizedText.parse(x.find("*[name()='Description']").first)
        c = create_class(browsename.name, nodeid, namespace)
      end

      # TODO: create all References of ReferenceTypes
      # TODO: create all References of DataTypes
      # TODO: create all References of VariableTypes
      # TODO: create all References of Objects

      # TODO: create all References of ObjectTypes
      doc.find("//*[name()='UAObjectType']").each do |x|
        browsename = QualifiedName.from_string(x.find("@BrowseName").first.to_s)
        nodeid = get_server_nodeid(NodeId.from_string(x.find("@NodeId").first.to_s), local_nss)
        x.find("*[name()='References']/*[name()='Reference']").each do |r|
          reference_type = r.find("@ReferenceType").first
          is_forward = r.find("@IsForward").first
          reference_nodeid = NodeId.from_string(r.find("text()").first.to_s)
        end
      end
    end
  end
end
