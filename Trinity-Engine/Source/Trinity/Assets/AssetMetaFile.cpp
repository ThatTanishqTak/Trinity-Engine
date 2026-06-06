#include <Trinity/Assets/AssetMetaFile.h>

#include <fstream>

#include <Trinity/Serialization/YAMLUtilities.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static constexpr uint32_t k_AssetMetaVersion = 1;

    bool AssetMetaFile::Write(const std::filesystem::path& metaPath, const AssetMetadata& metadata)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Version" << YAML::Value << k_AssetMetaVersion;
        l_Out << YAML::Key << "ID" << YAML::Value << static_cast<uint64_t>(metadata.ID);
        l_Out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(metadata.Type);
        l_Out << YAML::Key << "Source" << YAML::Value << metadata.SourcePath;
        l_Out << YAML::Key << "Timestamp" << YAML::Value << metadata.SourceTimestamp;

        l_Out << YAML::Key << "Import" << YAML::Value << YAML::BeginMap;
        l_Out << YAML::Key << "SRGB" << YAML::Value << metadata.Import.SRGB;
        l_Out << YAML::Key << "GenerateMips" << YAML::Value << metadata.Import.GenerateMips;
        l_Out << YAML::EndMap;

        if (!metadata.Dependencies.empty())
        {
            l_Out << YAML::Key << "Dependencies" << YAML::Value << YAML::BeginSeq;
            for (const UUID& it_Dependency : metadata.Dependencies)
            {
                l_Out << static_cast<uint64_t>(it_Dependency);
            }
            l_Out << YAML::EndSeq;
        }

        l_Out << YAML::EndMap;

        std::ofstream l_Stream(metaPath);
        if (!l_Stream.is_open())
        {
            TR_CORE_ERROR("AssetMetaFile: cannot write '{}'", metaPath.string());

            return false;
        }

        l_Stream << l_Out.c_str();

        return true;
    }

    std::optional<AssetMetadata> AssetMetaFile::Read(const std::filesystem::path& metaPath)
    {
        try
        {
            YAML::Node l_Root = YAML::LoadFile(metaPath.string());

            if (!l_Root["ID"] || !l_Root["Type"])
            {
                TR_CORE_WARN("AssetMetaFile: '{}' missing ID/Type", metaPath.string());

                return std::nullopt;
            }

            AssetMetadata l_Metadata;
            l_Metadata.ID = UUID(l_Root["ID"].as<uint64_t>());
            l_Metadata.Type = AssetTypeFromString(l_Root["Type"].as<std::string>());
            l_Metadata.SourcePath = l_Root["Source"] ? l_Root["Source"].as<std::string>() : "";
            l_Metadata.SourceTimestamp = l_Root["Timestamp"] ? l_Root["Timestamp"].as<uint64_t>() : 0;

            if (YAML::Node l_Import = l_Root["Import"])
            {
                l_Metadata.Import.SRGB = l_Import["SRGB"] ? l_Import["SRGB"].as<bool>() : true;
                l_Metadata.Import.GenerateMips = l_Import["GenerateMips"] ? l_Import["GenerateMips"].as<bool>() : true;
            }

            if (YAML::Node l_Dependencies = l_Root["Dependencies"])
            {
                for (const YAML::Node& it_Dependency : l_Dependencies)
                {
                    l_Metadata.Dependencies.emplace_back(it_Dependency.as<uint64_t>());
                }
            }

            return l_Metadata;
        }
        catch (const std::exception& exception)
        {
            TR_CORE_ERROR("AssetMetaFile: failed to read '{}' ({})", metaPath.string(), exception.what());

            return std::nullopt;
        }
    }
}