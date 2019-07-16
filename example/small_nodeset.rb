require 'nokogiri'

nodeset = Nokogiri::XML(File.read(File.join(File.dirname(__FILE__), "../lib/opcua/Opc.Ua.1.04.NodeSet2.xml")))

nodeset.xpath("/*/*").each do |n|
  if n.attr("NodeId") =~ /i=(.*)/
    n.remove if $1.to_i > 1000
  end
end

File.write(File.join(File.dirname(__FILE__), "../lib/opcua/Opc.Ua.tiny.NodeSet2.xml"), nodeset.to_xml)