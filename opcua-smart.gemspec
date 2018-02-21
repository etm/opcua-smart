Gem::Specification.new do |s|
  s.name             = "opcua-smart"
  s.version          = "0.1"
  s.platform         = Gem::Platform::RUBY
  s.license          = "LGPL-3.0"
  s.summary          = "Preliminary release of cloud process execution engine (cpee). If you just need workflow execution, without a rest/xmpp service exposing it, then use WEEL"

  s.description      = "see http://centurio.work"

  s.files            = Dir['{example/**/*,tools/**/*,lib/**/*,contrib/logo*}'] + %w(COPYING Rakefile opcua-smart.gemspec README.md)
  s.extensions       = Dir["ext/**/extconf.rb"]
  s.require_path     = 'ext'
  s.extra_rdoc_files = ['README.md']
  s.bindir           = 'tools'

  s.required_ruby_version = '>=2.3.0'

  s.authors          = ['Juergen eTM Mangler','Florian Pauker']

  s.email            = 'juergen.mangler@gmail.com'
  s.homepage         = 'http://centurio.work/'

  s.add_runtime_dependency 'riddl', '~> 0.99'
  s.add_runtime_dependency 'json', '~>2.1'
end
