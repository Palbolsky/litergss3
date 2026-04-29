#ifdef LITECGSS_USE_PHYSFS

#ifndef Asset_H
#define Asset_H

// <windows.h> defines `stat` as a macro on MSVC; clobbers cgss::AssetFile::stat.
#undef stat

#include "RubyValue.h"
#include <LiteCGSS/Common/Assets.h>
#include <memory>

extern VALUE rb_cAssetsArchive;
extern VALUE rb_cAssetFile;
extern VALUE rb_mAssetWriter;

void Init_Asset();

// cgss::AssetsArchive and cgss::AssetFile both lack default ctors (they
// require args at construction). Wrap in unique_ptr so rb::Alloc can
// default-construct an empty wrapper, with the actual instance built in
// Ruby `initialize`.
struct AssetsArchiveData
{
    std::unique_ptr<cgss::AssetsArchive> archive;
};

struct AssetFileData
{
    std::unique_ptr<cgss::AssetFile> file;
};

#endif
#endif
