if File.exists?(File.join(__dir__,'../../ext/opcua/server/server.so'))
  require_relative  '../../ext/opcua/server/server.so'
else
  require_relative 'server.so'
end
require 'daemonite'

module OPCUA
  class Server
    alias_method :initialize_base, :initialize

    class TypesSubNode
      def add_variables(*item)
        item.each do { |e| add_variable e }
      end
      def add_variables_rw(*item)
        item.each do { |e| add_variable_rw e }
      end
    end
  end
end
