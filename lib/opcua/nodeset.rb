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

      raise "Need #{local_namespaces.length - 1} namespace indices for:\n#{local_namespaces[1,local_namespaces.length - 1].join("\n")}" unless local_namespaces.length == namespace_indices.length
      
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
      # split into creation of BaseNode from xml and creation of ua node:
      # possibly allow hierarchical sorting of BaseNodes in the future
      # until then provided NodeSets should have been sorted hierachically
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
      end

      nodeset.find("//*[name()='UAObject']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create Objects
      end

      nodeset.find("//*[name()='UAMethod']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create Methods
      end

      nodeset.find("//*[name()='UAVariable']").each do |x|
        bn = BaseNode.new(self, x)
        t = create_from_basenode(bn) # create Variables
      end

      # TODO: create all References of VariableTypes
      # TODO: create all References of ObjectTypes
      # TODO: create all References of Objects
      # TODO: create all References of Variables
    end

    def check_supertypes(node, supertype)
      snode = node
      until snode.nil?
        snode = snode.follow_inverse(UA::HasSubtype).first
        return true if snode.to_s == supertype.to_s
      end
      false
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
          reference_node = server.nodes[bn.ReferenceNodeId.to_s]
          type_node = server.nodes[bn.TypeNodeId.to_s] unless bn.TypeNodeId.nil?
          datatype_node = server.nodes[bn.DataType.to_s] unless bn.DataType.nil?
          case bn.NodeClass
          when NodeClass::ReferenceType
            node = server.add_reference_type(bn.Name, bn.NodeId.to_s, parent_node, reference_node, true) if bn.Symmetric
            node = server.add_reference_type(bn.Name, bn.NodeId.to_s, parent_node, reference_node, false) unless bn.Symmetric
            node.abstract = true if bn.Abstract
            node.inverse = bn.InverseName.text unless bn.InverseName.nil?
          when NodeClass::DataType
            node = server.add_data_type(bn.Name, bn.NodeId.to_s, parent_node, reference_node)
          when NodeClass::VariableType
            dimensions = bn.Dimensions
            dimensions = [] if dimensions.nil?
            node = server.add_variable_type(bn.Name, bn.NodeId.to_s, parent_node, reference_node, UA::BaseDataType, dimensions) if datatype_node.nil?
            node = server.add_variable_type(bn.Name, bn.NodeId.to_s, parent_node, reference_node, datatype_node, dimensions) unless datatype_node.nil?
            node.abstract = true if bn.Abstract
          when NodeClass::ObjectType
            node = server.add_object_type(bn.Name, bn.NodeId.to_s, parent_node, reference_node)
            node.abstract = true if bn.Abstract
          when NodeClass::Object
            node = server.add_object(bn.Name, bn.NodeId.to_s, parent_node, reference_node, UA::BaseObjectType) if type_node.nil?
            node = server.add_object(bn.Name, bn.NodeId.to_s, parent_node, reference_node, type_node) unless type_node.nil?
            node.notifier = bn.EventNotifier unless bn.EventNotifier.nil?
          when NodeClass::Variable
            datatype_node = type_node.datatype unless type_node.datatype.nil? if datatype_node.nil?
            puts "\e[31m#{bn.Name} DataType is nil\e[0m" if datatype_node.nil?
            node = server.add_variable(bn.Name, bn.NodeId.to_s, parent_node, reference_node, type_node)
            # TODO: Set Value (necessary for e.g. Method Arguments)
          when NodeClass::Method
            node = server.add_method(bn.Name, bn.NodeId.to_s, parent_node, reference_node)
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

        unless mod.const_defined?(name, false)
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
      def Index() @index end
      def References() @references end
      def ReferenceNodeId() @parent_reference_nodeid end
      def TypeNodeId() @type_nodeid end
      def Symmetric() @symmetric end
      def Abstract() @abstract end
      def InverseName() @inverse_name end
      def DataType() @datatype end
      def SymbolicName() @symbolic_name end
      def EventNotifier() @eventnotifier end
      def Interval() @interval end
      def Rank() @rank end
      def Dimensions() @dimensions end

      def initialize(importer, xml)
        @xml = xml

        local_nodeid = NodeId.from_string(xml.find("string(@NodeId)"))
        @index = importer.namespace_indices[local_nodeid.ns]
        # namespace = importer.local_namespaces[local_nodeid.ns]
        @nodeid = importer.nodeid_to_server(local_nodeid)
        @name = LocalizedText.parse(xml.find("*[name()='DisplayName']").first).text
        @description = LocalizedText.parse xml.find("*[name()='Description']").first
        @nodeclass = NodeClass.const_get(xml.find("name()")[2..-1])

        @references = []
        xml.find("*[name()='References']/*[name()='Reference']").each do |r|
          @references.push(Reference.new(importer, r))
        end

        @parent_nodeid = importer.nodeid_from_nodeset(xml.find('string(@ParentNodeId)')) if xml.find('@ParentNodeId').first
        parent_reference = @references.select { |r| r.Forward == false }.select { |r| r.NodeId.to_s == @parent_nodeid.to_s }.first
        parent_reference = @references.select { |r| r.Forward == false }.select { |r| r.TypeNodeId.to_s == "ns=0;i=45" }.first unless parent_reference
        unless parent_reference
          @references.select { |r| r.Forward == false }.each do |ref|
            parent_reference = ref if importer.check_supertypes(importer.server.nodes[ref.TypeNodeId.to_s], UA::HierarchicalReferences)
          end
        end
        raise "Not the correct parent found: #{@nodeid}" unless parent_reference.NodeId.to_s == @parent_nodeid.to_s if parent_reference && @parent_nodeid
        # puts "No parent found: #{@name} - #{@nodeid}" unless parent_reference
        @parent_reference_nodeid = parent_reference.TypeNodeId if parent_reference
        @parent_nodeid = parent_reference.NodeId if parent_reference

        type_reference = @references.select { |r| r.TypeNodeId.to_s == "ns=0;i=40" }.first
        @type_nodeid = type_reference.NodeId if type_reference

        @symmetric = false unless @symmetric = xml.find('boolean(@Symmetric)')
        @abstract = false unless @abstract = xml.find('boolean(@IsAbstract)')
        @datatype = importer.nodeid_from_nodeset(xml.find('string(@DataType)')) if xml.find('@DataType').first
        @symbolic_name = xml.find('string(@SymbolicName)') if xml.find('@SymbolicName').first
        @inverse_name = LocalizedText.parse xml.find("*[name()='InverseName']").first
        @eventnotifier = xml.find('number(@EventNotifier)').to_i if xml.find('@EventNotifier').first
        @interval = xml.find('number(@MinimumSamplingInterval)').to_i if xml.find('@MinimumSamplingInterval').first
        @rank = xml.find('number(@ValueRank)').to_i if xml.find('@ValueRank').first
        @dimensions = xml.find('string(@ArrayDimensions)').split(",").map { |s| s.to_i } if xml.find('@ArrayDimensions').first
        
        value = xml.find("*[name()='Value']/*").first
        if @nodeclass == NodeClass::Variable && value
          #if value =~ "uax:ListOfExtensionObject"
          #end
          # TODO: get Value
          # Format: ListOfextensionObject, uax:ListOfextensionObject, ListOfDouble,String, Double...
          puts "#{@name} = #{value.qname}"
        end
      end
    end

    class ExtensionObject
      def TypeNodeId() @type_nodeid end
      def initialize(importer, xml)
        @type_nodeid = importer.nodeid_from_nodeset(xml.find("*[name()='TypeId']"))
      end
    end

    class Reference
      def NodeId() @nodeid end
      def TypeNodeId() @type_nodeid end
      def Forward() @forward end
      def initialize(importer, xml)
        @type_nodeid = importer.nodeid_from_nodeset(xml.find('string(@ReferenceType)'))
        @forward = true
        @forward = false unless xml.find('@IsForward').first.nil?
        # BUG in XML::Smart ? puts "#{xml.find('boolean(@IsForward)')} - #{xml.find('@IsForward').first}"
        @nodeid = importer.nodeid_from_nodeset(xml.find('string(text())'))
      end
    end
  end
end