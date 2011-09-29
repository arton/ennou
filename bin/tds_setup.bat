@echo off
:WinNT
"%~dp0ruby" -x "%~f0" %*
@goto endofruby
#!ruby
# coding: utf-8
require 'tempfile'
require 'fileutils'

if ARGV.size != 1 || !File.directory?(ARGV[0])
  $stderr.puts 'usage: tds_setup appname'
  exit
end

CONF = {
  'Gemfile' => [
    [/\Agem\s+'sqlite3'\s*\z/, "gem 'activerecord-sqlserver-adapter'"],
  ],
  'config/database.yml' => [
    [/\A\s+adapter:.+\z/, '  adapter: sqlserver'],
    [/\A\s+database:\s*db\/([^.]+).+\z/, "  database: \\1db\n  username: sa\n  password: hidden\n  dataserver: localhost\\SQLEXPRESS"],
  ],
  'config/application.rb' => [
    [/\A  end\s*\z/, "    config.action_controller.asset_path = \"/#{ARGV[0]}/%s\"\n  end"],
  ],
}

CONF.each do |file, repls|
  tmp = Tempfile.new('tdsup')
  File.open("#{ARGV[0]}/#{file}", 'r').each_line do |line|
    tmp.puts(repls.inject(line.rstrip) {|r, v| r.gsub(v[0], v[1])})
  end.close
  tmp.close
  FileUtils.cp tmp.path, "#{ARGV[0]}/#{file}"
  $stdout.puts "#{file} converted"
end

File.open("#{ARGV[0]}/lib/tasks/createdb.rake", 'w') do |fout|
  fout.write <<EOF
namespace :db do
  task :create_db => :load_config do
    exec_from_master :create_database
  end

  task :drop_db => :load_config do
    exec_from_master :drop_database
  end

  def exec_from_master(cmd)
    ActiveRecord::Base.configurations.each_value do |config|
      target = config['database']
      config['database'] = 'master'
      ActiveRecord::Base.establish_connection(config)
      ActiveRecord::Base.connection.__send__(cmd, target)
    end
  end
end
EOF
end

system("#{File.dirname($0)}/editable.bat #{ARGV[0]}")

__END__
:endofruby
