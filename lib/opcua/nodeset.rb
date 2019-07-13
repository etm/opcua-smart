require 'xml/smart'

module NodeSet
  class Importer
    def initialize(server, nodeset_xml, *namespace_indices)
      nodeset = XML::Smart.string(nodeset_xml)
      namespace_indices.unshift(:UA)

      local_namespaces = ["http://opcfoundation.org/UA/"]
      for i in nodeset.find("//*[name()='NamespaceUris']/*[name()='Uri']") do
        ns = i.find("text()").first.to_s
        local_namespaces.push(ns)
        unless server.namespaces.include? ns
          server.add_namespace ns
        end
      end
      
      aliases = Hash.new # get Aliases from Nodeset and use like: aliases['HasSubtype'] ... = i=45
      nodeset.find("//*[name()='Aliases']/*[name()='Alias']").each do |x|
        aliases[x.find("@Alias").first.to_s] = x.find("text()").first.to_s
      end

      @server = server
      @nodeset = nodeset
      @namespace_indices = namespace_indices
      @local_namespaces = local_namespaces
      @aliases = aliases
    end

    def server() return @server end
    def nodeset() return @nodeset end
    def namespace_indices() return @namespace_indices end
    def local_namespaces() return @local_namespaces end
    def aliases() return @aliases end

    def import
      nodeset.find("//*[name()='UAReferenceType' and @NodeId='i=45']").each do |x|
        t = create_from_nodeset(x) # create HasSubtype Reference First
      end

      nodeset.find("//*[name()='UAReferenceType']").each do |x|
        t = create_from_nodeset(x) # create all ReferenceTypes
      end

      nodeset.find("//*[name()='UADataType']").each do |x|
        t = create_from_nodeset(x) # create DataTypes
        # TODO: not completely implemented yet -> a lot of work to create actual structure dynamically in c
        # Definition, Fields, etc.
      end

      nodeset.find("//*[name()='UAVariableType']").each do |x|
        t = create_from_nodeset(x) # create VariableTypes
      end

      nodeset.find("//*[name()='UAObjectType']").each do |x|
        t = create_from_nodeset(x) # create ObjectTypes
      end
      
      # TODO: create all References of VariableTypes

      # TODO: create all References of ObjectTypes
      nodeset.find("//*[name()='UAObjectType']").each do |x|
        x.find("*[name()='References']/*[name()='Reference']").each do |r|
          reference_type = r.find("@ReferenceType").first
          is_forward = r.find("@IsForward").first
          reference_nodeid = NodeId.from_string(r.find("text()").first.to_s)
        end
      end

      nodeset.find("//*[name()='UAObject']").each do |x|
        # t = create_from_nodeset(x)
        # TODO
      end

      nodeset.find("//*[name()='UAMethod']").each do |x|
      end

      nodeset.find("//*[name()='UAVariable']").each do |x|
      end
    end

    def nodeid_from_nodeset(string_nodeid)
      unless string_nodeid.include? '='
        string_nodeid = aliases[string_nodeid]
      end
      local_nodeid = NodeId.from_string(string_nodeid)
      NodeId.new(server.namespaces.index(local_namespaces[local_nodeid.ns]), local_nodeid.id, local_nodeid.type)
    end

    def create_from_nodeset(xml)
      local_nodeid = NodeId.from_string(xml.find("string(@NodeId)"))
      namespace_index = namespace_indices[local_nodeid.ns]
      namespace = local_namespaces[local_nodeid.ns]
      nodeid = NodeId.new(server.namespaces.index(local_namespaces[local_nodeid.ns]), local_nodeid.id, local_nodeid.type)
      name = LocalizedText.parse(xml.find("*[name()='DisplayName']").first).text
      description = LocalizedText.parse xml.find("*[name()='Description']").first
      nodeclass = NodeClass.const_get(xml.find("name()")[2..-1])
      if xml.find("@ParentNodeId").first
        parent_nodeid_str = xml.find("string(@ParentNodeId)")
        parent_local_nodeid = NodeId.from_string(parent_nodeid_str)
        parent_nodeid = NodeId.new(server.namespaces.index(local_namespaces[parent_local_nodeid.ns]), parent_local_nodeid.id, parent_local_nodeid.type)
      elsif xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first
        parent_nodeid_str = xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first.to_s
        parent_local_nodeid = NodeId.from_string(parent_nodeid_str)
        parent_nodeid = NodeId.new(server.namespaces.index(local_namespaces[parent_local_nodeid.ns]), parent_local_nodeid.id, parent_local_nodeid.type)
      end

      node = server.get(nodeid.to_s)
      if node.nil?
        parent_node = server.get(parent_nodeid.to_s)
        unless parent_node.nil?
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
            reference_nodeid_str = xml.find("//*[text()='#{parent_nodeid_str}' and @IsForward='false']").first.find("string(@ReferenceType)")
            reference_nodeid = nodeid_from_nodeset(reference_nodeid_str, server, aliases, local_namespaces)
            reference_node = server.nodes[reference_nodeid.to_s]
            type_nodeid_str = xml.find("string(//*[name()='Reference' and @ReferenceType='HasTypeDefinition']/text())")
            type_nodeid = nodeid_from_nodeset(type_nodeid_str, server, aliases, local_namespaces)
            type_node = server.nodes[type_nodeid.to_s]
            node = server.add_object(name, nodeid.to_s, parent_node, reference_node, type_node)
            if xml.find("@SymbolicName").first
              sname = xml.find("@SymbolicName").first.to_s
            end
          else
            return nil
          end
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
        unless name[0] =~ /[A-Za-z]/
          name = "T_" + name
        end
        unless mod.const_defined?(name)
          mod.const_set(name, node)
          server.nodes[nodeid.to_s] = node
        end
      end
    end
  end
end