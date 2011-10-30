@echo off
"%~dp0ruby" -x "%~f0" %*
@goto endofruby
#!ruby
# coding: utf-8
require 'tempfile'
require 'fileutils'
if ARGV.size != 1 || !File.directory?(ARGV[0])
  $stderr.puts 'usage: use_closure appname'
  exit
end
CONF = {
  'Gemfile' =>
    [/^gem\s+'closure-compiler'\s*$/m, "gem 'closure-compiler'", nil],
  'config/environments/production.rb' =>
    [/^\s*config.assets.js_compressor = :closure\s*$/m, "  config.assets.js_compressor = :closure",
     /(.+)^(end\s*)$/m ],
}

CONF.each do |file, repls|
  fin = File.open("#{ARGV[0]}/#{file}", 'r')
  conf = fin.read
  fin.close
  unless conf =~ repls[0]
    tmp = Tempfile.new('uscl')
    if repls[2]
      conf =~ repls[2]
      tmp.write $1
      tmp.puts repls[1]
      tmp.write $2
    else
      tmp.write conf
      tmp.puts repls[1]
    end
    tmp.close
    FileUtils.cp tmp.path, "#{ARGV[0]}/#{file}"
    $stdout.puts "#{file} converted"
  else
    $stdout.puts "#{file} skip"
  end
end

__END__
:endofruby

