#include <Trinity/Core/Config.h>

#include <sstream>
#include <vector>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/FileManagement.h>

namespace Trinity
{
    static std::vector<std::string> SplitKey(const std::string& key)
    {
        std::vector<std::string> l_Parts;
        std::stringstream l_Stream(key);
        std::string l_Part;

        while (std::getline(l_Stream, l_Part, '.'))
        {
            if (!l_Part.empty())
            {
                l_Parts.push_back(l_Part);
            }
        }

        return l_Parts;
    }

    bool Config::Load(const std::filesystem::path& path)
    {
        std::optional<std::string> l_Content = FileManagement::ReadText(path);
        if (!l_Content)
        {
            return false;
        }

        try
        {
            m_Root = YAML::Load(*l_Content);
        }
        catch (const YAML::Exception& exception)
        {

            return false;
        }

        return true;
    }

    bool Config::Save(const std::filesystem::path& path) const
    {
        YAML::Emitter l_Emitter;
        l_Emitter << m_Root;

        if (!l_Emitter.good())
        {

            return false;
        }

        return FileManagement::WriteText(path, std::string(l_Emitter.c_str()));
    }

    bool Config::Has(const std::string& key) const
    {
        return static_cast<bool>(Resolve(key));
    }

    YAML::Node Config::Resolve(const std::string& key) const
    {
        std::vector<std::string> l_Parts = SplitKey(key);
        YAML::Node l_Node = YAML::Clone(m_Root);

        for (const std::string& l_Part : l_Parts)
        {
            if (!l_Node || !l_Node.IsMap())
            {
                return YAML::Node(YAML::NodeType::Undefined);
            }

            l_Node = l_Node[l_Part];
        }

        return l_Node;
    }

    YAML::Node Config::ResolveOrCreate(const std::string& key)
    {
        std::vector<std::string> l_Parts = SplitKey(key);
        YAML::Node l_Node = m_Root;

        for (const std::string& l_Part : l_Parts)
        {
            l_Node = l_Node[l_Part];
        }

        return l_Node;
    }
}