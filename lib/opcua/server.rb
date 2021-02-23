if File.exists?(File.join(__dir__,'../../ext/opcua/server/server.so'))
  require_relative  '../../ext/opcua/server/server.so'
else
  require_relative 'server.so'
end
require 'daemonite'

module OPCUA
  class Server
    alias_method :get_base, :get

    def get(*a)
      if a.length == 1 && a[0].to_s =~ /ns=(\d+);i=(.*)/
        get_base $1.to_i, $2.to_i
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
      def add_variables(*item,&blk)
        item.each do |e|
          if blk.nil?
            add_variable e
          else
            add_variable e, &blk
          end
        end
      end
      def add_variables_rw(*item,&blk)
        item.each do |e|
          if blk.nil?
            add_variable_rw e
          else
            add_variable_rw e, &blk
          end
        end
      end
    end
  end
end
