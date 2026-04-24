require 'fileutils'
require 'rake/extensiontask'
require 'rake/testtask'

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
  s.extensions = FileList["ext/LiteRGSS3/extconf.rb"]
  s.files = FileList["ext/LiteRGSS3/*.h", "ext/LiteRGSS3/*.hpp", "ext/LiteRGSS3/*.c", "ext/LiteRGSS3/*.cpp"]
  #s.metadata = { "source_code_uri" => "https://github.com/pokemonworkshop/litergss3", "documentation_uri" => "https://psdk.pokemonworkshop.fr/yard/LiteRGSS.html" }
  s.required_ruby_version = '>= 3.0.0'
end

Gem::PackageTask.new(spec) do |pkg|
end

Rake::ExtensionTask.new(ext_name, spec) do |ext|
  # Source tree is `ext/LiteRGSS3/` (kept from the v3 directory layout) but
  # the built artifact is `LiteRGSS.so` — `ext_name` above drives both the
  # artifact name and rake-compiler's default lookup dir, so point the dir
  # back explicitly.
  ext.ext_dir = 'ext/LiteRGSS3'
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
  require_relative 'external/litecgss2/build/raylib_installer'

  # Backend selection: --enable-sfml wins, otherwise default to raylib.
  backend =
    if enable_config('sfml')
      'sfml'
    else
      'raylib'
    end
  Cgss::RaylibInstaller.check! if backend == 'raylib'

  litecgss_root_dir = File.expand_path(File.dirname(__FILE__)) + "/external/litecgss2"

  Dir.chdir(litecgss_root_dir) {
    system("rake clean")
    extra_args = '--enable-debug '
    extra_args += "--enable-#{backend} "
    if enable_config('physfs')
      extra_args += '--enable-physfs '
    end
    system("rake compile -- #{extra_args}")
  }
end

# Non-interactive test suite. `test_*_live.rb` files open a real window and
# wait for user input — excluded here so this target stays CI-runnable.
Rake::TestTask.new(:test => :compile) do |t|
  t.libs << 'tests'
  t.test_files = FileList['tests/test_*.rb'].exclude(/_live\.rb$/)
  t.verbose = true
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
