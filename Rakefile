require 'fileutils'
require 'rake/extensiontask'

ext_name = 'LiteRGSS3'
litergss_root_dir = File.expand_path(File.dirname(__FILE__))

spec = Gem::Specification.new do |s|
  s.name = ext_name
  s.platform = Gem::Platform::RUBY
  s.version = '3.0.0-dev'
  s.summary = "LiteRGSS3 library, make games with Ruby easily"
  s.description = "LiteRGSS3 stands for Lite Ruby Game Scripting System and is an enhanced open-source version of the original RGSS written by Enterbrain."
  s.authors = ["Nuri Yuri", "Ota", "Palbolsky", "Scorbutics", "SuperFola", "nthoang-apcs"]
  s.homepage = "https://pokemonworkshop.com"
  s.extensions = FileList["ext/LiteRGSS3/extconf.rb"]
  s.files = FileList["ext/LiteRGSS3/*.h", "ext/LiteRGSS3/*.hpp", "ext/LiteRGSS3/*.c", "ext/LiteRGSS3/*.cpp"]
  #s.metadata = { "source_code_uri" => "https://github.com/pokemonworkshop/litergss3", "documentation_uri" => "https://psdk.pokemonworkshop.fr/yard/LiteRGSS.html" }
  s.required_ruby_version = '>= 3.0.0'
end

Gem::PackageTask.new(spec) do |pkg|
end

Rake::ExtensionTask.new(ext_name, spec) do |ext|
  # cross compilation requires a cross compile toolchain
  ext.cross_compile = true
  ext.cross_platform = ['x86-mingw32']
  ext.lib_dir = 'build/lib'
  ext.tmp_dir = 'build/tmp'
end

desc "Configure the project by checking / setting up a working environment"
task :configure do |t, args|
  require_relative 'external/litecgss2/build/system_env'
  require_relative 'external/litecgss2/build/ruby_installer'

  litecgss_root_dir = File.expand_path(File.dirname(__FILE__)) + "/external/litecgss2"
  Dir.chdir(litecgss_root_dir) {
    system("rake configure")
  }

  Dir.chdir(litecgss_root_dir) {
    system("rake clean")
    extra_args = '--enable-debug '
    if enable_config('physfs')
      extra_args += '--enable-physfs '
    end
    system("rake compile -- #{extra_args}")
  }
end

desc "Build the project, in release mode"
task :release do
  litecgss_root_dir = File.expand_path(File.dirname(__FILE__)) + "/external/litecgss2"
  Dir.chdir(litecgss_root_dir) {
    system("rake release")
  }
  Rake::Task[:compile].invoke()
end

desc "Clean the project"
task :clean do
    puts "Cleaning LiteRGSS in #{litergss_root_dir}... "
    Dir.chdir(litergss_root_dir) {
        FileUtils.rm_rf litergss_root_dir + "/build"
    }
end
