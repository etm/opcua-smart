require 'mkmf'

spec = eval(File.read(__dir__ + '/../../opcua-smart.gemspec'))

$CFLAGS  = '-g -Wall ' + $CFLAGS
$CFLAGS  << ''
$LDFLAGS << ' -lopen62541'
dir_config(spec.name.gsub(/.*-/,''))
create_makefile(spec.name.gsub(/.*-/,''))
