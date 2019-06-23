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
        c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
      end

      doc.find("//*[name()='UAVariableType']").each do |x|
        c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
      end

      doc.find("//*[name()='UAObjectType']").each do |x|
        c = BaseNode.from_xml(self, x, namespace_indices, local_nss)
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
      c = BaseNode.from_xml(self, xml, namespace_indices, local_namespaces)
      if find_nodeid(c.NodeId).nil?
        parent_nodeid = xml.find("*[name()='References']/*[name()='Reference' and @ReferenceType='HasSubtype' and @IsForward='false']/text()").first
        parent_xml = xml.find("/*[@NodeId='ns=1;i=6031']").first
        if parent_xml
          parent = create_with_parents(server, parent_xml, namespace_indices, local_namespaces)
        end
        # create by type...
      end
      return c
    end
  end
end
