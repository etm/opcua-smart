require 'mkmf'

spec = eval(File.read(__dir__ + '/../../../opcua.gemspec'))

$CFLAGS  = '-g -Wall ' + $CFLAGS
$CFLAGS  << ''
$LDFLAGS << ' -lopen62541'
dir_config('client')
create_makefile('client')
