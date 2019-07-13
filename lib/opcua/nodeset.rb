require 'xml/smart'

module NodeSet
  class Importer
    def server() return @server end
    def nodeset() return @nodeset end
    def namespace_indices() return @namespace_indices end
    def local_namespaces() return @local_namespaces end
    def aliases() return @aliases end

    def initialize(server, nodeset_xml, *namespace_indices)
      nodeset = XML::Smart.string(nodeset_xml)
      namespace_indices.unshift(:UA)

      local_namespaces = ["http://opcfoundation.org/UA/"]
      for i in nodeset.find("//*[name()='NamespaceUris']/*[name()='Uri']") do
        ns = i.find('string(text())')
        local_namespaces.push(ns)
        server.add_namespace ns unless server.namespaces.include? ns
      end
      
      aliases = Hash.new # get Aliases from Nodeset and use like: aliases['HasSubtype'] ... = i=45
      nodeset.find("//*[name()='Aliases']/*[name()='Alias']").each do |x|
        aliases[x.find("@Alias").first.to_s] = x.find('string(text())')
      end

      @server = server
      @nodeset = nodeset
      @namespace_indices = namespace_indices
      @local_namespaces = local_namespaces
      @aliases = aliases
    end

    def import
      nodeset.find("//*[name()='UAReferenceType' and @NodeId='i=45']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create HasSubtype Reference First
      end

      nodeset.find("//*[name()='UAReferenceType']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create ReferenceTypes
      end

      nodeset.find("//*[name()='UADataType']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create DataTypes
        # TODO: not completely implemented yet -> a lot of work to create actual structure dynamically in c
        # Definition, Fields, etc.
      end

      nodeset.find("//*[name()='UAVariableType']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create VariableTypes
      end

      nodeset.find("//*[name()='UAObjectType']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create ObjectTypes
        #puts bn.Name
        #bn.References.each do |r|
        #  puts "\t#{r.ReferenceNodeId}\t#{r.Forward}\t#{r.TypeNodeId}"
        #end
      end
      
      # TODO: create all References of VariableTypes
      # TODO: create all References of ObjectTypes

      nodeset.find("//*[name()='UAObject']").each do |x|
      end

      nodeset.find("//*[name()='UAMethod']").each do |x|
      end

      nodeset.find("//*[name()='UAVariable']").each do |x|
      end
    end

    def nodeid_from_nodeset(string_nodeid)
      string_nodeid = aliases[string_nodeid] unless string_nodeid.include? '='
      nodeid_to_server(NodeId.from_string(string_nodeid))
    end

    def nodeid_to_server(nodeid)
      NodeId.new(server.namespaces.index(local_namespaces[nodeid.ns]), nodeid.id, nodeid.type)
    end

    def create_from_basenode(bn)
      node = server.get(bn.NodeId.to_s)
      if node.nil?
        parent_node = server.get(bn.ParentNodeId.to_s)
        unless parent_node.nil?
          case bn.NodeClass
          when NodeClass::ReferenceType
            node = server.add_reference_type(bn.Name, bn.NodeId.to_s, parent_node, UA::HasSubtype, true) if bn.Symmetric
            node = server.add_reference_type(bn.Name, bn.NodeId.to_s, parent_node, UA::HasSubtype, false) unless bn.Symmetric
            node.abstract = true if bn.Abstract
          when NodeClass::DataType
            node = server.add_data_type(bn.Name, bn.NodeId.to_s, parent_node, UA::HasSubtype)
          when NodeClass::VariableType
            node = server.add_variable_type(bn.Name, bn.NodeId.to_s, parent_node, UA::HasSubtype)
            node.abstract = true if bn.Abstract
          when NodeClass::ObjectType
            node = server.add_object_type(bn.Name, bn.NodeId.to_s, parent_node, UA::HasSubtype)
            node.abstract = true if bn.Abstract
          when NodeClass::Object
            reference_nodeid_str = xml.find("//*[text()='#{parent_nodeid_str}' and @IsForward='false']").first.find("string(@ReferenceType)")
            reference_nodeid = nodeid_from_nodeset(reference_nodeid_str, server, aliases, local_namespaces)
            reference_node = server.nodes[reference_nodeid.to_s]
            type_nodeid_str = xml.find("string(//*[name()='Reference' and @ReferenceType='HasTypeDefinition']/text())")
            type_nodeid = nodeid_from_nodeset(type_nodeid_str, server, aliases, local_namespaces)
            type_node = server.nodes[type_nodeid.to_s]
            node = server.add_object(bn.Name, bn.NodeId.to_s, parent_node, reference_node, type_node)
          else
            return nil
          end
        end
      end

      if (bn.NodeClass == NodeClass::ReferenceType ||
          bn.NodeClass == NodeClass::DataType ||
          bn.NodeClass == NodeClass::VariableType ||
          bn.NodeClass == NodeClass::ObjectType)
        Object.const_set(bn.Index, Module.new) unless Object.const_defined?(bn.Index)
        mod = Object.const_get(bn.Index)
        name = bn.Name
        name = "T_" + bn.Name unless name[0] =~ /[A-Za-z]/
        unless mod.const_defined?(name)
          mod.const_set(name, node)
          server.nodes[bn.NodeId.to_s] = node
        end
      end
    end

    class BaseNode
      def NodeId() @nodeid end
      def Name() @name end
      def Description() @description end
      def NodeClass() @nodeclass end
      def ParentNodeId() @parent_nodeid end
      def Symmetric() @symmetric end
      def Abstract() @abstract end
      def DataType() @datatype end
      def SymbolicName() @symbolic_name end
      def EventNotifier() @eventnotifier end
      def Index() @index end
      def References() @references end

      def initialize(importer, xml)
        local_nodeid = NodeId.from_string(xml.find("string(@NodeId)"))
        @index = importer.namespace_indices[local_nodeid.ns]
        namespace = importer.local_namespaces[local_nodeid.ns]
        nodeid = importer.nodeid_to_server(local_nodeid)
        name = LocalizedText.parse(xml.find("*[name()='DisplayName']").first).text
        description = LocalizedText.parse xml.find("*[name()='Description']").first
        nodeclass = NodeClass.const_get(xml.find("name()")[2..-1])
        if xml.find("@ParentNodeId").first
          parent_nodeid_str = xml.find("string(@ParentNodeId)")
          parent_local_nodeid = NodeId.from_string(parent_nodeid_str)
          parent_nodeid = importer.nodeid_to_server(parent_local_nodeid)
        elsif xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first
          parent_nodeid_str = xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first.to_s
          parent_local_nodeid = NodeId.from_string(parent_nodeid_str)
          parent_nodeid = importer.nodeid_to_server(parent_local_nodeid)
        end

        @symmetric = false unless @symmetric = xml.find('boolean(@Symmetric)')
        @abstract = false unless @abstract = xml.find('boolean(@IsAbstract)')
        @datatype = importer.nodeid_from_nodeset(xml.find('string(@DataType)')) if xml.find('@DataType').first
        @symbolic_name = xml.find('string(@SymbolicName)') if xml.find('@SymbolicName').first
        @eventnotifier = xml.find('integer(@EventNotifier)') if xml.find('@EventNotifier').first

        @references = []
        xml.find("*[name()='References']/*[name()='Reference']").each do |r|
          @references.push(Reference.new(importer, r))
        end

        @name = name
        @nodeid = nodeid
        @parent_nodeid = parent_nodeid
        @description = description
        @nodeclass = nodeclass
        @xml = xml
      end
    end

    class Reference
      def ReferenceNodeId() @reference_nodeid end
      def TypeNodeId() @type_nodeid end
      def Forward() @forward end
      def initialize(importer, xml)
        @type_nodeid = importer.nodeid_from_nodeset(xml.find('string(@ReferenceType)'))
        @forward = false unless @forward = xml.find('boolean(@IsForward)')
        @reference_nodeid = importer.nodeid_from_nodeset(xml.find('string(text())'))
      end
    end
  end
end