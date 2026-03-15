#include "Trinity/ECS/SceneRenderer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/ECS/Components.h"
#include "Trinity/Assets/AssetManager.h"
#include "Trinity/Assets/MeshAsset.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

namespace Trinity
{
	SceneRenderer::RenderStats SceneRenderer::s_Stats{};
	float SceneRenderer::s_ViewportWidth = 1920.0f;
	float SceneRenderer::s_ViewportHeight = 1080.0f;

	std::vector<LightData> SceneRenderer::s_LightBuffer;
	glm::mat4 SceneRenderer::s_DirectionalLightSpaceMatrix = glm::mat4(1.0f);
	bool SceneRenderer::s_HasShadowCaster = false;

	void SceneRenderer::SetViewportSize(float width, float height)
	{
		s_ViewportWidth = width;
		s_ViewportHeight = height;
	}

	void SceneRenderer::Render(Scene& scene, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar)
	{
		ResetStats();

		CollectLights(scene);
		SubmitShadowPasses(scene);
		SubmitGeometryPass(scene, view, projection);
		SubmitLightingPass(view, projection, cameraPosition, cameraNear, cameraFar);
	}

	void SceneRenderer::Render(Scene& scene)
	{
		ResetStats();

		auto& a_Registry = scene.GetRegistry();
		bool l_Rendered = false;

		a_Registry.view<TransformComponent, CameraComponent>().each([&](const TransformComponent& transform, const CameraComponent& camera)
			{
				if (l_Rendered || !camera.Primary)
				{
					return;
				}

				const glm::mat4 l_View = glm::inverse(transform.GetTransform());

				glm::mat4 l_Projection;
				const float l_Aspect = s_ViewportHeight > 0.0f ? s_ViewportWidth / s_ViewportHeight : 1.0f;

				if (camera.Projection == ProjectionType::Perspective)
				{
					l_Projection = glm::perspective(glm::radians(camera.FieldOfViewDegrees), l_Aspect, camera.NearClip, camera.FarClip);
				}
				else
				{
					const float l_HalfSize = camera.OrthoSize * 0.5f;
					const float l_HalfWidth = l_HalfSize * l_Aspect;
					l_Projection = glm::ortho(-l_HalfWidth, l_HalfWidth, -l_HalfSize, l_HalfSize, camera.OrthoNear, camera.OrthoFar);
				}

				Render(scene, l_View, l_Projection, transform.Translation, camera.NearClip, camera.FarClip);
				l_Rendered = true;
			});
	}

