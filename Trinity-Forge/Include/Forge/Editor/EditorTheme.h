#pragma once

namespace Trinity
{
    class FileSystem;

    class EditorTheme
    {
    public:
        static void Apply(FileSystem& fileSystem);
    };
}