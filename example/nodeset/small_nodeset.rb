require 'nokogiri'

def is_necessary(node)
  necessary = [
    "StateMachineType"
  ]
  find_follow(node)
  false
end

def find_follow(node)
  follow = [
    "HasComponent",
    "HasSubtype"
  ]
  node.xpath('xmlns:References/xmlns:Reference').each do |ref|
    node.xpath("parent::*/*[@NodeId='#{ref.content}']").each do |fnode|
      puts "lol"
    end if follow.include? ref['ReferenceType']
  end
  false
end

nodeset = Nokogiri::XML(File.read(File.join(File.dirname(__FILE__), "../../lib/opcua/Opc.Ua.1.04.NodeSet2.xml")))

nodeset.xpath("/*/*").each do |node|
  unless node.name == 'UAReferenceType'
    if node['NodeId'] =~ /i=(.*)/ && $1.to_i > 1000
      node.remove unless is_necessary node
    end
  end
end

File.write(File.join(File.dirname(__FILE__), "../../lib/opcua/Opc.Ua.thin.NodeSet2.xml"), nodeset.to_xml)