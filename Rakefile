require 'rubygems/package_task'
require 'rake/extensiontask'

spec = eval(File.read('opcua.gemspec'))

puts `cert/generate_certificate.sh`

spec.extensions.each do |f|
  Rake.application.clear

  task :default => [:compile]

  t = Rake::ExtensionTask.new "opcua/" + File.basename(File.dirname(f)), spec
  Rake::Task[:default].invoke
end
Rake.application.clear

task :default => [:doit]

pkg = Gem::PackageTask.new(spec) { |pkg|
  pkg.need_zip = true
  pkg.need_tar = true
}

p pkg.class

task :doit => :gem do |r|
  `rm pkg/* -rf`
  `ln -sf #{spec.name}.gem pkg/#{spec.name}.gem`
end

task :push => :gem do |r|
  `gem push pkg/opcua.gem`
end

task :install => :gem do |r|
  `gem install pkg/opcua.gem`
end
