class NodeId
  def ns() @ns end
  def id() @id end
  def type() @type end
  def to_s() 
    if type.equal? NodeIdType::Numeric
      return "ns=#{ns};i=#{id}"
    elsif type.equal? NodeIdType::String
      return "ns=#{ns};s=#{id}"
    else
      return nil
    end
  end

  def initialize(namespaceindex, identifier, identifiertype=NodeIdType::String)
    raise "Bad namespaceindex #{namespaceindex}" unless(namespaceindex.is_a?(Integer) || namespaceindex >= 0)
    if(identifiertype == NodeIdType::String && (identifier =~ /\A[-+]?[0-9]+\z/) && identifier.to_i > 0)
      identifier = identifier.to_i
      identifiertype = NodeIdType::Numeric
    end
    @ns = namespaceindex
    @id = identifier
    @type = identifiertype
  end

  def self.from_string(nodeid)
    if nodeid.nil?
      return nil
    end
    if nodeid =~ /ns=(\d+);(.)=(.*)/
      ns = $1.to_i
      type = $2
      id = $3
    elsif nodeid =~ /(.)=(.*)/
      ns = 0
      type =  $1
      id =  $2
    else
      return nil
    end

    if type.eql? "i"
      nodeid_type = NodeIdType::Numeric
    elsif type.eql? "s"
      nodeid_type = NodeIdType::String
    else
      return nil
    end
    NodeId.new(ns, id, nodeid_type)
  end
end

class QualifiedName
  def ns() @ns end
  def name() @name end
  def to_s() "#{ns}:#{name}" end
    
  def initialize(namespaceindex, name) 
    raise "Bad namespaceindex #{namespaceindex}" unless(namespaceindex.is_a?(Integer) || namespaceindex >= 0)
    @ns = namespaceindex
    @name = name
  end

  def self.from_string(qualifiedname)
    if qualifiedname.include? ":"
      ns = qualifiedname.match(/^([^:]+):/)[1].to_i
      name = qualifiedname.match(/:(.*)/)[1]
    else
      ns = 0
      name = qualifiedname
    end
    QualifiedName.new(ns, name)
  end
end

class LocalizedText
  def locale() @locale end
  def text() @text end
  def to_s()
    return text if locale == ""
    "#{locale}:#{text}"
  end

  def initialize(text, locale="")
    return nil if text == ""
    @locale = locale
    @text = text
  end
  
  def self.from_string(localizedtext)
    locale = localizedtext.match(/^([^:]+):/)[1]
    text = localizedtext.match(/:(.*)/)[1]
    LocalizedText.new(text, locale)
  end
  def self.parse(xml_element)
    return nil if(xml_element.nil?)
    text = xml_element.to_s || ""
    locale = xml_element.find("@Locale").first.to_s || ""
    LocalizedText.new(text, locale)
  end
end

##
# NodeClasses see https://documentation.unified-automation.com/uasdkhp/1.0.0/html/_l2_ua_node_classes.html
class NodeClass
  Unspecified = 0
  Object = 1
  Variable = 2
  Method = 4
  ObjectType = 8
  VariableType = 16
  ReferenceType = 32
  DataType = 64
  View = 128
end

class NodeIdType
  Numeric = 0
  String = 3
end

class ValueRank
  ScalarOrOne = -3
  Any = -2
  Scalar = -1
  OneOrMore = 0
  One = 1
  Two = 2
  Three =3
end