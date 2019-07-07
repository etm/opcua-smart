if File.exists?(File.join(__dir__,'../../ext/opcua/client/client.so'))
  require_relative  '../../ext/opcua/client/client.so'
else
  require_relative 'client.so'
end
require 'daemonite'

module OPCUA
  class Client
    alias_method :initialize_base, :initialize
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

    def initialize(url)
      if url =~ /(^[^:]+):\/\/([^:]+):([^@]+)@(.*)$/
        initialize_base("#{$1}://#{$4}",$2,$3)
      else
        initialize_base(url,nil,nil)
      end
    end
  end
end
