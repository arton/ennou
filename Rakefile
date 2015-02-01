require 'fileutils'
require 'rake/extensiontask'
require 'rubygems/package_task'

def read_version
  File.open('ext/ennou/ennou.c').each_line do |x|
    m = /Ennou_VERSION\s+"(.+?)"/.match(x)
    if m
      return m[1]
    end
  end
  nil
end

spec = Gem::Specification.new do |spec|
  spec.name = 'ennou'
  spec.authors = 'arton'
  spec.email = 'artonx@gmail.com'
  if /mswin|mingw/ =~ RUBY_PLATFORM
    spec.platform = Gem::Platform::CURRENT
  else
    spec.platform = Gem::Platform::RUBY
  end
  spec.extensions = FileList["ext/**/extconf.rb"]
  spec.required_ruby_version = '>= 1.9.2'
  spec.summary = 'Windows Http.sys Binding for Rack'
  spec.homepage = 'https://github.com/arton/ennou'
  spec.version = read_version
  spec.requirements << 'none'
  spec.require_path = 'lib'
  files = FileList['bin/*.bat', 'bin/*.hta', 'ext/ennou/*.h', 'ext/ennou/*.c',
                   'lib/**/*.rb',
                   'test/*.rb', 'setup.rb', 'Rakefile', 'COPYING', 'README']
  if /mswin|mingw/ =~ RUBY_PLATFORM
    unless File.exists?("lib/#{RbConfig::CONFIG['arch']}")
      FileUtils.mkdir("lib/#{RbConfig::CONFIG['arch']}")
    end
    FileUtils.cp(Dir.glob('ext/**/*.so'), "lib/#{RbConfig::CONFIG['arch']}")
    files += FileList['ext/**/*.so']
  end
  spec.files = files
  spec.test_file = 'test/test_server.rb'
  spec.description =<<EOD
Ennou is Rackable Web Server stands on Http.sys.
EOD
end

Gem::PackageTask.new(spec) do |pkg|
  pkg.gem_spec = spec
  pkg.need_zip = false
  pkg.need_tar = false  
end

Rake::ExtensionTask.new('ennou', spec)

