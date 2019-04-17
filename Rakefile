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

task :default => []

Gem::PackageTask.new(spec) do |pkg|
  pkg.need_zip = true
  pkg.need_tar = true
  `rm pkg/* -rf`
  `ln -sf #{pkg.name}.gem #{pkg.package_dir}/#{spec.name}.gem`
end
