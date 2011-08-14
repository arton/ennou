@echo off
"%~dp0ruby" -x "%~f0" %*
@goto endofruby
#!ruby
# coding: utf-8
require 'tempfile'
require 'fileutils'
TXTS = ['.rb', '.ru', '.yml', '.erb', '.html', '.txt', '' ]
if ARGV.size != 1
  $stderr.puts 'usage: editable appname'
  exit
end

def convert(f)
  tmp = Tempfile.new('ned')
  tmp.binmode
  File.open(f, 'r').each_line do |line|
    tmp.write "#{line.rstrip}\r\n"
  end.close
  tmp.close
  FileUtils.cp tmp.path, f
  $stdout.puts "#{f} converted"
end

def visit(dir)
  Dir.chdir(dir)
  Dir.open('.').each do |f|
    next if f[0] == '.'
    if File.directory?(f)
      visit(f)
      Dir.chdir('..')
    elsif TXTS.include?(File.extname(f).downcase)
      convert(f)
    end
  end
end

visit ARGV[0]

__END__
:endofruby
