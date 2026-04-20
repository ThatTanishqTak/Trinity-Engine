#pragma once

#include "Trinity/Asset/AssetHandle.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <memory>

namespace Trinity
{
    struct TextureComponent
    {
        AssetHandle TextureAssetUUID = InvalidAsset;
        std::shared_ptr<Texture> TextureData;
    };
}
