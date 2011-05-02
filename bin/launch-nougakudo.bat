@echo off
"%~dp0ruby" -x "%~f0" %*
@goto endofruby
#!/usr/bin/ruby
require 'suexec'
path = File::dirname($0).gsub(File::SEPARATOR, File::ALT_SEPARATOR)
ENV['PATH'] = "#{path};#{ENV['PATH']}"
SuExec.exec %|#{ENV['SystemRoot']}\\system32\\mshta.exe|, *[%|"#{path}\\nougakudo.hta"|]
__END__
:endofruby


