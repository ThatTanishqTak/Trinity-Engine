#pragma once

#include <cstdint>
#include <string>

#include <entt/entt.hpp>

#include <Editor/ICommand.h>

namespace Trinity
{
    class Scene;
    class MeshLibrary;
    class Entity;

    Entity FindEntityByUUID(Scene& scene, uint64_t uuid);

    class CreateEntityCommand : public ICommand
    {
    public:
        CreateEntityCommand(Scene& scene, const std::string& name);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override { return "Create Entity"; }

        uint64_t GetResultUUID() const { return m_UUID; }

    private:
        Scene& m_Scene;
        std::string m_EntityName;
        uint64_t m_UUID = 0;
        bool m_HasUUID = false;
    };

    class DeleteEntityCommand : public ICommand
    {
    public:
        DeleteEntityCommand(Scene& scene, MeshLibrary& meshLibrary, entt::entity handle);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override { return "Delete Entity"; }

    private:
        Scene& m_Scene;
        MeshLibrary& m_MeshLibrary;
        uint64_t m_UUID = 0;
        std::string m_Data;
    };

    class DuplicateEntityCommand : public ICommand
    {
    public:
        DuplicateEntityCommand(Scene& scene, MeshLibrary& meshLibrary, entt::entity handle);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override { return "Duplicate Entity"; }

        uint64_t GetResultUUID() const { return m_DuplicateUUID; }

    private:
        Scene& m_Scene;
        MeshLibrary& m_MeshLibrary;
        uint64_t m_SourceUUID = 0;
        uint64_t m_DuplicateUUID = 0;
        std::string m_Data;
    };

    class RenameEntityCommand : public ICommand
    {
    public:
        RenameEntityCommand(Scene& scene, uint64_t uuid, const std::string& oldName, const std::string& newName);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override { return "Rename Entity"; }

    private:
        Scene& m_Scene;
        uint64_t m_UUID = 0;
        std::string m_OldName;
        std::string m_NewName;
    };

    class SetParentCommand : public ICommand
    {
    public:
        SetParentCommand(Scene& scene, uint64_t childUUID, uint64_t newParentUUID);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override { return "Reparent Entity"; }

    private:
        Scene& m_Scene;
        uint64_t m_ChildUUID = 0;
        uint64_t m_NewParentUUID = 0;
        uint64_t m_OldParentUUID = 0;
    };
}