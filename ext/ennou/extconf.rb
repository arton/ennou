#----------------------------------
# extconf.rb
# $Revision: $
# $Date: $
#----------------------------------
require 'mkmf'

dir_config("win32")

def create_ennou_makefile
  if have_library("Httpapi") && have_header("Http.h")
    create_makefile("ennou")
  end
end

if RUBY_VERSION >= '1.9.1'
  create_ennou_makefile
else
  $stderr.puts 'ennou requires ruby 1.9'
end
