#pragma once

#include <abi/Filesystem.h>

#include <libgraphic/Icon.h>
#include <libutils/String.h>

namespace FilePicker
{

struct FileInfo
{
    String name;
    FileType type;
    RefPtr<Graphic::Icon> icon;
    size_t size;
};

} // namespace FilePicker