require_relative 'client.so'

module OPCUA
  class Client
    alias_method :initialize_base, :initialize

    def initialize(url)
      if url =~ /(^[^:]+):\/\/([^:]+):([^@]+)@(.*)$/
        initialize_base("#{$1}://#{$4}",$2,$3)
      else
        initialize_base(url,nil,nil)
      end
    end
  end
end
