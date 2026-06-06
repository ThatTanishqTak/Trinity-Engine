#include <Trinity/Assets/AssetType.h>

#include <algorithm>
#include <cctype>

namespace Trinity
{
    const char* AssetTypeToString(AssetType type)
    {
        switch (type)
        {
            case AssetType::Mesh: return "Mesh";
            case AssetType::Texture: return "Texture";
            default: return "None";
        }
    }

    AssetType AssetTypeFromString(const std::string& text)
    {
        if (text == "Mesh")
        {
            return AssetType::Mesh;
        }

        if (text == "Texture")
        {
            return AssetType::Texture;
        }

        return AssetType::None;
    }

    AssetType AssetTypeFromExtension(const std::string& extension)
    {
        std::string l_Lower = extension;
        std::transform(l_Lower.begin(), l_Lower.end(), l_Lower.begin(), [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

        if (l_Lower == ".obj" || l_Lower == ".fbx" || l_Lower == ".gltf" || l_Lower == ".glb" || l_Lower == ".dae" || l_Lower == ".3ds")
        {
            return AssetType::Mesh;
        }

        if (l_Lower == ".png" || l_Lower == ".jpg" || l_Lower == ".jpeg" || l_Lower == ".tga" || l_Lower == ".bmp" || l_Lower == ".hdr")
        {
            return AssetType::Texture;
        }

        return AssetType::None;
    }
}