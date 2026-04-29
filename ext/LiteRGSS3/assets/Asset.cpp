// PhysFS-backed asset access. Whole TU is gated by LITECGSS_USE_PHYSFS;
// when undefined this file compiles to nothing and the Ruby classes
// AssetsArchive / AssetFile / AssetWriter are not registered. Callers can
// detect availability via `defined?(LiteRGSS::AssetsArchive)`.

#ifdef LITECGSS_USE_PHYSFS

#include "Asset.h"
#include "LiteRGSS.h"
#include "../rbAdapter.h"
#include <stdexcept>

VALUE rb_cAssetsArchive = Qnil;
VALUE rb_cAssetFile = Qnil;
VALUE rb_mAssetWriter = Qnil;

static AssetsArchiveData *get_archive(VALUE self) { return rb::GetPtr<AssetsArchiveData>(self); }
static AssetFileData *get_file(VALUE self) { return rb::GetPtr<AssetFileData>(self); }

VALUE rb_AssetsArchive_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE path;
    rb_scan_args(argc, argv, "1", &path);
    rb_check_type(path, T_STRING);
    auto *a = get_archive(self);
    try {
        a->archive = std::make_unique<cgss::AssetsArchive>(std::string{StringValueCStr(path)});
    } catch (const std::runtime_error &e) {
        rb_raise(rb_eRGSSError, "%s", e.what());
    }
    return self;
}

VALUE rb_AssetsArchive_Dispose(VALUE self) { get_archive(self)->archive.reset(); return Qnil; }

VALUE rb_AssetFile_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE path, mode;
    rb_scan_args(argc, argv, "2", &path, &mode);
    rb_check_type(path, T_STRING);
    rb_check_type(mode, T_STRING);
    auto *f = get_file(self);
    try {
        f->file = std::make_unique<cgss::AssetFile>(
            std::string{StringValueCStr(path)},
            std::string{StringValueCStr(mode)});
    } catch (const std::runtime_error &e) {
        rb_raise(rb_eRGSSError, "Error opening asset %s mode %s : %s",
                 StringValueCStr(path), StringValueCStr(mode), e.what());
    }
    return self;
}

VALUE rb_AssetFile_Dispose(VALUE self) { get_file(self)->file.reset(); return Qnil; }
VALUE rb_AssetFile_Close(VALUE self)
{
    auto *f = get_file(self);
    if (f->file) f->file->close();
    return self;
}

VALUE rb_AssetFile_Read(VALUE self)
{
    auto *f = get_file(self);
    if (!f->file) return Qnil;
    try {
        const auto data = f->file->fullLoad();
        return rb_str_new(data.data(), static_cast<long>(data.size()));
    } catch (const std::runtime_error &e) {
        rb_raise(rb_eRGSSError, "Error reading asset %s : %s",
                 f->file->path().c_str(), e.what());
    }
    return Qnil;
}

VALUE rb_AssetFile_Append(VALUE self, VALUE content)
{
    rb_check_type(content, T_STRING);
    auto *f = get_file(self);
    if (!f->file) return self;
    try {
        *f->file << std::string{RSTRING_PTR(content),
                                static_cast<size_t>(RSTRING_LEN(content))};
    } catch (const std::runtime_error &e) {
        rb_raise(rb_eRGSSError, "Error appending to asset : %s", e.what());
    }
    return self;
}

VALUE rb_AssetFile_Exist(VALUE self, VALUE filepath)
{
    (void)self;
    rb_check_type(filepath, T_STRING);
    return cgss::AssetFile::exists(StringValueCStr(filepath)) ? Qtrue : Qfalse;
}

VALUE rb_AssetFile_IsDirectory(VALUE self, VALUE filepath)
{
    (void)self;
    rb_check_type(filepath, T_STRING);
    auto stat = cgss::AssetFile::stat(StringValueCStr(filepath));
    if (!stat.has_value()) return Qfalse;
    return stat->filetype == cgss::AssetFile::FileType::Directory ? Qtrue : Qfalse;
}

VALUE rb_AssetFile_IsSymlink(VALUE self, VALUE filepath)
{
    (void)self;
    rb_check_type(filepath, T_STRING);
    auto stat = cgss::AssetFile::stat(StringValueCStr(filepath));
    if (!stat.has_value()) return Qfalse;
    return stat->filetype == cgss::AssetFile::FileType::Symlink ? Qtrue : Qfalse;
}

VALUE rb_AssetFile_Enumerate(VALUE self, VALUE directory_path)
{
    (void)self;
    rb_check_type(directory_path, T_STRING);
    auto list = cgss::AssetFile::enumerate(StringValueCStr(directory_path));
    VALUE result = rb_ary_new_capa(static_cast<long>(list.size()));
    for (const auto &name : list) rb_ary_push(result, rb_str_new_cstr(name.c_str()));
    return result;
}

VALUE rb_AssetWriter_SetWriteDir(VALUE self, VALUE directory_path)
{
    rb_check_type(directory_path, T_STRING);
    if (cgss::AssetWriter::SetDirectory(std::string{StringValueCStr(directory_path)}) == 0) {
        rb_raise(rb_eRGSSError, "Error setting asset write directory '%s'",
                 StringValueCStr(directory_path));
    }
    return self;
}

void Init_Asset()
{
    rb_cAssetsArchive = rb_define_class_under(rb_mLiteRGSS, "AssetsArchive", rb_cObject);
    rb_define_alloc_func(rb_cAssetsArchive, rb::Alloc<AssetsArchiveData>);
    rb_define_method(rb_cAssetsArchive, "initialize", _rbf rb_AssetsArchive_Initialize, -1);
    rb_define_method(rb_cAssetsArchive, "dispose", _rbf rb_AssetsArchive_Dispose, 0);

    rb_cAssetFile = rb_define_class_under(rb_mLiteRGSS, "AssetFile", rb_cObject);
    rb_define_alloc_func(rb_cAssetFile, rb::Alloc<AssetFileData>);
    rb_define_method(rb_cAssetFile, "initialize", _rbf rb_AssetFile_Initialize, -1);
    rb_define_method(rb_cAssetFile, "read", _rbf rb_AssetFile_Read, 0);
    rb_define_method(rb_cAssetFile, "dispose", _rbf rb_AssetFile_Dispose, 0);
    rb_define_method(rb_cAssetFile, "close", _rbf rb_AssetFile_Close, 0);
    rb_define_method(rb_cAssetFile, "<<", _rbf rb_AssetFile_Append, 1);
    rb_define_singleton_method(rb_cAssetFile, "exist?", _rbf rb_AssetFile_Exist, 1);
    rb_define_singleton_method(rb_cAssetFile, "is_directory?", _rbf rb_AssetFile_IsDirectory, 1);
    rb_define_singleton_method(rb_cAssetFile, "is_symlink?", _rbf rb_AssetFile_IsSymlink, 1);
    rb_define_singleton_method(rb_cAssetFile, "enumerate", _rbf rb_AssetFile_Enumerate, 1);

    rb_mAssetWriter = rb_define_module_under(rb_mLiteRGSS, "AssetWriter");
    rb_define_module_function(rb_mAssetWriter, "write_dir=", _rbf rb_AssetWriter_SetWriteDir, 1);
}

#endif
