if File.exists?(File.join(__dir__,'../../ext/opcua/server/server.so'))
  require_relative  '../../ext/opcua/server/server.so'
else
  require_relative 'server.so'
end
require_relative 'server_classes.rb'
require_relative 'nodeset.rb'
require 'daemonite'

module OPCUA
  class Server
    def nodes
      if @nodes.nil?
        @nodes = Hash.new
      end
      @nodes
    end

    def import_ua
      add_nodeset File.read(File.join(File.dirname(__FILE__), "Opc.Ua.tiny.NodeSet2.xml"))
    end
    
    def import_ua_full
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

    class Node
      def follow(reference_type)
        follow_reference(reference_type, 0)
      end
      def follow_inverse(reference_type)
        follow_reference(reference_type, 1)
      end
      def follow_all(reference_type)
        follow_reference(reference_type, 2)
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
      NodeSet::Importer.new(self, nodeset, *namespace_indices).import
    end
  end
end
