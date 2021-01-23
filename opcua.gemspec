Gem::Specification.new do |s|
  s.name             = "opcua"
  s.version          = "0.22"
  s.platform         = Gem::Platform::RUBY
  s.license          = "LGPL-3.0"
  s.summary          = "Preliminary release of opcua (open62541) ruby bindings. C performance, Ruby elegance, simplicity, and productivity."

  s.description      = "see https://github.com/etm/opcua-smart"

  s.files            = Dir['{example/**/*,tools/**/*,lib/**/*.rb,ext/**/*.c,ext/**/*.h,cert/*.h,contrib/logo*}'] + %w(COPYING Rakefile opcua.gemspec README.md)
  s.extensions       = Dir["ext/**/extconf.rb"]
  s.require_path     = 'lib'
  s.extra_rdoc_files = ['README.md']
  s.bindir           = 'tools'

  s.required_ruby_version = '>=2.5.0'

  s.authors          = ['Juergen eTM Mangler','Florian Pauker','Matthias Ehrendorfer']

  s.email            = 'juergen.mangler@gmail.com'
  s.homepage         = 'https://github.com/etm/opcua-smart'

  s.add_runtime_dependency 'daemonite', '~> 0', '>= 0.5.4'
  s.add_development_dependency 'rake', '~> 12'
  s.add_development_dependency 'rake-compiler', '~> 1.0'
end
