# coding: utf-8
=begin
  this scripts remove *.obj, *.lib, *.exp, vc*.pdb *.log from ext directory for each gems
=end
require 'fileutils'

if ARGV.size == 0
  $stderr.puts 'usage: ruby remove_workfiles.rb gems-directory'
  exit
end

Dir.glob("#{ARGV[0]}/**/ext").each do |dir|
  FileUtils.remove ['*.obj', '*.lib', '*.exp', 'vc*.pdb', '*.def', '*.log'].map {|tmpl|
    Dir.glob("#{dir}/**/#{tmpl}")
  }.flatten, :verbose => true
end
