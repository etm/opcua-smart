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
    def import_ua
      add_nodeset File.read(File.join(File.dirname(__FILE__), "Opc.Ua.1.04.NodeSet2.xml"))
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

      doc.find("//*[name()='UAReferenceType']").each do |x|
        create_with_parents(self, x, namespace_indices, local_nss)
      end

      doc.find("//*[name()='UADataType']").each do |x|
        create_with_parents(self, x, namespace_indices, local_nss)
        # c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
        # TODO: not implemented in c yet
      end

      doc.find("//*[name()='UAVariableType']").each do |x|
        c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
        # TODO: creates Errors, missing DataType
        # create_with_parents(self, x, namespace_indices, local_nss)
      end

      doc.find("//*[name()='UAObjectType']").each do |x|
        create_with_parents(self, x, namespace_indices, local_nss)
      end

      # TODO: just add without BaseNode
      doc.find("//*[name()='UAMethod']").each do |x|
      end

      # TODO: just add without BaseNode
      doc.find("//*[name()='UAVariable']").each do |x|
      end

      # TODO: just add without BaseNode
      doc.find("//*[name()='UAObject']").each do |x|
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

    def create_with_parents(server, xml, namespace_indices, local_namespaces)
      c = BaseNode.from_xml(server, xml, namespace_indices, local_namespaces)
      if(!c.nil? && server.find_nodeid(c.NodeId).nil?)# && c.NodeId.ns != 0)  # TODO: also add ns=0 because of some missing types in V1.00 --> V1.04
        parent_nodeid_str = xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first.to_s
        parent_xml = xml.find("/*[@NodeId='#{parent_nodeid_str}']").first
        if parent_xml # create parents before
          parent = create_with_parents(server, parent_xml, namespace_indices, local_namespaces)
          server.add_type(c.BrowseName.name, c, parent.NodeId, "i=45", c.NodeClass, "")
        elsif parent_nodeid_str.strip.empty?
          # it is a top node -> UA base Node -> do not add
        else
          local_parent_nodeid = NodeId.from_string(parent_nodeid_str)
          unless local_parent_nodeid.nil?
            parent_nodeid = NodeId.new(server.namespaces.index(local_namespaces[local_parent_nodeid.ns]), local_parent_nodeid.id, local_parent_nodeid.type)
            unless server.find_nodeid(parent_nodeid).nil? # only create if parent already exists
              server.add_type(c.BrowseName.name, c, parent_nodeid, "i=45", c.NodeClass, "")
            else
              if parent_nodeid.to_s.eql? "ns=0;i=22"
                server.add_type(c.BrowseName.name, c, parent_nodeid, "i=45", c.NodeClass, "")
              end
              # puts "not found: #{parent_nodeid}"
              # TODO: we assume the parent already exists if it is not defined within this nodeset
              # all non-existant parents will getinto this loop
            end
          end
        end
        # create by type...
      end
      return c
    end
  end
end
