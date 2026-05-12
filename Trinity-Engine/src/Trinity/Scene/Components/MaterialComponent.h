#pragma once

#include "Trinity/Asset/AssetHandle.h"
#include "Trinity/Renderer/Resources/Material.h"

#include <memory>

namespace Trinity
{
    struct MaterialComponent
    {
        AssetHandle MaterialAssetUUID = InvalidAsset;

        std::shared_ptr<Material> MaterialData = std::make_shared<Material>("Default Material");

        MaterialProperties OverrideProperties;

        bool UseOverrideProperties = true;

        MaterialProperties& GetEditableProperties()
        {
            if (UseOverrideProperties || !MaterialData)
            {
                return OverrideProperties;
            }

            return MaterialData->GetProperties();
        }

        const MaterialProperties& GetEditableProperties() const
        {
            if (UseOverrideProperties || !MaterialData)
            {
                return OverrideProperties;
            }

            return MaterialData->GetProperties();
        }
    };
}