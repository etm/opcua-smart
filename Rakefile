require 'rubygems/package_task'
require 'fileutils'

spec = eval(File.read('opcua.gemspec'))

task :compile do
  require 'rake/extensiontask'

  puts `cert/generate_certificate.sh`
  spec.extensions.each do |f|
    Rake.application.clear

    task :default => [:compile]

    t = Rake::ExtensionTask.new "opcua/" + File.basename(File.dirname(f)), spec
    Rake::Task[:default].invoke
  end
  Rake.application.clear
end

task :default => [:compile]

Gem::PackageTask.new(spec) do |pkg|
  pkg.need_zip = true
  pkg.need_tar = true
  FileUtils.mkdir 'pkg' rescue nil
  FileUtils.rm_rf Dir.glob('pkg/*')
  FileUtils.ln_sf "#{pkg.name}.gem", "pkg/#{spec.name}.gem"
end

task :push => :gem do |r|
  `gem push pkg/opcua.gem`
end

task :install => :gem do |r|
  `gem install pkg/opcua.gem`
end