	void SceneRenderer::CollectLights(Scene& scene)
	{
		s_LightBuffer.clear();
		s_HasShadowCaster = false;

		auto& a_Registry = scene.GetRegistry();

		a_Registry.view<TransformComponent, LightComponent>().each([&](const TransformComponent& transform, const LightComponent& light)
			{
				if (s_LightBuffer.size() >= MaxLights)
				{
					return;
				}

				LightData l_Data{};

				if (light.Type == LightType::Directional)
				{
					const glm::quat l_Rot = glm::quat(transform.Rotation);
					const glm::vec3 l_Dir = l_Rot * glm::vec3(0.0f, -1.0f, 0.0f);

					l_Data.PositionAndType = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
					l_Data.DirectionAndIntensity = glm::vec4(glm::normalize(l_Dir), light.Intensity);
					l_Data.ColorAndRange = glm::vec4(light.Color, 0.0f);
					l_Data.SpotAngles = glm::vec4(0.0f);

					if (light.CastShadows && !s_HasShadowCaster)
					{
						constexpr float l_ShadowOrthoSize = 20.0f;
						constexpr float l_ShadowNear = 0.1f;
						constexpr float l_ShadowFar = 100.0f;
						const glm::mat4 l_LightProj = glm::ortho(-l_ShadowOrthoSize, l_ShadowOrthoSize, -l_ShadowOrthoSize, l_ShadowOrthoSize, l_ShadowNear, l_ShadowFar);
						const glm::mat4 l_LightView = glm::lookAt(-glm::normalize(l_Dir) * (l_ShadowFar * 0.3f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); s_DirectionalLightSpaceMatrix = l_LightProj * l_LightView;
						s_HasShadowCaster = true;
					}
				}
				else if (light.Type == LightType::Point)
				{
					l_Data.PositionAndType = glm::vec4(transform.Translation, 1.0f);
					l_Data.DirectionAndIntensity = glm::vec4(0.0f, 0.0f, 0.0f, light.Intensity);
					l_Data.ColorAndRange = glm::vec4(light.Color, light.Range);
					l_Data.SpotAngles = glm::vec4(0.0f);
				}
				else
				{
					const glm::quat l_Rot = glm::quat(transform.Rotation);
					const glm::vec3 l_Dir = glm::normalize(l_Rot * glm::vec3(0.0f, -1.0f, 0.0f));

					const float l_InnerCos = std::cos(glm::radians(light.InnerConeAngleDegrees));
					const float l_OuterCos = std::cos(glm::radians(light.OuterConeAngleDegrees));

					l_Data.PositionAndType = glm::vec4(transform.Translation, 2.0f);
					l_Data.DirectionAndIntensity = glm::vec4(l_Dir, light.Intensity);
					l_Data.ColorAndRange = glm::vec4(light.Color, light.Range);
					l_Data.SpotAngles = glm::vec4(l_InnerCos, l_OuterCos, 0.0f, 0.0f);
				}

				s_LightBuffer.push_back(l_Data);
			});

		s_Stats.LightCount = static_cast<uint32_t>(s_LightBuffer.size());
	}

	void SceneRenderer::SubmitShadowPasses(Scene& scene)
	{
		if (!s_HasShadowCaster)
		{
			return;
		}

		AssetManager& l_AssetManager = AssetManager::Get();
		auto& a_Registry = scene.GetRegistry();

		RenderCommand::BeginShadowPass(s_DirectionalLightSpaceMatrix);

		a_Registry.view<TransformComponent, MeshRendererComponent>().each([&](const TransformComponent& transform, const MeshRendererComponent& meshRenderer)
			{
				if (!meshRenderer.Mesh)
				{
					return;
				}

				std::shared_ptr<MeshAsset> l_Mesh = l_AssetManager.Resolve(meshRenderer.Mesh);
				if (!l_Mesh || !l_Mesh->IsValid())
				{
					return;
				}

				const glm::mat4 l_LightSpaceMVP = s_DirectionalLightSpaceMatrix * transform.GetTransform();
				RenderCommand::DrawMeshShadow(l_Mesh->GetVertexBuffer(), l_Mesh->GetIndexBuffer(), l_Mesh->GetIndexCount(), l_LightSpaceMVP);
			});

		RenderCommand::EndShadowPass();
	}

	void SceneRenderer::SubmitGeometryPass(Scene& scene, const glm::mat4& view, const glm::mat4& projection)
	{
		AssetManager& l_AssetManager = AssetManager::Get();
		auto& a_Registry = scene.GetRegistry();
		const glm::mat4 l_VP = projection * view;

		RenderCommand::BeginGeometryPass();

		a_Registry.view<TransformComponent, MeshRendererComponent>().each([&](entt::entity entity, const TransformComponent& transform, const MeshRendererComponent& meshRenderer)
			{
				if (!meshRenderer.Mesh)
				{
					return;
				}

				std::shared_ptr<MeshAsset> l_Mesh = l_AssetManager.Resolve(meshRenderer.Mesh);
				if (!l_Mesh || !l_Mesh->IsValid())
				{
					return;
				}

				Texture2D* l_Albedo = nullptr;

				if (a_Registry.all_of<MaterialComponent>(entity))
				{
					const MaterialComponent& l_MatComp = a_Registry.get<MaterialComponent>(entity);
					(void)l_MatComp;
				}

				RenderCommand::DrawMeshDeferred(l_Mesh->GetVertexBuffer(), l_Mesh->GetIndexBuffer(), l_Mesh->GetIndexCount(), transform.GetTransform(), l_VP, l_Albedo);

				s_Stats.DrawCalls++;
				s_Stats.EntityCount++;
			});

		RenderCommand::EndGeometryPass();
	}

	void SceneRenderer::SubmitLightingPass(const glm::mat4& view, const glm::mat4& projection,
		const glm::vec3& cameraPosition, float cameraNear, float cameraFar)
	{
		LightingUniformData l_UniformData{};

		const uint32_t l_Count = static_cast<uint32_t>(s_LightBuffer.size());
		l_UniformData.LightCount = l_Count;
		l_UniformData.ShadowsEnabled = s_HasShadowCaster ? 1u : 0u;
		l_UniformData.DirectionalLightSpaceMatrix = s_DirectionalLightSpaceMatrix;

		for (uint32_t i = 0; i < l_Count; ++i)
		{
			l_UniformData.Lights[i] = s_LightBuffer[i];
		}

		RenderCommand::BeginLightingPass();
		RenderCommand::UploadLights(&l_UniformData, static_cast<uint32_t>(sizeof(LightingUniformData)));

		const glm::mat4 l_InvVP = glm::inverse(projection * view);
		RenderCommand::DrawLightingQuad(l_InvVP, cameraPosition, cameraNear, cameraFar);

		RenderCommand::EndLightingPass();
	}

	const SceneRenderer::RenderStats& SceneRenderer::GetStats()
	{
		return s_Stats;
	}

	void SceneRenderer::ResetStats()
	{
		s_Stats.DrawCalls = 0;
		s_Stats.EntityCount = 0;
		s_Stats.LightCount = 0;
	}
}