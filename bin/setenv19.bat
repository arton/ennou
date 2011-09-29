@echo off
:WinNT
"%~dp0ruby" -x "%~f0" %*
@goto endofruby
#!/usr/bin/ruby
require 'winpath'
path = Pathname.new(File.dirname($0)).shortname.gsub(File::SEPARATOR, File::ALT_SEPARATOR)
ENV['PATH'] = "#{path};#{ENV['PATH']}"
Dir.chdir '../..'
if ARGV.size > 0
  cmd = 'start ruby "' + ARGV[0] + '"'
else
  cmd = "start cmd"
end
system(cmd)
__END__
:endofruby
