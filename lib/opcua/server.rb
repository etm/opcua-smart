if File.exists?(File.join(__dir__,'../../ext/opcua/server/server.so'))
  require_relative  '../../ext/opcua/server/server.so'
else
  require_relative 'server.so'
end
require_relative 'server_classes.rb'
require 'daemonite'
require 'xml/smart'

module OPCUA
  class Server
    def nodes
      if @nodes.nil?
        @nodes = Hash.new
      end
      @nodes
    end
    def import_ua
      add_nodeset File.read(File.join(File.dirname(__FILE__), "Opc.Ua.1.04.NodeSet2.xml"))
    end

    alias_method :get_base, :get
    def get(*a)
      if a.length == 1 && a[0].to_s =~ /ns=(\d+);i=(.*)/
        get_base $1.to_i, $2.to_i
      elsif a.length == 1 && a[0].to_s =~ /i=(.*)/
        get_base 0, $2.to_i
      elsif a.length == 1 && a[0].to_s =~ /ns=(\d+);s=(.*)/
        get_base $1.to_i, $2
      else
        get_base *a
      end
    end

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
      def add_property(name)
        add_variable name, OPCUA::PROPERTYTYPE
      end
      def add_property_rw
        add_variable_rw name, OPCUA::PROPERTYTYPE
      end
      def add_properties(*item)
        item.each { |e| add_property e }
      end
      def add_property_rw(*item)
        item.each { |e| add_property_rw e }
      end
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
      namespace_indices.unshift(:UA)
      local_nss = ["http://opcfoundation.org/UA/"]
      for i in doc.find("//*[name()='NamespaceUris']/*[name()='Uri']") do
        ns = i.find("text()").first.to_s
        local_nss.push(ns)
        unless namespaces.include? ns
          add_namespace ns
        end
      end
      
      aliases = Hash.new # get Aliases from Nodeset and use like: aliases['HasSubtype'] ... = i=45
      doc.find("//*[name()='Aliases']/*[name()='Alias']").each do |x|
        aliases[x.find("@Alias").first.to_s] = x.find("text()").first.to_s
      end

      doc.find("//*[name()='UAReferenceType' and @NodeId='i=45']").each do |x|
        t = create_from_nodeset(self, x, namespace_indices, local_nss, aliases)
        #t = create_type_from_nodeset(self, x, namespace_indices, local_nss, aliases)
      end

      doc.find("//*[name()='UAReferenceType']").each do |x|
        t = create_from_nodeset(self, x, namespace_indices, local_nss, aliases)
      end

      doc.find("//*[name()='UADataType']").each do |x|
        t = create_from_nodeset(self, x, namespace_indices, local_nss, aliases)
        # TODO: not completely implemented yet -> a lot of work to create actual structure dynamically in c
      end

      doc.find("//*[name()='UAVariableType']").each do |x|
        t = create_from_nodeset(self, x, namespace_indices, local_nss, aliases)
      end

      doc.find("//*[name()='UAObjectType']").each do |x|
        t = create_from_nodeset(self, x, namespace_indices, local_nss, aliases)
      end

      doc.find("//*[name()='UAObject']").each do |x|
        # t = create_from_nodeset(self, x, namespace_indices, local_nss, aliases)
        # TODO
      end

      doc.find("//*[name()='UAMethod']").each do |x|
      end

      doc.find("//*[name()='UAVariable']").each do |x|
      end
      
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

    def create_from_nodeset(server, xml, namespace_indices, local_namespaces, aliases)
      local_nodeid = NodeId.from_string(xml.find("@NodeId").first.to_s)
      namespace_index = namespace_indices[local_nodeid.ns]
      namespace = local_namespaces[local_nodeid.ns]
      nodeid = NodeId.new(server.namespaces.index(local_namespaces[local_nodeid.ns]), local_nodeid.id, local_nodeid.type)
      name = LocalizedText.parse(xml.find("*[name()='DisplayName']").first).text
      description = LocalizedText.parse xml.find("*[name()='Description']").first
      nodeclass = NodeClass.const_get(xml.find("name()")[2..-1])
      if xml.find("@ParentNodeId").first
        parent_local_nodeid = NodeId.from_string(xml.find("@ParentNodeId").first.to_s)
        parent_nodeid = NodeId.new(server.namespaces[0].index(local_namespaces[parent_local_nodeid.ns]), parent_local_nodeid.id, parent_local_nodeid.type)
      elsif xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first
        parent_nodeid_str = xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first.to_s
        parent_local_nodeid = NodeId.from_string(parent_nodeid_str)
        parent_nodeid = NodeId.new(server.namespaces.index(local_namespaces[parent_local_nodeid.ns]), parent_local_nodeid.id, parent_local_nodeid.type)
      end

      node = server.get(nodeid.to_s)
      if node.nil?
        parent_node = server.get(parent_nodeid.to_s)
        case nodeclass
        when NodeClass::ReferenceType
          if xml.find('boolean(@Symmetric)')
            node = server.add_reference_type(name, nodeid.to_s,parent_node, UA::HasSubtype, true)
          else
            node = server.add_reference_type(name, nodeid.to_s,parent_node, UA::HasSubtype, false)
          end
          if xml.find('boolean(@IsAbstract)')
            node.abstract = true
          end
        when NodeClass::DataType
          node = server.add_data_type(name, nodeid.to_s,parent_node, UA::HasSubtype)
        when NodeClass::VariableType
          node = server.add_variable_type(name, nodeid.to_s,parent_node, UA::HasSubtype)
          if xml.find('boolean(@IsAbstract)')
            node.abstract = true
          end
        when NodeClass::ObjectType
          node = server.add_object_type(name, nodeid.to_s,parent_node, UA::HasSubtype)
          if xml.find('boolean(@IsAbstract)')
            node.abstract = true
          end
        when NodeClass::Object
          # TODO:
          # node = server.add_object_type(name, nodeid.to_s,parent_node, UA::HasSubtype)
          if xml.find("@SymbolicName").first
            sname = xml.find("@SymbolicName").first.to_s
          end
        else
          return nil
        end
      end

      if (nodeclass == NodeClass::ReferenceType ||
          nodeclass == NodeClass::DataType ||
          nodeclass == NodeClass::VariableType ||
          nodeclass == NodeClass::ObjectType)
        unless Object.const_defined?(namespace_index)
          Object.const_set(namespace_index, Module.new)
        end
        mod = Object.const_get(namespace_index)
        if name.start_with? '3'
          name = "T_#{name}"
        end
        unless mod.const_defined?(name)
          mod.const_set(name, node)
          server.nodes[nodeid.to_s] = node
        end
      end
    end
















    def create_type_from_nodeset(server, xml, namespace_indices, local_namespaces, aliases)
      c = BaseNode.from_xml(server, xml, namespace_indices, local_namespaces)
      if(!c.nil? && server.get(c.NodeId).nil?)# && c.NodeId.ns != 0)  # TODO: also add ns=0 because of some missing types in V1.00 --> V1.04
        parent_nodeid_str = xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first.to_s
        local_parent_nodeid = NodeId.from_string(parent_nodeid_str)
        unless local_parent_nodeid.nil?
          parent_nodeid = NodeId.new(server.namespaces.index(local_namespaces[local_parent_nodeid.ns]), local_parent_nodeid.id, local_parent_nodeid.type)
          unless server.get(parent_nodeid).nil? # only create if parent already exists
            if (c.NodeClass == NodeClass::ReferenceType)
              if xml.find('boolean(@Symmetric)')
                type = server.add_reference_type(c.BrowseName.name, c, parent_nodeid, "i=45", true)
              else
                type = server.add_reference_type(c.BrowseName.name, c, parent_nodeid, "i=45", false)
              end
              if xml.find('boolean(@IsAbstract)')
                type.abstract = true
              end
            elsif (c.NodeClass == NodeClass::DataType)
              type = server.add_data_type(c.BrowseName.name, c, parent_nodeid, "i=45")
            elsif (c.NodeClass == NodeClass::ObjectType)
              type = server.add_object_type(c.BrowseName.name, c, parent_nodeid, "i=45")
              if xml.find('boolean(@IsAbstract)')
                type.abstract = true
              end
            elsif (c.NodeClass == NodeClass::VariableType)
              type = server.add_variable_type(c.BrowseName.name, c, parent_nodeid, "i=45")
              if xml.find('boolean(@IsAbstract)')
                type.abstract = true
              end
            end
            # TODO: 
            # - VariableType & Variable: ValueRank="1" ArrayDimensions="0" DataType="i=868" or DataType="String" (use Alias)
            # - Object: SymbolicName="CPIdentifier"
          else
            warn "Nodeset - Parent #{parent_nodeid} not found"
          end
        end
        return nil
      end
      return nil
    end
  end
end
