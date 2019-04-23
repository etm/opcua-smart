Gem::Specification.new do |s|
  s.name             = "opcua"
  s.version          = "0.1"
  s.platform         = Gem::Platform::RUBY
  s.license          = "LGPL-3.0"
  s.summary          = "Preliminary release of cloud process execution engine (cpee). If you just need workflow execution, without a rest/xmpp service exposing it, then use WEEL"

  s.description      = "see http://centurio.work"

  s.files            = Dir['{example/**/*,tools/**/*,lib/**/*,contrib/logo*}'] + %w(COPYING Rakefile opcua.gemspec README.md)
  s.extensions       = Dir["ext/**/extconf.rb"]
  p s.extensions
  s.require_path     = 'lib'
  s.extra_rdoc_files = ['README.md']
  s.bindir           = 'tools'

  s.required_ruby_version = '>=2.3.0'

  s.authors          = ['Juergen eTM Mangler','Florian Pauker']

  s.email            = 'juergen.mangler@gmail.com'
  s.homepage         = 'http://centurio.work/'

  s.add_runtime_dependency 'riddl', '~> 0.99'
  s.add_development_dependency 'rake-compiler', '>= 0.7.9'
end