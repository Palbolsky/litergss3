require 'fileutils'
require 'rake/extensiontask'

ext_name = 'LiteRGSS'
litergss_root_dir = File.expand_path(File.dirname(__FILE__))

spec = Gem::Specification.new do |s|
  s.name = ext_name
  s.platform = Gem::Platform::RUBY
  s.version = '3.0.0-dev'
  s.summary = "LiteRGSS3 library, make games with Ruby easily"
  s.description = "LiteRGSS3 stands for Lite Ruby Game Scripting System and is an enhanced open-source version of the original RGSS written by Enterbrain."
  s.authors = ["Nuri Yuri", "Ota", "Palbolsky", "Scorbutics", "SuperFola", "nthoang-apcs"]
  s.homepage = "https://pokemonworkshop.com"
  s.extensions = FileList["ext/LiteRGSS/extconf.rb"]
  s.files = FileList["ext/LiteRGSS/*.h", "ext/LiteRGSS/*.hpp", "ext/LiteRGSS/*.c", "ext/LiteRGSS/*.cpp"]
  #s.metadata = { "source_code_uri" => "https://github.com/pokemonworkshop/litergss3", "documentation_uri" => "https://psdk.pokemonworkshop.fr/yard/LiteRGSS.html" }
  s.required_ruby_version = '>= 3.0.0'
end

Rake::ExtensionTask.new(ext_name, spec) do |ext|
  # cross compilation requires a cross compile toolchain
  ext.cross_compile = true
  ext.cross_platform = ['x86-mingw32']
  ext.lib_dir = 'build/lib'
  ext.tmp_dir = 'build/tmp'
end

desc "Clean the project"
task :clean do
    puts "Cleaning LiteRGSS in #{litergss_root_dir}... "
    Dir.chdir(litergss_root_dir) {
        FileUtils.rm_rf litergss_root_dir + "/build"
    }
end
