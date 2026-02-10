#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace Trinity
{
	namespace Geometry
	{
		enum class PrimitiveType : uint8_t
		{
			Triangle = 0,
			Quad,
			Cube,
			Count
		};

		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec2 UV;
		};

		struct MeshData
		{
			std::vector<Vertex> Vertices;
			std::vector<uint32_t> Indices;
		};

		inline const MeshData& GetPrimitive(PrimitiveType type)
		{
			static const MeshData s_Triangle = []
			{
				MeshData l_MeshData;
				l_MeshData.Vertices =
				{
					{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
					{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
					{ {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 1.0f } },
				};
				l_MeshData.Indices = { 0, 1, 2 };

				return l_MeshData;
			}();

			static const MeshData s_Quad = []
			{
				MeshData l_MeshData;
				l_MeshData.Vertices =
				{
					{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
					{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
					{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
					{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				};
				l_MeshData.Indices = { 0, 1, 2, 2, 3, 0 };

				return l_MeshData;
			}();

			static const MeshData s_Cube = []
			{
				MeshData l_MeshData;
				l_MeshData.Vertices.reserve(24);
				l_MeshData.Indices.reserve(36);

				auto AddFace = [&l_MeshData](float nx, float ny, float nz, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3)
				{
					const uint32_t l_Base = (uint32_t)l_MeshData.Vertices.size();
					l_MeshData.Vertices.push_back({ {x0,y0,z0}, {nx,ny,nz}, {0,0} });
					l_MeshData.Vertices.push_back({ {x1,y1,z1}, {nx,ny,nz}, {1,0} });
					l_MeshData.Vertices.push_back({ {x2,y2,z2}, {nx,ny,nz}, {1,1} });
					l_MeshData.Vertices.push_back({ {x3,y3,z3}, {nx,ny,nz}, {0,1} });

					l_MeshData.Indices.insert(l_MeshData.Indices.end(), { l_Base + 0, l_Base + 1, l_Base + 2, l_Base + 2, l_Base + 3, l_Base + 0 });
				};

				const float l_Height = 0.5f;

				AddFace(0, 0, 1, -l_Height, -l_Height, l_Height, l_Height, -l_Height, l_Height, l_Height, l_Height, l_Height, -l_Height, l_Height, l_Height); // front
				AddFace(0, 0, -1, l_Height, -l_Height, -l_Height, -l_Height, -l_Height, -l_Height, -l_Height, l_Height, -l_Height, l_Height, l_Height, -l_Height); // back
				AddFace(1, 0, 0, l_Height, -l_Height, l_Height, l_Height, -l_Height, -l_Height, l_Height, l_Height, -l_Height, l_Height, l_Height, l_Height); // right
				AddFace(-1, 0, 0, -l_Height, -l_Height, -l_Height, -l_Height, -l_Height, l_Height, -l_Height, l_Height, l_Height, -l_Height, l_Height, -l_Height); // left
				AddFace(0, 1, 0, -l_Height, l_Height, l_Height, l_Height, l_Height, l_Height, l_Height, l_Height, -l_Height, -l_Height, l_Height, -l_Height); // top
				AddFace(0, -1, 0, -l_Height, -l_Height, -l_Height, l_Height, -l_Height, -l_Height, l_Height, -l_Height, l_Height, -l_Height, -l_Height, l_Height); // bottom

				return l_MeshData;
			}();

			switch (type)
			{
				case PrimitiveType::Triangle:
					return s_Triangle;
				case PrimitiveType::Quad:
					return s_Quad;
				case PrimitiveType::Cube:
					return s_Cube;
				default:
					return s_Triangle;
			}
		}
	}
}