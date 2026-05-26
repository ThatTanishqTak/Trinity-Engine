# Trinity Engine — Master Integration Roadmap
### Vulkan Backend + Trinity Forge Editor, Full Deferred Rendering, Physics, Audio, Scripting, Build Pipeline — Commercial Architecture

---

## Overview

This is the single authoritative plan for bringing Trinity Engine from its current stripped state to a fully operational Vulkan-backed engine with a complete ImGui-driven editor, 2D and 3D physics, spatial audio, Lua scripting, and a custom asset-cooking and distribution build pipeline. It is a single linear sequence of twenty-four phases, each producing a concrete compilable milestone with a clear validation criterion.

No phase leaves the engine in a broken state. Every decision made during planning is encoded here and does not need to be revisited.

---

## Confirmed Decisions Reference

| Decision | Choice |
|---|---|
| Renderer abstraction | Thick — Forge sees zero Vulkan |
| Rendering approach | Deferred (GeometryBuffer + Shadow now, gaps for future passes) |
| Backend selection | Runtime polymorphism via factory |
| Shader compilation | shaderc at build-time and editor-time only, SPIR-V at runtime |
| Windowing and Input | SDL3 for Desktop, platform-specific stubs for consoles |
| ImGui ownership | Engine-side `ImGuiLayer`, Forge pushes it |
| Docking | Full docking within main window |
| Multi-viewport | Disabled — panels dock inside main window only, no OS tear-off |
| Viewport rendering | Render to texture, displayed via `ImGui::Image` |
| Panel architecture | Panel registration system with toggle support |
| ImGuizmo | Yes, including Vulkan NDC Y-axis flip |
| Editor UI framework | ImGui (MIT, no commercial licence required) |
| Console platforms | PlayStation and Nintendo .gitignored, Xbox stub present |
| ECS | EnTT — already integrated, gaps filled across scene phases |
| Math | GLM — already integrated |
| 3D model import | Assimp — dependency present, import layer to be made functional |
| Logging | spdlog — already integrated |
| Scene serialisation | yaml-cpp — already integrated |
| 2D physics | Box2D — separate phase from 3D |
| 3D physics | PhysX 5 open-source — separate phase from 2D |
| Physics editor panels | Deferred to visual polish phase, not part of physics phases |
| Audio | OpenAL Soft — basic mixer and bus system from the start |
| Scripting language | Lua via sol2 — full component access from Lua |
| Script workflow | External editor + file-watcher hot-reload, no embedded text editor |
| Build pipeline | Fully custom asset-cooking and packaging system, independent of CMake |
| Distribution format | Flat folder — executable + cooked assets, matching Unity/Unreal convention |
| Build pipeline targets | Windows primary, macOS and Xbox as stubs |

---

## Platform Matrix

| Platform | Window / Input | Graphics API | Status |
|---|---|---|---|
| Windows | SDL3 | Vulkan | Active development |
| macOS | SDL3 | MoltenVK (Vulkan over Metal) | Stub |
| Xbox | GDK | DirectX 12 | Stub |
| PlayStation | Sony SDK | GNM / GNMX | .gitignored |
| Nintendo Switch | NVN SDK | NVN | .gitignored |

---

## Phase 1 — Repository and CMake Restructure

This phase touches no runtime logic. It is purely reorganisation of files and build configuration so every subsequent phase builds on a clean foundation.

### 1.1 — Platform Layer Reorganisation

The existing `WindowsWindow.h/.cpp` and `SDLEventTranslator.h/.cpp` are moved and renamed. No logic changes, only relocation.

```
Trinity-Engine/src/Trinity/Platform/
|-- Window/
|   |-- Window.h                      ← abstract base (unchanged interface)
|   |-- Window.cpp                    ← factory using compile-time defines
|   |-- Desktop/
|   |   |-- DesktopWindow.h           ← renamed from WindowsWindow.h
|   |   └-- DesktopWindow.cpp         ← renamed from WindowsWindow.cpp
|   |-- Xbox/
|   |   |-- XboxWindow.h              ← empty stub
|   |   └-- XboxWindow.cpp            ← empty stub
|   |-- PlayStation/                  ← .gitignored
|   |   |-- PlayStationWindow.h
|   |   └-- PlayStationWindow.cpp
|   └-- Nintendo/                     ← .gitignored
|       |-- NintendoWindow.h
|       └-- NintendoWindow.cpp
|
└-- Input/
    |-- Desktop/
    |   |-- DesktopInput.h
    |   |-- DesktopInput.cpp
    |   |-- SDLEventTranslator.h      ← moved from Platform/SDL/
    |   └-- SDLEventTranslator.cpp
    |-- Xbox/
    |   |-- XboxInput.h               ← empty stub
    |   └-- XboxInput.cpp
    |-- PlayStation/                  ← .gitignored
    |   |-- PlayStationInput.h
    |   └-- PlayStationInput.cpp
    └-- Nintendo/                     ← .gitignored
        |-- NintendoInput.h
        └-- NintendoInput.cpp
```

### 1.2 — Platform Factory Update

`Window.cpp` selects the correct implementation using CMake-injected defines. No runtime branching, no string comparison.

```cpp
std::unique_ptr<Window> Window::Create()
{
#if defined(TRINITY_PLATFORM_DESKTOP)
    return std::make_unique<DesktopWindow>();
#elif defined(TRINITY_PLATFORM_XBOX)
    return std::make_unique<XboxWindow>();
#else
    static_assert(false, "No platform defined for Window::Create()");
#endif
}
```

### 1.3 — CMake Platform Defines

```cmake
if(WIN32 OR (UNIX AND NOT APPLE))
    target_compile_definitions(TrinityEngine PUBLIC TRINITY_PLATFORM_DESKTOP)
elseif(APPLE)
    target_compile_definitions(TrinityEngine PUBLIC TRINITY_PLATFORM_DESKTOP)
    target_compile_definitions(TrinityEngine PUBLIC TRINITY_PLATFORM_MACOS)
endif()
```

`TRINITY_PLATFORM_MACOS` is a supplementary define used later by the Vulkan backend to select the MoltenVK surface extension.

### 1.4 — Shader Directory Structure

```
Trinity-Engine/
└-- shaders/
    |-- source/
    |   |-- geometry_pass.vert.glsl
    |   |-- geometry_pass.frag.glsl
    |   |-- shadow_pass.vert.glsl
    |   |-- shadow_pass.frag.glsl
    |   |-- lighting_pass.vert.glsl       ← gap, empty for now
    |   └-- lighting_pass.frag.glsl       ← gap, empty for now
    └-- compiled/                         ← .gitignored, generated by build
        |-- geometry_pass.vert.spv
        |-- geometry_pass.frag.spv
        |-- shadow_pass.vert.spv
        └-- shadow_pass.frag.spv
```

### 1.5 — Shader CMake Compile Step

A `TrinityShaders` custom CMake target compiles every `.glsl` file in `shaders/source/` to `.spv` in `shaders/compiled/` using `Vulkan::glslc` before the engine library builds. `TrinityEngine` depends on `TrinityShaders`.

```cmake
function(trinity_compile_shader SHADER_SOURCE OUTPUT_SPV STAGE)
    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND Vulkan::glslc -fshader-stage = ${STAGE} ${SHADER_SOURCE} -o ${OUTPUT_SPV}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader ${SHADER_SOURCE}"
    )
endfunction()
```

**Milestone:** Project builds cleanly. Window opens. Input works. No renderer yet.

---

## Phase 2 — Abstract Renderer Interface

This phase defines every abstract type the rest of the engine and editor will ever use. After this phase all higher-level code can be written against these interfaces without knowing anything about Vulkan. This is the most important architectural phase of the entire plan.

The interface set defined here is intentionally larger than the original Hazel-derived skeleton. Descriptor sets, command lists, and compute pipelines are first-class abstract types from day one. Adding them later forces a rewrite of every pass, every material, and every backend; declaring them now costs nothing extra and unblocks every later phase.

### 2.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Renderer/
|-- RendererAPI.h/.cpp                    ← top-level backend interface and factory
|-- Renderer.h/.cpp                       ← high-level static facade (what Forge calls)
|-- CommandList.h                         ← abstract recording surface
|-- RenderPass.h                          ← abstract render pass base
|-- Resources/
|   |-- Buffer.h
|   |-- Texture.h
|   |-- Framebuffer.h
|   |-- Shader.h
|   |-- Pipeline.h
|   |-- ComputePipeline.h
|   |-- Sampler.h
|   |-- DescriptorSetLayout.h
|   |-- DescriptorSet.h
|   |-- QueryPool.h
|-- Camera/
|   |-- Camera.h/.cpp                     ← view/projection math, no API knowledge
|   └-- EditorCamera.h/.cpp               ← fly camera for editor viewport
└-- SceneRenderer.h/.cpp                  ← orchestrates all passes for one scene
```

### 2.2 — RendererAPI

```cpp
// RendererAPI.h
#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Texture.h"
#include "Trinity/Renderer/Resources/Framebuffer.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/ComputePipeline.h"
#include "Trinity/Renderer/Resources/Sampler.h"
#include "Trinity/Renderer/Resources/DescriptorSetLayout.h"
#include "Trinity/Renderer/Resources/DescriptorSet.h"
#include "Trinity/Renderer/Resources/QueryPool.h"
#include "Trinity/Renderer/CommandList.h"

#include <cstdint>
#include <memory>
#include <string>

namespace Trinity
{
    class Window;

    enum class RendererBackend : uint8_t
    {
        None = 0,
        Vulkan,
        DirectX12,
        Metal
    };

    enum class QueueType : uint8_t
    {
        Graphics = 0,
        AsyncCompute,
        Transfer
    };

    struct RendererAPISpecification
    {
        uint32_t MaxFramesInFlight = 2;
        bool EnableValidation = false;
        bool EnableBindless = true;
        bool EnableAsyncCompute = true;
        bool EnableTimestampQueries = true;
        std::string PipelineCachePath = "PipelineCache.bin";
    };

    class RendererAPI
    {
    public:
        virtual ~RendererAPI() = default;

        virtual void Initialize(Window& window, const RendererAPISpecification& spec) = 0;
        virtual void Shutdown() = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void Present() = 0;
        virtual void WaitIdle() = 0;

        virtual std::shared_ptr<Buffer> CreateBuffer(const BufferSpecification& spec) = 0;
        virtual std::shared_ptr<Texture> CreateTexture(const TextureSpecification& spec) = 0;
        virtual std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferSpecification& spec) = 0;
        virtual std::shared_ptr<Shader> CreateShader(const ShaderSpecification& spec) = 0;
        virtual std::shared_ptr<Pipeline> CreatePipeline(const PipelineSpecification& spec) = 0;
        virtual std::shared_ptr<ComputePipeline> CreateComputePipeline(const ComputePipelineSpecification& spec) = 0;
        virtual std::shared_ptr<Sampler> CreateSampler(const SamplerSpecification& spec) = 0;
        virtual std::shared_ptr<DescriptorSetLayout> CreateDescriptorSetLayout(const DescriptorSetLayoutSpecification& spec) = 0;
        virtual std::shared_ptr<DescriptorSet> AllocateDescriptorSet(const std::shared_ptr<DescriptorSetLayout>& layout) = 0;
        virtual std::shared_ptr<DescriptorSet> AllocateTransientDescriptorSet(const std::shared_ptr<DescriptorSetLayout>& layout) = 0;
        virtual std::shared_ptr<QueryPool> CreateQueryPool(const QueryPoolSpecification& spec) = 0;

        virtual std::shared_ptr<CommandList> AcquireCommandList(QueueType queue) = 0;
        virtual void SubmitCommandList(const std::shared_ptr<CommandList>& cmd, QueueType queue) = 0;

        virtual void OnWindowResize(uint32_t width, uint32_t height) = 0;
        virtual uint32_t GetSwapchainWidth() const = 0;
        virtual uint32_t GetSwapchainHeight() const = 0;
        virtual uint32_t GetCurrentFrameIndex() const = 0;
        virtual uint32_t GetMaxFramesInFlight() const = 0;
        virtual bool SupportsAsyncCompute() const = 0;
        virtual bool SupportsBindless() const = 0;
        virtual bool SupportsRayTracing() const = 0;
        virtual bool SupportsMeshShaders() const = 0;

        RendererBackend GetBackend() const { return m_Backend; }

        static std::unique_ptr<RendererAPI> Create(RendererBackend backend);

    protected:
        RendererBackend m_Backend = RendererBackend::None;
    };
}
```

### 2.3 — Abstract Resource Types

All specification structs contain only plain data types. No Vulkan types cross this boundary under any circumstance.

**BufferUsage, BufferMemoryType, BufferSpecification, Buffer** — GPU buffer with `SetData`, `Map`, `Unmap`. Usage flags include `Vertex`, `Index`, `Uniform`, `Storage`, `Indirect`, `TransferSrc`, `TransferDst`. `BufferDeviceAddress` is requested when the bindless feature is enabled.

**TextureFormat, TextureUsage, TextureSpecification, Texture** — Image with `GetOpaqueHandle()` returning a `uint64_t` that is only meaningful inside the Vulkan layer. Format set is broad: `RGBA8`, `RGBA8_SRGB`, `BGRA8`, `BGRA8_SRGB`, `RGBA16F`, `RGBA32F`, `RG16F`, `RG32F`, `R8`, `R16F`, `R32F`, `R11G11B10F`, `RGB10A2`, `Depth32F`, `Depth24Stencil8`, plus block-compressed `BC1`–`BC7` and `ASTC_4x4` for cooked content. `Usage` is a bitmask covering `ColorAttachment`, `DepthStencilAttachment`, `Sampled`, `Storage`, `TransferSrc`, `TransferDst`, `InputAttachment`. Mip count, array layer count, sample count, and cube flag are all part of the spec.

**FramebufferAttachmentSpecification, FramebufferSpecification, Framebuffer** — Container of attachment textures. With `VK_KHR_dynamic_rendering` there are no `VkFramebuffer` objects — this type is purely a texture container.

**ShaderStage, ShaderModuleSpecification, ShaderSpecification, Shader** — Loads from `.spv` blob in memory or from path. Stages include `Vertex`, `Fragment`, `Compute`, `Geometry`, `TessControl`, `TessEval`, plus `Task` and `Mesh` reserved for the mesh-shader phase. Each module carries an entry-point string and an optional permutation hash. Shader modules are destroyed after pipeline creation, not held at draw time.

**PrimitiveTopology, CullMode, DepthCompareOp, BlendFactor, BlendOp, ColorWriteMask, VertexAttribute, PipelineSpecification, Pipeline** — Graphics pipeline state. Dynamic viewport and scissor always enabled. The spec carries an array of `DescriptorSetLayout` references (one per set index), an array of push-constant ranges, per-attachment blend state, depth-bias parameters, and conservative-raster flag.

**ComputePipelineSpecification, ComputePipeline** — Single-stage compute pipeline. Carries shader, descriptor set layouts, push constants, and a workgroup size hint used for validation.

**SamplerSpecification, Sampler** — Sampler state with `MagFilter`, `MinFilter`, `MipmapMode`, `AddressU/V/W`, `MaxAnisotropy`, `MinLod`, `MaxLod`, `BorderColor`, `CompareOp`. A cache in `VulkanRendererAPI` prevents duplicate creation.

**DescriptorBindingType, DescriptorBindingFlags, DescriptorSetLayoutBinding, DescriptorSetLayoutSpecification, DescriptorSetLayout** — Describes one set's bindings. Types: `UniformBuffer`, `StorageBuffer`, `SampledImage`, `StorageImage`, `Sampler`, `CombinedImageSampler`, `UniformBufferDynamic`, `StorageBufferDynamic`. Flags include `PartiallyBound`, `UpdateAfterBind`, `VariableDescriptorCount` to support bindless.

**DescriptorSet** — Allocated against a `DescriptorSetLayout`. Exposes `WriteUniformBuffer(binding, buffer, offset, range)`, `WriteStorageBuffer(...)`, `WriteSampledImage(binding, texture, sampler)`, `WriteStorageImage(...)`, plus array-index variants for bindless arrays. Two allocation paths exist: persistent (lives with an asset, e.g. material) and transient (recycled every frame, used for view UBOs and per-pass scratch sets).

**QueryType, QueryPoolSpecification, QueryPool** — `Timestamp`, `Occlusion`, `PipelineStatistics`. The renderer creates one timestamp pool sized for `MaxPasses × MaxFramesInFlight × 2`.

**`CommandList`** — Abstract recording surface. The only object passes ever interact with directly. See section 2.5.

### 2.4 — High-Level Renderer Facade

```cpp
// Renderer.h
#pragma once
#include "Trinity/Renderer/RendererAPI.h"

namespace Trinity
{
    class Window;

    struct RendererSpecification
    {
        RendererBackend Backend = RendererBackend::Vulkan;
        uint32_t MaxFramesInFlight = 2;
        bool EnableValidation = false;
        bool EnableBindless = true;
        bool EnableAsyncCompute = true;
        bool EnableTimestampQueries = true;
        std::string PipelineCachePath = "PipelineCache.bin";
    };

    class Renderer
    {
    public:
        static void Initialize(Window& window, const RendererSpecification& spec);
        static void Shutdown();

        static bool BeginFrame();
        static void EndFrame();
        static void Present();
        static void WaitIdle();

        static void OnWindowResize(uint32_t width, uint32_t height);

        static RendererAPI& GetAPI();
        static RendererBackend GetBackend();
        static uint32_t GetCurrentFrameIndex();
        static uint32_t GetMaxFramesInFlight();

    private:
        static std::unique_ptr<RendererAPI> s_API;
    };
}
```

### 2.5 — CommandList Abstract Type

`CommandList` is the only object passes ever record into. No pass ever pulls a `VkCommandBuffer` directly — the backend specialisation is hidden inside the implementation. This is what permits a future DX12 or Metal backend without rewriting any pass.

```cpp
// CommandList.h
#pragma once

#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/ComputePipeline.h"
#include "Trinity/Renderer/Resources/DescriptorSet.h"
#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    struct RenderingAttachment
    {
        std::shared_ptr<Texture> ColorTexture;
        bool ClearOnLoad = true;
        float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct DepthAttachment
    {
        std::shared_ptr<Texture> DepthTexture;
        bool ClearOnLoad = true;
        float ClearDepth = 0.0f;
        uint32_t ClearStencil = 0;
    };

    struct RenderingInfo
    {
        std::vector<RenderingAttachment> ColorAttachments;
        DepthAttachment Depth;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };

    enum class ResourceState : uint8_t
    {
        Undefined = 0,
        ColorAttachment,
        DepthStencilAttachment,
        DepthStencilRead,
        ShaderRead,
        ShaderWrite,
        TransferSrc,
        TransferDst,
        Present
    };

    class CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void BeginRendering(const RenderingInfo& info) = 0;
        virtual void EndRendering() = 0;

        virtual void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) = 0;
        virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

        virtual void BindPipeline(const std::shared_ptr<Pipeline>& pipeline) = 0;
        virtual void BindComputePipeline(const std::shared_ptr<ComputePipeline>& pipeline) = 0;
        virtual void BindDescriptorSet(uint32_t setIndex, const std::shared_ptr<DescriptorSet>& set, uint32_t dynamicOffsetCount = 0, const uint32_t* dynamicOffsets = nullptr) = 0;
        virtual void PushConstants(uint32_t offset, uint32_t size, const void* data) = 0;
        virtual void BindVertexBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(const std::shared_ptr<Buffer>& buffer, uint64_t offset = 0, bool indexType32Bit = true) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawIndexedIndirectCount(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, const std::shared_ptr<Buffer>& countBuffer, uint64_t countOffset, uint32_t maxDraws, uint32_t stride) = 0;

        virtual void Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = 0;
        virtual void DispatchIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset) = 0;

        virtual void TransitionResource(const std::shared_ptr<Texture>& texture, ResourceState before, ResourceState after) = 0;
        virtual void BufferBarrier(const std::shared_ptr<Buffer>& buffer, ResourceState before, ResourceState after) = 0;

        virtual void CopyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, uint64_t srcOffset, uint64_t dstOffset, uint64_t size) = 0;
        virtual void CopyBufferToTexture(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Texture>& dst, uint32_t mipLevel, uint32_t arrayLayer) = 0;
        virtual void BlitTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst) = 0;
        virtual void GenerateMips(const std::shared_ptr<Texture>& texture) = 0;

        virtual void WriteTimestamp(const std::shared_ptr<QueryPool>& pool, uint32_t index) = 0;
        virtual void ResetQueryPool(const std::shared_ptr<QueryPool>& pool, uint32_t firstIndex, uint32_t count) = 0;

        virtual void BeginDebugLabel(const std::string& name, float color[4]) = 0;
        virtual void EndDebugLabel() = 0;
        virtual void InsertDebugMarker(const std::string& name) = 0;
    };
}
```

### 2.6 — RendererAPI Factory

```cpp
std::unique_ptr<RendererAPI> RendererAPI::Create(RendererBackend backend)
{
#if defined(TRINITY_RENDERER_VULKAN)
    if (backend == RendererBackend::Vulkan)
        return std::make_unique<VulkanRendererAPI>();
#endif
#if defined(TRINITY_RENDERER_DIRECTX12)
    if (backend == RendererBackend::DirectX12)
        return std::make_unique<DirectX12RendererAPI>();
#endif
    TR_CORE_CRITICAL("Requested renderer backend is not compiled into this build.");
    std::abort();
}
```

### 2.7 — SceneRenderer

The top-level pass orchestrator. Uses pImpl so its header contains zero Vulkan includes. Owns a `RenderPipeline` description that lists which passes are active and their order; passes are constructed from this description each time it changes. Inline lambda passes are explicitly forbidden — every pass is a class implementing `IRenderPass`.

```cpp
// SceneRenderer.h
#pragma once
#include "Trinity/Renderer/Camera/Camera.h"
#include <memory>
#include <vector>

namespace Trinity
{
    class Texture;
    class Mesh;
    class Material;

    struct MeshDrawCommand
    {
        std::shared_ptr<Mesh> MeshRef;
        std::shared_ptr<Material> MaterialRef;
        float Transform[16] = {};
        uint32_t LayerMask = 0xFFFFFFFFu;
        bool CastShadow = true;
        bool ReceiveShadow = true;
    };

    struct DirectionalLight
    {
        float Direction[3] = { 0.0f, -1.0f, 0.0f };
        float Intensity = 1.0f;
        float Color[3] = { 1.0f, 1.0f, 1.0f };
        bool CastShadow = true;
    };

    struct PointLight
    {
        float Position[3] = {};
        float Color[3] = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Range = 10.0f;
        bool CastShadow = false;
    };

    struct SpotLight
    {
        float Position[3] = {};
        float Direction[3] = { 0.0f, -1.0f, 0.0f };
        float Color[3] = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Range = 10.0f;
        float InnerConeRadians = 0.5f;
        float OuterConeRadians = 0.7f;
        bool CastShadow = false;
    };

    struct EnvironmentLighting
    {
        std::shared_ptr<Texture> SkyboxCube;
        std::shared_ptr<Texture> IrradianceCube;
        std::shared_ptr<Texture> SpecularCube;
        std::shared_ptr<Texture> BrdfLut;
        float SkyIntensity = 1.0f;
    };

    struct SceneRenderData
    {
        DirectionalLight SunLight;
        std::vector<PointLight> PointLights;
        std::vector<SpotLight> SpotLights;
        EnvironmentLighting Environment;
    };

    enum class AntiAliasingMode : uint8_t { None, FXAA, TAA, FSR2 };

    struct RenderPipelineSettings
    {
        bool LightingEnabled = true;
        bool SsaoEnabled = true;
        bool BloomEnabled = true;
        bool TaaEnabled = true;
        bool VolumetricsEnabled = false;
        bool SsrEnabled = false;
        bool RayTracedShadowsEnabled = false;
        AntiAliasingMode AntiAliasing = AntiAliasingMode::TAA;
        uint32_t ShadowCascadeCount = 4;
        uint32_t ShadowMapResolution = 2048;
    };

    struct SceneRendererStats
    {
        uint32_t DrawCalls = 0;
        uint32_t VertexCount = 0;
        uint32_t IndexCount = 0;
        uint32_t MeshesSubmitted = 0;
        uint32_t MeshesCulled = 0;
        uint32_t LightsSubmitted = 0;
        uint32_t LightsCulled = 0;
        float ShadowPassMs = 0.0f;
        float GeometryPassMs = 0.0f;
        float LightingPassMs = 0.0f;
        float SsaoPassMs = 0.0f;
        float BloomPassMs = 0.0f;
        float PostProcessMs = 0.0f;
        float TotalGPUMs = 0.0f;
        uint32_t GeometryBufferMemoryMB = 0;
        uint32_t TransientMemoryMB = 0;
    };

    class SceneRenderer
    {
    public:
        SceneRenderer();
        ~SceneRenderer();

        void Initialize(uint32_t width, uint32_t height);
        void Shutdown();

        void SetSettings(const RenderPipelineSettings& settings);
        const RenderPipelineSettings& GetSettings() const;

        void BeginScene(const Camera& camera, const SceneRenderData& sceneData);
        void SubmitMesh(const MeshDrawCommand& command);
        void EndScene();

        void Render();
        void OnResize(uint32_t width, uint32_t height);

        std::shared_ptr<Texture> GetFinalOutput() const;
        std::shared_ptr<Texture> GetGBufferAlbedo() const;
        std::shared_ptr<Texture> GetGBufferNormal() const;
        std::shared_ptr<Texture> GetGBufferMaterial() const;
        std::shared_ptr<Texture> GetGBufferDepth() const;
        std::shared_ptr<Texture> GetShadowMap(uint32_t cascade) const;
        const SceneRendererStats& GetStats() const;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> m_Implementation;

        std::vector<MeshDrawCommand> m_DrawList;
        Camera m_Camera;
        SceneRenderData m_SceneData;
        RenderPipelineSettings m_Settings;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };
}
```

**Milestone:** All abstract types compile. `Renderer::Initialize` can be called from `Application`. Headers contain zero Vulkan includes. Nothing renders yet.

---
## Phase 3 — Vulkan Backend Skeleton

Creates the Vulkan-specific implementation of every abstract interface from Phase 2. After this phase the backend initialises, creates a device with full feature set, creates separated graphics + async compute + transfer queues, creates a swap chain, loads a persistent pipeline cache from disk, and shuts down cleanly.

### 3.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Renderer/Vulkan/
|-- VulkanRendererAPI.h/.cpp
|-- VulkanDevice.h/.cpp
|-- VulkanSwapchain.h/.cpp
|-- VulkanAllocator.h/.cpp                ← VMA wrapper
|-- VulkanCommandPool.h/.cpp              ← per-queue, per-frame pools
|-- VulkanCommandList.h/.cpp              ← CommandList implementation
|-- VulkanDescriptorAllocator.h/.cpp      ← persistent + transient pools
|-- VulkanPipelineCache.h/.cpp            ← VkPipelineCache disk persistence
|-- VulkanSyncObjects.h/.cpp
|-- VulkanDebug.h/.cpp                    ← debug-utils labels, named objects
|-- VulkanQueryPool.h/.cpp
|-- VulkanUploadQueue.h/.cpp              ← batched async upload via transfer queue
|-- Resources/
|   |-- VulkanBuffer.h/.cpp
|   |-- VulkanTexture.h/.cpp
|   |-- VulkanFramebuffer.h/.cpp
|   |-- VulkanShader.h/.cpp
|   |-- VulkanPipeline.h/.cpp
|   |-- VulkanComputePipeline.h/.cpp
|   |-- VulkanSampler.h/.cpp
|   |-- VulkanDescriptorSetLayout.h/.cpp
|   └-- VulkanDescriptorSet.h/.cpp
|-- RenderGraph/
|   |-- VulkanRenderGraph.h/.cpp
|   └-- VulkanTransientPool.h/.cpp        ← aliased VkImage memory
└-- VulkanUtilities.h/.cpp
```

**Critical rule:** Nothing outside `Trinity/Renderer/Vulkan/` ever includes a Vulkan header.

### 3.2 — VulkanDevice

Enumerates physical devices, selects one (prefer discrete GPU), creates logical device, retrieves queue handles for every queue family the engine needs.

Vulkan minimum: 1.3.

Required core features:
- `dynamicRendering`
- `synchronization2`
- `bufferDeviceAddress`
- `descriptorIndexing` plus `descriptorBindingSampledImageUpdateAfterBind`, `descriptorBindingPartiallyBound`, `runtimeDescriptorArray`, `shaderSampledImageArrayNonUniformIndexing`
- `timelineSemaphore`
- `scalarBlockLayout`
- `shaderInt64`
- `samplerAnisotropy`
- `multiDrawIndirect`, `drawIndirectFirstInstance`, `drawIndirectCount`

Optional features detected and stored on the device for later phases:
- `VK_KHR_ray_tracing_pipeline`, `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`
- `VK_EXT_mesh_shader`
- `VK_KHR_fragment_shading_rate`
- `VK_EXT_swapchain_colorspace` (HDR display output)

The device creates three queue indices when available: a graphics+present queue, a dedicated async compute queue (if a separate compute family exists), and a dedicated transfer queue (if a separate transfer family exists). When dedicated families are unavailable, queues fall back to the graphics queue and `SupportsAsyncCompute()` returns false.

### 3.3 — VulkanSwapchain

Creates swap chain, acquires images, manages image views, handles resize by destroying and recreating. Uses `VK_KHR_dynamic_rendering` so no `VkRenderPass` objects exist here.

Default format: `VK_FORMAT_B8G8R8A8_UNORM` with `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`. Wide-gamut and HDR colour spaces (`VK_COLOR_SPACE_HDR10_ST2084_EXT`, `VK_COLOR_SPACE_BT2020_LINEAR_EXT`) are detected and exposed via a property; HDR display output is wired in a later phase.

Present mode preference: `VK_PRESENT_MODE_MAILBOX_KHR` falling back to `VK_PRESENT_MODE_FIFO_KHR`. Always creates a matching depth image in `VK_FORMAT_D32_SFLOAT`.

### 3.4 — VulkanAllocator

Thin wrapper around VMA. All `VkBuffer` and `VkImage` allocations go through this. Three named pools are created: `Persistent` (long-lived assets), `Transient` (per-frame intermediates, used by render graph aliasing), `Upload` (host-visible staging). No code outside the Vulkan layer ever calls VMA directly. A `GetMemoryStats` API exposes used / budget / fragmentation per pool for the editor stats panel.

### 3.5 — VulkanCommandPool

Manages per-queue, per-frame command buffer pools. For each queue family the engine uses (graphics, async compute, transfer), `MaxFramesInFlight` pools are created. Acquiring a command list resets and returns a fresh primary buffer from the appropriate pool. Secondary command buffers are reserved for the multi-threaded recording phase.

### 3.6 — VulkanDescriptorAllocator

Two-tier allocator:

- **Persistent pool** — large `VkDescriptorPool` instances grown on demand. Used for material descriptor sets and other long-lived bindings. Each allocation is tracked so `Free` reclaims slots.
- **Transient ring** — one `VkDescriptorPool` per frame in flight, reset entirely at frame begin. Used for per-pass scratch sets, view UBOs, and any binding that does not outlive the frame.

A bindless heap (single set with a `VariableDescriptorCount` array of `MaxBindlessTextures = 16384`) is allocated up front when `EnableBindless` is true. Texture registration writes into a free index of this heap; the renderer hands out the integer index to shaders via push constant or storage buffer.

### 3.7 — VulkanPipelineCache

Loads `VkPipelineCache` from `PipelineCachePath` at startup, passes it to every `vkCreate*Pipelines` call, and writes it back to disk on shutdown. Cold-start hitches caused by driver compilation are eliminated after the first run. The header magic is validated; mismatched UUID resets the cache file.

### 3.8 — VulkanSyncObjects

Per frame in flight:
- `VkSemaphore ImageAvailableSemaphore` — signalled when swap chain image is ready
- `VkSemaphore RenderFinishedSemaphore` — signalled when the graphics queue finishes the frame
- `VkFence InFlightFence` — CPU waits on this before reusing the frame's resources

Plus persistent timeline semaphores for cross-queue synchronisation between graphics and async compute. Timeline values are tracked inside `VulkanRendererAPI` and used by the render graph when scheduling async compute passes.

### 3.9 — VulkanRendererAPI Key Methods

**`BeginFrame()`** — Wait on `InFlightFence`, reset transient descriptor pool for the current frame, acquire next swap chain image, reset graphics command pool, write the start timestamp into the per-frame query pool. Returns `false` if swap chain is out of date.

**`EndFrame()`** — Write the end timestamp, end command buffer recording, transition swap chain image to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`, submit with semaphore synchronisation. Read back the previous-frame timestamps and populate `SceneRendererStats`.

**`Present()`** — `vkQueuePresentKHR`. Handles `VK_ERROR_OUT_OF_DATE_KHR` and `VK_SUBOPTIMAL_KHR` by triggering swap chain recreation.

**`WaitIdle()`** — Unconditional `vkDeviceWaitIdle()`. The first thing `Shutdown()` calls. This is the permanent fix for the GPU-CPU sync race that existed in the previous codebase.

**Milestone:** Vulkan initialises and shuts down cleanly. Validation layers are silent. Window displays a clear colour. CPU-GPU synchronisation is correct. Pipeline cache is persisted. All optional features are detected and reported via `SupportsX()` query methods.

---
## Phase 4 — Render Graph

Implements the render graph before any passes are written, because passes declare themselves in terms of the graph's resource API. The render graph is the centrepiece of the renderer's hot path — every barrier the engine emits is computed by it, never written by hand.

### 4.1 — Design

The render graph does five things. During **declaration**, each pass states what textures and buffers it reads and writes — no Vulkan calls happen. During **compilation**, the graph builds a dependency-ordered pass list, computes resource lifetimes, allocates physical resources from the transient pool with aliasing for non-overlapping lifetimes, and determines the exact `VkImageMemoryBarrier2` between every pair of passes that share a resource. During **scheduling**, passes flagged for async compute are emitted onto the compute queue with timeline semaphore sync against the graphics queue. During **execution**, the graph walks the ordered list, inserts compiled barriers, opens debug labels, writes timestamp queries, and invokes each pass's record callback. After **execution**, statistics from the previous frame's queries are read back and accumulated.

The graph is rebuilt every time the active pass set changes (settings toggled, viewport resized, lighting mode changed). Recompilation is amortised by hashing the structural shape of the graph — declarations with the same shape reuse the previous compile result.

### 4.2 — Folder Structure

```
Trinity-Engine/src/Trinity/Renderer/RenderGraph/
|-- RenderGraph.h/.cpp
|-- RenderGraphPass.h/.cpp
|-- RenderGraphResource.h
|-- RenderGraphContext.h
└-- IRenderPass.h                         ← abstract pass class
```

### 4.3 — RenderGraphResource

A lightweight handle — not a real `VkImage`. A name and description of what the resource will be. The graph allocates real textures during compilation, and aliases memory between resources whose lifetimes do not overlap.

```cpp
struct RenderGraphTextureDescription
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t MipLevels = 1;
    uint32_t ArrayLayers = 1;
    uint32_t SampleCount = 1;
    TextureFormat Format = TextureFormat::None;
    TextureUsage Usage = TextureUsage::None;
    bool MatchSwapchainSize = true;
    bool Persistent = false;
    std::string DebugName;
};

struct RenderGraphBufferDescription
{
    uint64_t Size = 0;
    BufferUsage Usage = BufferUsage::None;
    bool Persistent = false;
    std::string DebugName;
};

struct RenderGraphResourceHandle
{
    uint32_t Index = UINT32_MAX;
    bool IsValid() const { return Index != UINT32_MAX; }
};
```

`Persistent = true` opts a resource out of aliasing and out of frame recycling — used for shadow atlases, history buffers (TAA, SSR), and the IBL cubemap chain.

### 4.4 — RenderGraphPass

```cpp
enum class RenderPassQueue : uint8_t
{
    Graphics = 0,
    AsyncCompute
};

class RenderGraphPass
{
public:
    RenderGraphPass& Read(RenderGraphResourceHandle resource);
    RenderGraphPass& ReadWrite(RenderGraphResourceHandle resource);
    RenderGraphPass& Write(RenderGraphResourceHandle resource);
    RenderGraphPass& WriteColor(RenderGraphResourceHandle resource, uint32_t slot);
    RenderGraphPass& WriteDepth(RenderGraphResourceHandle resource);
    RenderGraphPass& OnQueue(RenderPassQueue queue);
    RenderGraphPass& SetExecuteCallback(std::function<void(CommandList&, RenderGraphContext&)> callback);

private:
    std::string m_Name;
    RenderPassQueue m_Queue = RenderPassQueue::Graphics;
    std::vector<RenderGraphResourceHandle> m_Reads;
    std::vector<RenderGraphResourceHandle> m_ReadWrites;
    std::vector<RenderGraphResourceHandle> m_Writes;
    std::vector<std::pair<RenderGraphResourceHandle, uint32_t>> m_ColorWrites;
    RenderGraphResourceHandle m_DepthWrite;
    std::function<void(CommandList&, RenderGraphContext&)> m_ExecuteCallback;
};
```

The execute callback receives a `CommandList` reference and the context. It records draw calls. It does not record barriers — those are the graph's responsibility.

### 4.5 — RenderGraph

```cpp
class RenderGraph
{
public:
    RenderGraphResourceHandle DeclareTexture(const RenderGraphTextureDescription& desc);
    RenderGraphResourceHandle DeclareBuffer(const RenderGraphBufferDescription& desc);
    RenderGraphResourceHandle ImportTexture(const std::shared_ptr<Texture>& texture, const std::string& debugName);
    RenderGraphResourceHandle ImportBuffer(const std::shared_ptr<Buffer>& buffer, const std::string& debugName);

    RenderGraphPass& AddPass(const std::string& name);

    void Compile();
    void Execute();
    void OnResize(uint32_t width, uint32_t height);
    void Reset();

    std::shared_ptr<Texture> GetTexture(RenderGraphResourceHandle handle) const;
    std::shared_ptr<Buffer> GetBuffer(RenderGraphResourceHandle handle) const;

    uint32_t GetTransientMemoryUsageMB() const;

private:
    struct ResourceLifetime { uint32_t FirstUse; uint32_t LastUse; };
    bool m_Compiled = false;
    std::vector<RenderGraphTextureDescription> m_TextureDescs;
    std::vector<RenderGraphBufferDescription> m_BufferDescs;
    std::vector<std::shared_ptr<Texture>> m_Textures;
    std::vector<std::shared_ptr<Buffer>> m_Buffers;
    std::vector<RenderGraphPass> m_Passes;
    std::vector<uint32_t> m_ExecutionOrder;
    std::vector<ResourceLifetime> m_Lifetimes;
    uint64_t m_StructureHash = 0;
};
```

`VulkanRenderGraph` subclasses `RenderGraph` and implements: physical resource allocation through `VulkanTransientPool`, barrier compilation using `VkImageMemoryBarrier2`, async-compute scheduling via timeline semaphores, debug label emission via `VK_EXT_debug_utils`, and timestamp queries around every pass. The abstract graph layer knows nothing about barriers.

### 4.6 — Transient Pool and Aliasing

`VulkanTransientPool` allocates a small number of large `VkDeviceMemory` blocks at startup and sub-allocates `VkImage` objects from them using `vkBindImageMemory` with computed offsets. During graph compilation, a first-fit allocator places resources into the pool such that any two resources sharing memory have non-overlapping lifetimes. Aliased resources receive an explicit aliasing barrier when their slot is reused.

Memory savings on a typical AAA frame are 3–5×: SSAO intermediates, bloom mip chain, and TAA scratch can all share a single 32 MB block.

### 4.7 — IRenderPass

Inline lambda passes are forbidden. Every pass in the engine derives from `IRenderPass`:

```cpp
class IRenderPass
{
public:
    virtual ~IRenderPass() = default;
    virtual const char* GetName() const = 0;
    virtual void Setup(RenderGraph& graph, const FrameContext& frame) = 0;
    virtual void Execute(CommandList& cmd, RenderGraphContext& ctx) = 0;
    virtual bool IsEnabled(const RenderPipelineSettings& settings) const { return true; }
};
```

`SceneRenderer` holds an ordered list of `unique_ptr<IRenderPass>`. Each frame: walk the list, for each enabled pass call `Setup` to declare its dependencies, then call `graph.Compile()` once, then `graph.Execute()`. Resources captured by `Setup` are passed into `Execute` via `RenderGraphContext`.

**Milestone:** Render graph compiles and executes passes with correct barriers across two queues. Validated with a multi-pass test that includes one async-compute pass writing to a buffer that a subsequent graphics pass reads. Transient pool reports memory aliasing savings. Timestamp queries report per-pass GPU time.

---
## Phase 5 — Vulkan Resource Implementations

Fills in the concrete Vulkan implementations of every abstract resource type from Phase 2. This phase comes before pass implementation because passes need every resource type fully working.

### 5.1 — VulkanBuffer

Wraps `VkBuffer` + `VmaAllocation`. Maps abstract `BufferUsage` flags to `VkBufferUsageFlags` plus `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT` when bindless or buffer-device-address-using paths request it. `SetData` performs a direct mapped write for `CPUToGPU` buffers or hands the upload to `VulkanUploadQueue` for `GPU` buffers (asynchronous on the transfer queue when available, synchronous fallback otherwise).

### 5.2 — VulkanTexture

Wraps `VkImage` + `VkImageView` + `VmaAllocation`. All image creation goes through `VulkanAllocator`. `GetOpaqueHandle()` returns `VkImageView` cast to `uint64_t` for ImGui integration. Nothing outside the Vulkan layer casts this back. Supports mipped, arrayed, cube-mapped, and multisampled textures. Block-compressed formats (`BC1`–`BC7`, `ASTC_4x4`) skip the mip generation path because cooked textures arrive with mips embedded.

`GenerateMips` on the command list issues blit chain operations and emits the layout transitions inline.

### 5.3 — VulkanFramebuffer

With `VK_KHR_dynamic_rendering` there are no `VkFramebuffer` objects. `VulkanFramebuffer` is a container of `VulkanTexture` attachment instances. The `BeginRendering` call on `CommandList` builds `VkRenderingAttachmentInfo` from these textures inline.

### 5.4 — VulkanShader

Loads `.spv` from blob in memory or from a path. Creates one `VkShaderModule` per stage. SPIRV-Cross runs at construction to extract reflection data: descriptor set layouts, push constant ranges, vertex inputs, specialisation constants, and entry points. The reflection result is cached and surfaced via `Shader::GetReflection()`. Pipelines built from the shader can either consume the reflected layouts directly (zero-config path) or override them (bindless path overrides set 0 with the global bindless heap).

Shader modules are destroyed after pipeline creation — they are not needed at draw time.

### 5.5 — VulkanPipeline

Creates `VkPipeline` from `PipelineSpecification`. Uses `VkDynamicState` for viewport, scissor, and depth-bias so no pipeline recreation is needed on resize or for shadow bias tweaks. Uses `VkPipelineRenderingCreateInfo` instead of `VkRenderPass`. Consumes `DescriptorSetLayout` references from the spec to build a `VkPipelineLayout`. Always passed to `vkCreateGraphicsPipelines` with the loaded `VkPipelineCache`.

### 5.6 — VulkanComputePipeline

Mirror of `VulkanPipeline` for compute. Single shader stage. Uses the same descriptor + push constant pattern. Always passed to `vkCreateComputePipelines` with the pipeline cache.

### 5.7 — VulkanSampler

Wraps `VkSampler`. A sampler cache keyed on `SamplerSpecification` lives in `VulkanRendererAPI` and is used by all `VulkanSampler` constructions to prevent duplicate creation.

### 5.8 — VulkanDescriptorSetLayout

Wraps `VkDescriptorSetLayout` built from `DescriptorSetLayoutSpecification`. Layouts are cached in `VulkanRendererAPI` keyed on the binding set so that two specs with identical bindings produce one underlying object.

### 5.9 — VulkanDescriptorSet

Wraps `VkDescriptorSet` allocated by `VulkanDescriptorAllocator`. The `Write*` methods build `VkWriteDescriptorSet` entries and batch them; the actual `vkUpdateDescriptorSets` call happens lazily before the next bind, or eagerly via `Flush()`.

### 5.10 — VulkanQueryPool

Wraps `VkQueryPool`. Implements timestamp, occlusion, and pipeline-statistics query pools. Per-frame buffering is built in: writes target the current frame's slice, reads come from the previous frame's slice.

### 5.11 — VulkanUploadQueue

A batched, ring-buffered staging system on the dedicated transfer queue. Calls to `Buffer::SetData` and `Texture::Upload` for GPU-resident resources enqueue a copy command and a staging-buffer slice. `Flush` is implicit at frame begin and explicit on demand for synchronous loads. Timeline semaphores synchronise transfer-queue completions with the consuming queue.

**Milestone:** All resource types create and destroy without validation errors. VMA reports no leaks. Descriptor allocator round-trips persistent and transient sets across multiple frames without exhausting any pool. Upload queue successfully streams a 256 MB texture asynchronously while frames continue rendering.

---
## Phase 6 — Descriptor Convention and Material System

Establishes the binding conventions every shader and pass in the engine will follow, and defines the material asset and runtime material instance. Without this phase, every pass invents its own descriptor layout and the engine cannot cleanly render textured content.

### 6.1 — Descriptor Set Convention

Every graphics pipeline in the engine uses a fixed set numbering scheme. Compute pipelines use the same scheme, except set 1 (per-object) is replaced with compute-specific resources.

| Set | Lifetime | Contents |
|---|---|---|
| 0 | Per frame | Frame UBO (view, proj, view-proj, inverse, time, jitter, exposure, sun direction/intensity, cascade matrices, fog params), light buffer SSBO, cluster grid SSBO, environment IBL textures, shadow atlas |
| 1 | Per object | Object UBO (model matrix, prev model matrix, custom data) — bound once per draw via dynamic offset |
| 2 | Per material | Material parameter UBO + material textures, persistent allocation |
| 3 | Per pass | Pass-specific scratch (e.g. SSAO history, bloom mip slot) |

The bindless heap, when enabled, replaces set 0's texture bindings with a single variable-count array; texture references in materials become 32-bit indices into the heap.

### 6.2 — Frame UBO

```cpp
struct FrameUBO
{
    glm::mat4 View;
    glm::mat4 Projection;
    glm::mat4 ViewProjection;
    glm::mat4 InverseView;
    glm::mat4 InverseProjection;
    glm::mat4 InverseViewProjection;
    glm::mat4 PreviousViewProjection;
    glm::vec4 CameraPosition;
    glm::vec4 CameraDirection;
    glm::vec4 ScreenSize;
    glm::vec4 Jitter;
    glm::vec4 SunDirection;
    glm::vec4 SunColor;
    glm::vec4 AmbientColor;
    float Time;
    float DeltaTime;
    float Exposure;
    float NearPlane;
    float FarPlane;
    uint32_t FrameIndex;
    uint32_t PointLightCount;
    uint32_t SpotLightCount;
};
```

Updated by `SceneRenderer::BeginScene` into a transient host-visible buffer, bound at set=0 binding=0. Every shader that needs view information reads from this.

### 6.3 — Material Asset

```cpp
class MaterialAsset
{
public:
    UUID Id;
    std::string Name;
    UUID ShaderId;
    BlendMode Blend = BlendMode::Opaque;
    bool DoubleSided = false;
    bool CastShadow = true;
    bool ReceiveShadow = true;
    std::unordered_map<std::string, MaterialParameter> Parameters;
    std::unordered_map<std::string, UUID> TextureReferences;
};
```

Stored on disk as YAML alongside its shader. Shader reflection populates the parameter list at import time so the editor inspector knows what to expose.

### 6.4 — Runtime Material

```cpp
class Material
{
public:
    Material(const std::shared_ptr<MaterialAsset>& asset);
    void SetParameter(const std::string& name, const MaterialValue& value);
    void SetTexture(const std::string& name, const std::shared_ptr<Texture>& texture);
    void Bind(CommandList& cmd) const;
    BlendMode GetBlendMode() const;
    bool IsDoubleSided() const;
    uint64_t GetSortKey() const;
private:
    std::shared_ptr<MaterialAsset> m_Asset;
    std::shared_ptr<Buffer> m_ParameterUBO;
    std::shared_ptr<DescriptorSet> m_DescriptorSet;
    std::vector<std::shared_ptr<Texture>> m_Textures;
};
```

Each runtime material owns one persistent descriptor set bound at set=2. Parameter writes mark the UBO dirty; flushes happen lazily before bind. Sort keys combine shader id, blend mode, and material id for front-to-back / back-to-front sorting in passes that need it.

### 6.5 — Material Library

`MaterialLibrary` is a singleton-like service inside `SceneRenderer` that loads, caches, and reloads materials by UUID. Hot-reload of a `.material` file regenerates the runtime material in place without invalidating mesh references.

### 6.6 — Default Materials

The engine ships with three default materials:
- `DefaultLit` — standard PBR opaque, used as fallback for any mesh without an assigned material
- `DefaultUnlit` — straight albedo output, used by editor gizmos
- `DefaultTransparent` — PBR with `BlendMode::Translucent`, used as fallback for transparent meshes

These are built from default shader assets that ship with the engine and are loaded at `Renderer::Initialize`.

**Milestone:** A mesh with an assigned material containing albedo, normal, MR, and AO textures is submitted to the SceneRenderer. The geometry pass binds frame UBO, object UBO, and material descriptor sets correctly. The `geometry_pass.frag.glsl` samples real material textures and writes correct PBR data into the GBuffer. Materials hot-reload without restart.

---
## Phase 7 — Render Pass Architecture, GeometryBuffer, Shadow Pass

Implements the two foundational passes of the deferred pipeline as proper `IRenderPass` classes, replacing any inline lambda implementation. Establishes the convention every later rendering phase will follow.

### 7.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Renderer/Vulkan/Passes/
|-- VulkanGeometryPass.h/.cpp
|-- VulkanShadowPass.h/.cpp
|-- VulkanLightingPass.h/.cpp                   ← Phase 12
|-- VulkanSkyPass.h/.cpp                        ← Phase 12
|-- VulkanBloomPass.h/.cpp                      ← Phase 13
|-- VulkanTonemapPass.h/.cpp                    ← Phase 13
|-- VulkanSsaoPass.h/.cpp                       ← Phase 14
|-- VulkanClusteredLightCullPass.h/.cpp         ← Phase 15
|-- VulkanForwardTransparentPass.h/.cpp         ← Phase 16
|-- VulkanTaaPass.h/.cpp                        ← Phase 38
|-- VulkanSsrPass.h/.cpp                        ← Phase 39
|-- VulkanVolumetricFogPass.h/.cpp              ← Phase 40
|-- VulkanDecalPass.h/.cpp                      ← Phase 41
|-- VulkanRayTracedShadowPass.h/.cpp            ← Phase 46
└-- VulkanGIPass.h/.cpp                         ← Phase 47
```

### 7.2 — GeometryBuffer Layout

| Attachment | Format | Contents |
|---|---|---|
| GBuffer A | `RGBA8_SRGB` | Albedo RGB + opacity flag |
| GBuffer B | `RGBA16F` | World-space Normal RGB + per-pixel material flags packed in A |
| GBuffer C | `RGBA8` | Metallic R + Roughness G + AO B + specular factor A |
| GBuffer D | `R11G11B10F` | Emissive RGB |
| GBuffer Velocity | `RG16F` | Screen-space motion vectors |
| GBuffer Depth | `Depth32F` | Scene depth (reverse-Z) |

All attachments are render-graph transient resources sized to the viewport. The velocity attachment is a deliberate inclusion — it costs little and unblocks TAA and per-object motion blur without touching the pass later.

### 7.3 — Shadow Map Layout

A single 4096×4096 `Depth32F` shadow atlas holds the cascades. Cascades 0–3 occupy quadrants. Point and spot light shadows occupy further sub-rectangles allocated at runtime by the shadow atlas allocator (Phase 11 detail).

### 7.4 — Geometry Pass Shaders

**Vertex shader** transforms to clip space using reverse-Z infinite far projection from the frame UBO. Outputs world-space position, world-space normal, world-space tangent (gap-marked when tangents arrive in Phase 31), UV0, UV1, prev clip-space position for velocity.

**Fragment shader** samples albedo (sRGB), normal (decoded from `RG8` octahedral or `RGB8` tangent-space depending on material variant), metallic-roughness, AO, and emissive textures. Writes all six attachments. The shader is generated per material variant — the permutation system in Phase 9 produces e.g. `geometry_pass.lit.skinned.normalmap.frag.spv` and `geometry_pass.lit.unskinned.frag.spv`.

Push constants carry only an `ObjectIndex` (for per-object UBO offset) and a `MaterialIndex` (for bindless material lookup when bindless is on).

### 7.5 — Shadow Pass Shaders

**Vertex shader** transforms position only using the cascade-local `LightViewProjection` matrix from the frame UBO's cascade array.

**Fragment shader** is empty for opaque shadow casters — depth is written by the fixed-function depth stage. A separate `shadow_pass.alphatested.frag.spv` variant samples the material's albedo alpha channel and discards.

Depth bias is configured via dynamic state (`vkCmdSetDepthBias`) per cascade so the bias scales with cascade size. This eliminates the long-tail acne and Peter Panning that fixed bias produces.

### 7.6 — VulkanGeometryPass

`IRenderPass` implementation. Owns no Vulkan handles — it allocates pipeline + descriptor set layouts in its constructor (cached through `VulkanRendererAPI`) and reuses them for the lifetime of the SceneRenderer. `Setup()` declares the GBuffer attachments as writes. `Execute()` walks the draw list (already culled and sorted by the `RenderQueue` infrastructure), binds material set 2 once per material change, binds object set 1 with dynamic offsets per draw, and issues `DrawIndexed`. When indirect-draw infrastructure lands (Phase 27), the same pass conditionally swaps to `DrawIndexedIndirectCount`.

### 7.7 — VulkanShadowPass

`IRenderPass` implementation. `Setup()` declares the shadow atlas as a write. `Execute()` records depth-only draws for the directional cascades. Per-cascade viewport and depth bias are set via dynamic state. Alpha-tested variants are handled by binding the alpha-tested shadow pipeline and the same material descriptor set used by the geometry pass.

### 7.8 — RenderQueue

A small CPU-side service that takes the SceneRenderer's submitted `MeshDrawCommand` list and produces sorted, culled draw streams per pass. Sorting is by material sort key for opaque, by depth (back-to-front) for transparent, and by light view depth for shadow casters. Frustum culling lands in Phase 26 and is bolted into this service without changing the pass code.

**Milestone:** A scene with a textured mesh and a directional light renders into the GBuffer with all six attachments correctly populated. Cascade 0 of the shadow atlas is populated from the same draw list, alpha-tested shadow casters discard correctly. Output texture displays the GBuffer albedo. Validation is silent. Per-pass GPU time is reported in the stats struct.

---
## Phase 8 — Application Integration

Wires the renderer into the `Application` loop and handles window resize correctly.

### 8.1 — Application Constructor

```cpp
RendererSpecification l_RendererSpec;
l_RendererSpec.Backend = RendererBackend::Vulkan;
l_RendererSpec.MaxFramesInFlight = 2;
l_RendererSpec.EnableBindless = true;
l_RendererSpec.EnableAsyncCompute = true;
l_RendererSpec.EnableTimestampQueries = true;
l_RendererSpec.PipelineCachePath = "Trinity-PipelineCache.bin";
#ifdef TRINITY_DEBUG
l_RendererSpec.EnableValidation = true;
#endif
Renderer::Initialize(*m_Window, l_RendererSpec);
```

### 8.2 — Application Destructor

`Renderer::WaitIdle()` is the unconditional first call. This permanently closes the GPU-CPU destruction race.

```cpp
Application::~Application()
{
    Renderer::WaitIdle();

    m_LayerStack.Shutdown();
    Renderer::Shutdown();
    m_Window->Shutdown();
}
```

### 8.3 — Frame Loop

```cpp
void Application::Run()
{
    while (s_Running)
    {
        Utilities::Time::Update();
        Input::BeginFrame();

        m_Window->OnUpdate();

        auto& a_EventQueue = m_Window->GetEventQueue();
        std::unique_ptr<Event> l_Event;
        while (a_EventQueue.TryPopEvent(l_Event))
            OnEvent(*l_Event);

        if (m_Window->ShouldClose()) { s_Running = false; break; }

        if (m_Window->IsMinimized())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        if (!Renderer::BeginFrame())
            continue;

        const float l_DeltaTime = Utilities::Time::DeltaTime();

        for (auto& it_Layer : m_LayerStack)
            it_Layer->OnUpdate(l_DeltaTime);

        m_ImGuiLayer->Begin();

        for (auto& it_Layer : m_LayerStack)
            it_Layer->OnImGuiRender();

        m_ImGuiLayer->End();

        Renderer::EndFrame();
        Renderer::Present();
    }
}
```

**Milestone:** Full frame loop runs. Resize works. Validation clean. SceneRenderer renders a fully shaded GBuffer from `ForgeLayer`.

---
## Phase 9 — Shader Build System and Hot-Reload

Completes the shader compilation pipeline so it is fully automatic, incremental, supports permutations, and reloads at runtime in editor builds.

### 9.1 — CMake Shader Target

A `TrinityShaders` custom target compiles every `.glsl` file in `shaders/source/` to `.spv` in `shaders/compiled/` using `Vulkan::glslc`. `TrinityEngine` depends on `TrinityShaders`.

```cmake
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source)
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compiled)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(COMPILED_SHADERS "")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)

    if(SHADER MATCHES "\\.vert\\.glsl$")
        set(STAGE vertex)
        string(REPLACE ".vert.glsl" ".vert.spv" OUT_NAME "${SHADER_NAME}${SHADER_EXT}")
    elseif(SHADER MATCHES "\\.frag\\.glsl$")
        set(STAGE fragment)
        string(REPLACE ".frag.glsl" ".frag.spv" OUT_NAME "${SHADER_NAME}${SHADER_EXT}")
    elseif(SHADER MATCHES "\\.comp\\.glsl$")
        set(STAGE compute)
        string(REPLACE ".comp.glsl" ".comp.spv" OUT_NAME "${SHADER_NAME}${SHADER_EXT}")
    endif()

    set(OUTPUT_PATH ${SHADER_OUTPUT_DIR}/${OUT_NAME})
    add_custom_command(
        OUTPUT ${OUTPUT_PATH}
        COMMAND Vulkan::glslc -fshader-stage=${STAGE} ${SHADER} -o ${OUTPUT_PATH} -O
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME}${SHADER_EXT}"
    )
    list(APPEND COMPILED_SHADERS ${OUTPUT_PATH})
endforeach()

add_custom_target(TrinityShaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(TrinityEngine TrinityShaders)
```

### 9.2 — Runtime Shader Loading

The compiled shader directory is injected as a compile definition:

```cmake
target_compile_definitions(TrinityEngine PRIVATE TRINITY_SHADER_DIR="${SHADER_OUTPUT_DIR}/")
```

`VulkanShader` constructs the full path as `std::string(TRINITY_SHADER_DIR) + relativePath` and reads the `.spv` binary with standard file I/O.

### 9.3 — Shader Permutation System

`ShaderLibrary` is the engine-side service that produces shader variants. Each base shader declares its permutation axes in a `.shaderdef` sidecar file:

```yaml
shader: geometry_pass
stages: [vertex, fragment]
permutations:
  - SKINNED: [0, 1]
  - NORMAL_MAP: [0, 1]
  - ALPHA_TEST: [0, 1]
```

The build step expands the cross-product into `geometry_pass.skinned1_normalmap1_alphatest0.frag.spv` files. At runtime, `ShaderLibrary::Get(name, defines)` hashes the active define set and looks up the cached SPIR-V blob, falling back to runtime compilation via in-process shaderc when an editor-only define combination is requested.

### 9.4 — Editor-Time Hot-Reload

In `TRINITY_DEBUG` and `TRINITY_EDITOR` builds, `ShaderHotReloader` runs a file-watcher thread on `shaders/source/`. When a `.glsl` file changes, the affected base shader is recompiled in-process via shaderc, all permutations are rebuilt, and every pipeline that referenced any affected SPIR-V module is rebuilt against the new module. The swap happens at frame begin so no in-flight command buffer references a destroyed pipeline. Shader compile errors are logged and the previous valid binary is retained — the editor never crashes from a typo in a shader.

### 9.5 — Runtime Loading from Cooked Asset

In shipped builds, shaders are loaded from the cooked asset blob produced by `ShaderCooker` (Phase 37). `VulkanShader` accepts an in-memory blob directly so no filesystem access is required at runtime.

**Milestone:** Clean build from scratch compiles all shaders and all permutations automatically. Modifying a `.glsl` file in editor mode triggers in-process recompilation, dependent pipelines are rebuilt within one frame, and the next frame renders with the new shader. A shipped build loads shaders entirely from the cooked asset blob.

---
## Phase 10 — Frame Constants, Reverse-Z, and GPU Timing

Closes the renderer's foundational hygiene gaps before any feature pass is added on top. Each item below is small in isolation but each is a common source of intractable bugs and visual artefacts when retrofitted later.

### 10.1 — Reverse-Z Infinite Far Plane

Switch the engine project-wide to reverse-Z. `DepthCompareOp` defaults to `Greater`. Depth clear value defaults to `0.0`. The `Camera` class produces an infinite reverse-Z projection matrix:

```cpp
glm::mat4 Camera::ComputeReverseZProjection(float fovYRadians, float aspect, float nearPlane)
{
    const float f = 1.0f / std::tan(fovYRadians * 0.5f);
    glm::mat4 m(0.0f);
    m[0][0] = f / aspect;
    m[1][1] = f;
    m[2][2] = 0.0f;
    m[2][3] = -1.0f;
    m[3][2] = nearPlane;
    return m;
}
```

ImGuizmo's projection-space Y-flip remains; the gizmo path additionally converts to a finite forward-Z projection for ImGuizmo's expected NDC.

### 10.2 — GPU Timestamp Queries Wired Through

`VulkanRenderGraph` writes a timestamp at the start and end of every pass into a per-frame slice of a 256-entry timestamp pool. Read-back happens at the start of frame N+2 (avoiding any wait), values are converted using `vkPhysicalDeviceLimits::timestampPeriod`, and individual pass times are written into `SceneRendererStats`. The stats panel (Phase 23) renders these as a stacked bar chart.

### 10.3 — Debug-Utils Labels Everywhere

Every command list automatically opens a debug label region for each pass with the pass name and a colour derived from the pass's hash. Every long-lived Vulkan object created through `VulkanRendererAPI` is named via `vkSetDebugUtilsObjectNameEXT` using the spec's `DebugName`. RenderDoc and Pix captures show a clean, readable hierarchy.

### 10.4 — Per-Frame Object Buffer

A single per-frame ring `Buffer` of size `MaxObjectsPerFrame × sizeof(ObjectUBO)` is allocated in host-visible memory. `SceneRenderer::EndScene` builds the buffer from the submitted draw list. Each draw command's descriptor binding is a dynamic-offset binding into this buffer. This eliminates per-draw descriptor allocation and per-draw push-constant chatter.

```cpp
struct ObjectUBO
{
    glm::mat4 Model;
    glm::mat4 PreviousModel;
    glm::mat4 NormalMatrix;
    uint32_t MaterialIndex;
    uint32_t LayerMask;
    uint32_t Flags;
    uint32_t _Pad;
};
```

### 10.5 — Light SSBO

Point and spot light data lives in a per-frame SSBO bound on set 0. The structures are 16-byte aligned for `std140` compatibility. The lighting pass and clustered-cull pass both read this buffer.

**Milestone:** Reverse-Z is active engine-wide; long-distance scenes show correct depth without precision artefacts. Stats panel shows accurate per-pass GPU times. RenderDoc captures present a labelled, readable timeline. Object UBO buffer is allocated and consumed by the geometry pass without per-draw descriptor writes.

---
## Phase 11 — Cascaded Shadow Maps

Replaces the single-cascade shadow stub with a stable, filtered, four-cascade directional shadow system. This is the single most visible improvement in the engine after lighting itself.

### 11.1 — Cascade Layout

Four cascades, each `2048×2048` by default (configurable via `RenderPipelineSettings::ShadowMapResolution`), packed into a single `4096×4096 Depth32F` atlas. Cascade splits use practical-split scheme — a blend of logarithmic and uniform splits with a configurable lambda (default 0.75).

### 11.2 — Stable Cascades

Each cascade's view-projection is snapped to texel boundaries to eliminate the shimmering artefact common in naive implementations. The cascade frustum is computed in world-space, its bounding sphere is taken, and the light's orthographic projection is sized to the sphere with the centre snapped to the nearest texel. The result is a shadow that does not crawl as the camera moves.

### 11.3 — VulkanShadowPass Multi-Cascade

`VulkanShadowPass::Execute` records four sub-rendering scopes back-to-back, one per cascade. Each scope sets the appropriate viewport rectangle into the atlas, sets the cascade's depth bias via `vkCmdSetDepthBias`, and replays the shadow draw list. Per-cascade culling is performed against the cascade frustum so distant cascades draw fewer meshes.

### 11.4 — Lighting Pass Sampling

The lighting pass receives all four cascade matrices in the frame UBO plus the cascade split distances. For each lit pixel: select the cascade by depth, compute shadow-space UV, perform PCF filtering, and apply slope-scaled bias. The default PCF kernel is 3×3 with hardware comparison sampling.

### 11.5 — Filtering Quality Tiers

`RenderPipelineSettings::ShadowFilter` selects between `Hard` (1-tap), `PCF3x3` (default), `PCF5x5`, and `PCSS` (percentage-closer soft shadows). PCSS is implemented in a single shader with a fixed sample pattern using a Vogel disk.

### 11.6 — Cascade Visualisation

A debug view in the renderer menu tints each cascade a different colour for diagnosis. Off in shipping builds.

**Milestone:** A scene with a directional light casts stable, filtered shadows from four cascades. Camera movement does not produce shimmering. Shadow acne and Peter Panning are absent at default bias. The cascade visualisation cleanly marks the four ranges. Shadow pass time appears as a separate entry in the GPU timing stats.

---
## Phase 12 — PBR Deferred Lighting, Skybox, and IBL

The first lit frame. Implements the deferred lighting pass with full Cook-Torrance / GGX BRDF, including direct sun light, image-based diffuse and specular, and the skybox draw. After this phase, the engine produces a recognisably AAA-grade lit image.

### 12.1 — VulkanLightingPass

Compute pass dispatched at full resolution. Reads all GBuffer attachments, the shadow atlas, the cascade matrices, the IBL cubemaps, and the BRDF LUT from set 0. Writes to a single `RGBA16F` HDR output texture. One thread per pixel.

The lighting model is standard real-time PBR: Lambertian diffuse with energy compensation, Cook-Torrance specular with GGX NDF, Smith joint geometry term, Schlick Fresnel. Direct lighting from the sun, image-based lighting from the prefiltered cubemaps, occlusion from the GBuffer AO channel and SSAO output (when SSAO is enabled).

### 12.2 — Skybox Pass

A small `VulkanSkyPass` runs after the lighting pass, drawing the skybox into the lit output for any pixel where the GBuffer depth is the sky (depth value `0.0` under reverse-Z). Implementation is a single triangle covering screen, sampling the skybox cubemap with a view-direction reconstructed from screen UV.

### 12.3 — IBL Generation

Cubemap baking is a one-shot offline-style operation that runs in a worker render-graph rebuild when an environment cubemap is loaded:

- **Diffuse irradiance** — a 32×32 cubemap, each face produced by a compute pass that integrates the environment over the hemisphere.
- **Prefiltered specular** — a mipped 256×256 cubemap, mip N produced by a compute pass that sums environment samples weighted by GGX with roughness `mip / mipCount`.
- **BRDF LUT** — a 512×512 `RG16F` 2D texture, produced once per engine launch by a compute pass integrating the split-sum approximation.

The BRDF LUT is shared across all environments and cached on disk after first generation. Diffuse and specular cubemaps are baked when an environment asset is imported and stored alongside it as a `.envcube` file.

### 12.4 — Sky Material Variants

`MaterialAsset` gains a `Sky` variant flag. A scene's `EnvironmentLighting` resolves to either:
- A fully-baked HDR cubemap loaded from a `.hdr` source via `stbi_loadf`
- An analytic Hosek-Wilkie sky drawn directly into the skybox cubemap by a one-shot compute pass before frame begin

Both feed the same IBL bake.

### 12.5 — Tone-Mapped Debug Output

Until Phase 13 lands, the lighting pass output is tone-mapped inline by a temporary debug draw at the end of the frame. This is removed when the proper tonemap pass is added.

**Milestone:** A scene with a textured PBR mesh, a directional sun, and an HDR skybox renders with correct direct lighting, correct shadows from Phase 11, correct image-based ambient and reflections, and a visible skybox. Materials with metallic 0.0 / roughness 1.0 read as matte; metallic 1.0 / roughness 0.0 read as a mirror. Validation is silent.

---
## Phase 13 — HDR Pipeline: Bloom, Tonemap, Exposure

Closes the HDR loop. Until this phase lands the renderer's HDR output is not correctly displayed; this phase makes the picture finally look right on an SDR monitor.

### 13.1 — VulkanBloomPass

Two-stage compute chain: a 6-mip downsample with 13-tap filter (the COD: Advanced Warfare style), then an upsample with tent filter and additive blend back to mip N-1. Output is composed into the final tonemap pass. Threshold and intensity are exposed in `RenderPipelineSettings`.

Bloom is a textbook example of why aliased transient memory matters — the 6 mip levels sum to a small fraction of full-res frame memory but would hold 1.5× the GBuffer if persistently allocated. They alias with TAA and SSAO scratch.

### 13.2 — Auto-Exposure

A single 64-bin luminance histogram is built each frame by a compute pass that samples the lighting output at 1/4 resolution. A second compute pass reduces the histogram to a single average-log-luminance value, biased by exposure compensation, and writes it to a 1×1 R32F texture that persists across frames. Adaptation rate is configurable; default is 1.1 EV/sec.

Manual exposure mode bypasses the histogram entirely and uses the configured value.

### 13.3 — VulkanTonemapPass

A single fragment shader covering the screen samples the lit colour, samples the bloom result, applies exposure, and applies the ACES filmic tone-mapping curve. Output is the final SDR `BGRA8_SRGB` for the swapchain (or HDR-encoded for HDR display output, see Phase 50).

### 13.4 — Final Output Convention

The lighting output, post bloom and tonemap, is the texture that `SceneRenderer::GetFinalOutput` returns. Forge's `ViewportPanel` displays this texture via `ImGui::Image`. The viewport thus shows the correctly tone-mapped frame matching what a shipped game would display.

**Milestone:** Bright lights bloom in a way that matches reference. Walking from a sunlit area into shadow produces a visible exposure adaptation. Manual exposure produces a stable image. The final image displayed in the editor viewport is gamma-correct, tone-mapped, and not visibly washed out or crushed.

---
## Phase 14 — Screen-Space Ambient Occlusion (GTAO)

Adds ground-truth ambient occlusion as a half-res compute pass. GTAO is chosen over SSAO and HBAO because its integral is closer to ground truth and it produces noticeably less haloing.

### 14.1 — VulkanGtaoPass

Compute pass dispatched at half resolution. Reads GBuffer depth and normal. Per pixel, samples a small number of azimuth directions, marches each direction in horizon-search style, and integrates visibility under the surface's hemisphere. Samples are temporally jittered using the same Halton sequence as TAA so that a small base sample count produces a stable result after TAA accumulation.

The output is a single-channel `R8` half-res texture.

### 14.2 — Bilateral Upsample and Blur

A pair of separable bilateral blur passes upsample the GTAO result to full resolution while preserving edges defined by depth and normal. The result is a full-resolution `R8` AO texture.

### 14.3 — Lighting Pass Integration

The lighting pass binds the AO texture and modulates the indirect diffuse and specular IBL contributions by it. Direct light is unaffected. The material AO channel from the GBuffer is multiplied with the SSAO term so artist-authored AO maps still apply.

### 14.4 — Quality Tiers

`RenderPipelineSettings::SsaoQuality` selects between `Off`, `Low` (4 azimuth, 4 march steps), `Medium` (8 azimuth, 6 steps), and `High` (16 azimuth, 12 steps). Async-compute scheduling: when async compute is supported, the GTAO compute runs concurrently with the shadow pass on the compute queue, shaving its cost off the critical path.

**Milestone:** Indoor and overlapping geometry shows clean contact darkening without visible haloing. The AO term applies only to indirect light. Toggling the pass off and on is visually obvious. On hardware with async compute, the GTAO pass time disappears from the graphics-queue timeline.

---
## Phase 15 — Punctual Lights and Clustered Light Culling

Adds point and spot lights with shadows, and the clustered-light culling system that makes thousands of lights affordable.

### 15.1 — Cluster Grid

The view frustum is divided into a 16×9×24 cluster grid (2304 clusters). Cluster slices in Z are exponentially distributed so near-camera clusters are smaller than far ones.

### 15.2 — VulkanClusteredLightCullPass

A compute pass dispatched once per frame, one workgroup per cluster. Each workgroup tests every active light against its cluster's AABB and writes the surviving light indices into a global light-index list. A per-cluster offset and count are written into a small grid texture. Total work scales with `clusters × lights` but is bounded by an early-out when a per-cluster light cap is hit.

### 15.3 — Lighting Pass Integration

The lighting pass reads the cluster grid and the light index list. Per pixel: compute its cluster, walk only the lights in that cluster's range, accumulate their contribution. The same code path serves both the deferred lighting pass and the forward transparency pass (Phase 16) without duplication.

### 15.4 — Shadowed Punctual Lights

Point lights with shadows allocate a 6-face shadow rectangle in the shadow atlas. Spot lights with shadows allocate a single shadow rectangle. The shadow atlas allocator runs each frame: it sorts shadow casters by importance (distance × intensity), allocates rectangles greedily, and frees rectangles whose lights are no longer present. The `VulkanShadowPass` records per-light shadow draws after the cascade draws.

### 15.5 — Light Importance and Culling

Lights below a screen-space size threshold are culled before reaching the cluster pass. Shadows below an importance threshold are drawn at half resolution; lights below a stricter threshold drop their shadow entirely and are drawn unshadowed. These thresholds are exposed through `RenderPipelineSettings`.

**Milestone:** A scene with a directional sun plus 100+ punctual lights (mix of point and spot, mix of shadowed and unshadowed) renders correctly. Light culling reduces the cost from O(pixels × lights) to O(pixels × lights-per-cluster). Lights moving on and off screen are added to and removed from the shadow atlas without flicker.

---
## Phase 16 — Forward Transparency Pass

Adds the forward pass for transparent geometry that runs after deferred resolve. Until this phase, anything with `BlendMode::Translucent` is invisible.

### 16.1 — VulkanForwardTransparentPass

Runs after the lighting pass, before bloom. Reads the lit HDR output (as both colour write target and potentially a pre-blur source for refraction), reads the GBuffer depth (read-only), reads the shadow atlas, reads the cluster grid + light index list, reads the IBL cubemaps.

The same lighting code that runs in the lighting pass is reused here as a shader include — the only difference is that the forward shader receives surface inputs from the vertex shader and the material directly rather than from a GBuffer sample.

### 16.2 — Sort and Submit

The `RenderQueue` sorts all `BlendMode::Translucent` materials back-to-front by camera distance. Submission is recorded into the forward pass's draw list. Order-independent transparency is left as a future phase (Phase 44) — sorted alpha blending is sufficient for shipping, with documented artefacts on overlapping translucent surfaces.

### 16.3 — Per-Pixel Refraction Hook

A `MaterialFlag::Refractive` opts a material into a copy of the lit colour buffer (taken once before the transparent draw begins) bound as a sampled input. The transparent fragment can then sample the buffer with a UV offset for refraction. This is the engine's path for glass and water without a full screen-space refraction pass.

### 16.4 — Transparent Shadow Receivers

Transparent materials sample shadow maps the same way opaque materials do. The light-index list lookup is shared with the deferred lighting pass.

**Milestone:** A scene mixing opaque and transparent meshes (e.g. a stained glass window in a stone wall) renders with correct PBR shading, correct shadow reception, correct fog colour, and correct sort order. A material flagged `Refractive` shows a visible refraction offset.

---
## Phase 17 — ImGui and SDL3 Backend Initialisation

This phase brings ImGui online. The renderer is fully operational before this phase begins.

### 17.1 — Folder Structure

```
Trinity-Engine/src/Trinity/ImGui/
|-- ImGuiLayer.h/.cpp
|-- ImGuiTheme.h/.cpp
|-- ImGuiUtils.h/.cpp
└-- Platform/
    └-- Vulkan/
        |-- ImGuiVulkanBackend.h/.cpp   ← internal only
        └-- ImGuiSDL3Backend.h/.cpp     ← internal only
```

**Critical rule:** `ImGuiVulkanBackend.h` is never included outside `Trinity/ImGui/Platform/Vulkan/`. It is an implementation detail of `ImGuiLayer.cpp` via the pImpl struct. ImGui's Vulkan backend transitively includes `vulkan.h` — if `ImGuiLayer.h` included it, that header would propagate into `ForgeLayer.h` and collapse the abstraction.

### 17.2 — ImGuiLayer Header

```cpp
// ImGuiLayer.h
#pragma once
#include "Trinity/Layer/Layer.h"
#include <cstdint>
#include <memory>

namespace Trinity
{
    class Texture;

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        void OnInitialize() override;
        void OnShutdown() override;
        void OnEvent(Event& e) override;

        void Begin();
        void End();

        void SetMenuBarCallback(std::function<void()> callback);

        uint64_t RegisterTexture(const std::shared_ptr<Texture>& texture);
        void UnregisterTexture(uint64_t textureID);

        void SetDarkTheme();

        static ImGuiLayer& Get();

    private:
        void PushDockspace();

        struct Impl;
        std::unique_ptr<Impl> m_Impl;

        std::function<void()> m_MenuBarCallback;

        static ImGuiLayer* s_Instance;
    };
}
```

### 17.3 — Initialisation Order

**`OnInitialize()`:**
1. `IMGUI_CHECKVERSION()`, `ImGui::CreateContext()`
2. Enable `ImGuiConfigFlags_DockingEnable`, leave `ImGuiConfigFlags_ViewportsEnable` unset — this is what prevents OS-level window tear-off while preserving full in-window docking
3. Load fonts and apply dark theme via `ImGuiTheme`
4. Create dedicated `VkDescriptorPool` for ImGui (1000 of each descriptor type)
5. `imgui_impl_sdl3` initialised with SDL window handle
6. `imgui_impl_vulkan` initialised with `UseDynamicRendering = true` and swapchain colour format

**`Begin()`:**
1. `ImGui_ImplVulkan_NewFrame()`, `ImGui_ImplSDL3_NewFrame()`, `ImGui::NewFrame()`
2. `PushDockspace()`

**`End()`:**
1. `ImGui::Render()`
2. Retrieve current frame command buffer from `Renderer::GetAPI()`
3. Begin dynamic rendering scope targeting current swapchain image
4. `ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer)`
5. End dynamic rendering scope

**`OnShutdown()`:**
1. `Renderer::WaitIdle()` — must drain GPU before destroying ImGui resources
2. `ImGui_ImplVulkan_Shutdown()`, `ImGui_ImplSDL3_Shutdown()`, `ImGui::DestroyContext()`
3. Destroy `VkDescriptorPool`

### 17.4 — RegisterTexture / UnregisterTexture

```cpp
uint64_t ImGuiLayer::RegisterTexture(const std::shared_ptr<Texture>& texture)
{
    VkImageView l_View = reinterpret_cast<VkImageView>(texture->GetOpaqueHandle());
    VkSampler l_Sampler = m_Impl->GetDefaultSampler();

    ImTextureID l_ID = ImGui_ImplVulkan_AddTexture(l_Sampler, l_View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return reinterpret_cast<uint64_t>(l_ID);
}

void ImGuiLayer::UnregisterTexture(uint64_t textureID)
{
    ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(reinterpret_cast<void*>(textureID)));
}
```

Forge calls `RegisterTexture` with an abstract `shared_ptr<Texture>`, stores the returned `uint64_t`, and passes it to `ImGui::Image`. It never looks inside the value.

**Milestone:** ImGui initialises and shuts down without validation errors. An empty ImGui frame renders over the swapchain.

---

## Phase 18 — Dockspace

### 18.1 — Implementation

```cpp
void ImGuiLayer::PushDockspace()
{
    const ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(l_Viewport->WorkPos);
    ImGui::SetNextWindowSize(l_Viewport->WorkSize);
    ImGui::SetNextWindowViewport(l_Viewport->ID);

    ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##TrinityDockspace", nullptr, l_Flags);
    ImGui::PopStyleVar(3);

    const ImGuiID l_DockspaceID = ImGui::GetID("TrinityDockspace");
    ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMenuBar())
    {
        if (m_MenuBarCallback)
            m_MenuBarCallback();
        ImGui::EndMenuBar();
    }

    ImGui::End();
}
```

`ImGuiDockNodeFlags_PassthruCentralNode` makes the central empty area transparent so the clear colour shows through when no panel occupies it.

**Milestone:** Dockspace renders. Panels dock to any window edge. Layout persists across frames.

---

## Phase 19 — Panel Registration System

### 19.1 — Folder Structure

```
Trinity-Forge/src/Forge/
|-- ForgeLayer.h/.cpp
|-- Panels/
|   |-- PanelManager.h/.cpp
|   |-- Panel.h
|   |-- SceneHierarchyPanel.h/.cpp
|   |-- InspectorPanel.h/.cpp
|   |-- ContentBrowserPanel.h/.cpp
|   |-- ViewportPanel.h/.cpp
|   |-- RendererStatsPanel.h/.cpp
|   └-- LogPanel.h/.cpp
└-- Utilities/
    └-- ForgeUtilities.h/.cpp
```

### 19.2 — Panel Abstract Base

```cpp
// Panel.h
#pragma once
#include <string>

namespace Forge
{
    class Panel
    {
    public:
        explicit Panel(std::string name) : m_Name(std::move(name)) {}
        virtual ~Panel() = default;

        virtual void OnInitialize() {}
        virtual void OnShutdown() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnRender() = 0;

        const std::string& GetName() const { return m_Name; }
        bool& GetOpenState() { return m_Open; }
        bool IsOpen() const { return m_Open; }

    protected:
        std::string m_Name;
        bool m_Open = true;
    };
}
```

### 19.3 — PanelManager

```cpp
// PanelManager.h
#pragma once
#include "Forge/Panels/Panel.h"
#include <memory>
#include <vector>

namespace Forge
{
    class PanelManager
    {
    public:
        void Initialize();
        void Shutdown();

        template<typename T, typename... Args>
        T* RegisterPanel(Args&&... args)
        {
            auto l_Panel = std::make_unique<T>(std::forward<Args>(args)...);
            T* l_Ptr = l_Panel.get();
            l_Panel->OnInitialize();
            m_Panels.push_back(std::move(l_Panel));
            return l_Ptr;
        }

        void UpdatePanels(float deltaTime);
        void RenderPanels();
        void RenderViewMenu();

        Panel* FindPanel(const std::string& name) const;

    private:
        std::vector<std::unique_ptr<Panel>> m_Panels;
    };
}
```

### 19.4 — ForgeLayer Registration

```cpp
void ForgeLayer::OnInitialize()
{
    m_PanelManager.Initialize();

    m_ViewportPanel = m_PanelManager.RegisterPanel<ViewportPanel>("Viewport");
    m_HierarchyPanel = m_PanelManager.RegisterPanel<SceneHierarchyPanel>("Scene Hierarchy", &m_SelectionContext);
    m_InspectorPanel = m_PanelManager.RegisterPanel<InspectorPanel>("Inspector", &m_SelectionContext);
    m_ContentPanel = m_PanelManager.RegisterPanel<ContentBrowserPanel>("Content Browser");
    m_StatsPanel = m_PanelManager.RegisterPanel<RendererStatsPanel>("Renderer Stats");
    m_LogPanel = m_PanelManager.RegisterPanel<LogPanel>("Log");

    Trinity::ImGuiLayer::Get().SetMenuBarCallback([this]() { RenderMenuBar(); });
}
```

`SelectionContext` is a plain struct owned by `ForgeLayer` and passed by pointer to both `SceneHierarchyPanel` and `InspectorPanel`.

```cpp
struct SelectionContext
{
    entt::entity SelectedEntity = entt::null;
    Trinity::Scene* ActiveScene = nullptr;
};
```

**Milestone:** All panels register, open, close, and dock. View menu toggles them. Layout is stable.

---

## Phase 20 — Main Menu Bar

### 20.1 — Menu Structure

```
File
    New Scene           Ctrl+N
    Open Scene          Ctrl+O
    Save Scene          Ctrl+S
    Save Scene As       Ctrl+Shift+S
    ─────────────────
    Exit

Edit
    Undo                Ctrl+Z    ← gap until command system implemented
    Redo                Ctrl+Y    ← gap

View
    [auto-generated by PanelManager::RenderViewMenu()]

Renderer
    Backend: Vulkan     (read-only label)
    ─────────────────
    Show GeometryBuffer Albedo     ← greyed out (gap: requires lighting pass)
    Show GeometryBuffer Normal     ← greyed out (gap)
    Show GeometryBuffer MRA        ← greyed out (gap)
    Show Shadow Map         ← greyed out (gap)
    ─────────────────
    Renderer Stats

Help
    About Trinity Engine
```

GeometryBuffer debug items use `ImGui::BeginDisabled()` until `SceneRenderer::SetDebugOutput()` is implemented post-lighting-pass.

**Milestone:** All menus render. File operations hook scene serialisation. View menu delegates to `PanelManager`.

---

## Phase 21 — Viewport Panel and Editor Camera

### 21.1 — Viewport Resize Handling

```cpp
void ViewportPanel::OnRender()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport", &m_Open);
    ImGui::PopStyleVar();

    const ImVec2 l_PanelSize = ImGui::GetContentRegionAvail();
    const uint32_t l_Width = static_cast<uint32_t>(l_PanelSize.x);
    const uint32_t l_Height = static_cast<uint32_t>(l_PanelSize.y);

    if (l_Width != m_LastWidth || l_Height != m_LastHeight)
    {
        if (l_Width > 0 && l_Height > 0)
        {
            m_LastWidth = l_Width;
            m_LastHeight = l_Height;

            if (m_ViewportTextureID != 0)
                Trinity::ImGuiLayer::Get().UnregisterTexture(m_ViewportTextureID);

            m_SceneRenderer.OnResize(l_Width, l_Height);

            m_ViewportTextureID = Trinity::ImGuiLayer::Get().RegisterTexture(m_SceneRenderer.GetFinalOutput());
        }
    }

    if (m_ViewportTextureID != 0)
    {
        ImGui::Image(reinterpret_cast<ImTextureID>(m_ViewportTextureID), l_PanelSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    }

    RenderGizmos();
    ImGui::End();
}
```

### 21.2 — Editor Camera

Active only when the viewport is both hovered and focused.

```cpp
void ViewportPanel::OnUpdate(float deltaTime)
{
    if (!ImGui::IsWindowHovered() || !ImGui::IsWindowFocused())
        return;

    if (Trinity::Input::MouseButtonDown(Code::MouseCode::TR_BUTTON_RIGHT))
    {
        Trinity::Application::Get().GetWindow().SetCursorLocked(true);

        const float l_Speed = m_CameraSpeed * deltaTime;
        const Trinity::Input::Vector2 l_Delta = Trinity::Input::MouseDelta();

        m_Camera.Rotate(l_Delta.x * m_Sensitivity, l_Delta.y * m_Sensitivity);

        if (Trinity::Input::KeyDown(Code::KeyCode::TR_KEY_W)) m_Camera.MoveForward(l_Speed);
        if (Trinity::Input::KeyDown(Code::KeyCode::TR_KEY_S)) m_Camera.MoveForward(-l_Speed);
        if (Trinity::Input::KeyDown(Code::KeyCode::TR_KEY_A)) m_Camera.MoveRight(-l_Speed);
        if (Trinity::Input::KeyDown(Code::KeyCode::TR_KEY_D)) m_Camera.MoveRight(l_Speed);
        if (Trinity::Input::KeyDown(Code::KeyCode::TR_KEY_E)) m_Camera.MoveUp(l_Speed);
        if (Trinity::Input::KeyDown(Code::KeyCode::TR_KEY_Q)) m_Camera.MoveUp(-l_Speed);
    }
    else
    {
        Trinity::Application::Get().GetWindow().SetCursorLocked(false);
    }

    const float l_Scroll = Trinity::Input::MouseScrolled().y;
    if (l_Scroll != 0.0f)
        m_Camera.AdjustFOV(-l_Scroll * 2.0f);
}
```

**Milestone:** Scene renders into the viewport. Camera flies correctly. Viewport resizes cleanly without validation errors.

---

## Phase 22 — ImGuizmo

### 22.1 — The Y-Axis Flip

ImGuizmo assumes OpenGL NDC (Y up). Vulkan NDC is Y down. The projection matrix passed to ImGuizmo must have its Y axis flipped. This is a local copy used only for the gizmo draw call — the renderer's projection matrix is never modified.

```cpp
void ViewportPanel::RenderGizmos()
{
    if (!m_SelectionContext->SelectedEntity != entt::null)
        return;
    if (m_GizmoOperation == static_cast<ImGuizmo::OPERATION>(-1))
        return;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    const ImVec2 l_WindowPos = ImGui::GetWindowPos();
    const ImVec2 l_WindowSize = ImGui::GetWindowSize();
    ImGuizmo::SetRect(l_WindowPos.x, l_WindowPos.y, l_WindowSize.x, l_WindowSize.y);

    const glm::mat4& l_View = m_Camera.GetViewMatrix();
    glm::mat4 l_Projection = m_Camera.GetProjectionMatrix();
    l_Projection[1][1] *= -1.0f;   // Vulkan NDC Y-down correction, gizmo draw only

    auto& l_Transform = m_SelectionContext->ActiveScene->GetRegistry().get<TransformComponent>(m_SelectionContext->SelectedEntity);

    glm::mat4 l_ModelMatrix = l_Transform.GetMatrix();

    ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(l_ModelMatrix));

    if (ImGuizmo::IsUsing())
    {
        glm::vec3 l_Translation, l_Rotation, l_Scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(l_ModelMatrix), glm::value_ptr(l_Translation), glm::value_ptr(l_Rotation), glm::value_ptr(l_Scale));

        l_Transform.Position = l_Translation;
        l_Transform.Rotation = glm::radians(l_Rotation);
        l_Transform.Scale = l_Scale;
    }
}
```

### 22.2 — Operation Switching

Active only when viewport is focused and no gizmo manipulation is in progress.

```cpp
if (ImGui::IsWindowFocused() && !ImGuizmo::IsUsing())
{
    if (Trinity::Input::KeyPressed(Code::KeyCode::TR_KEY_W))
        m_GizmoOperation = ImGuizmo::TRANSLATE;
    if (Trinity::Input::KeyPressed(Code::KeyCode::TR_KEY_E))
        m_GizmoOperation = ImGuizmo::ROTATE;
    if (Trinity::Input::KeyPressed(Code::KeyCode::TR_KEY_R))
        m_GizmoOperation = ImGuizmo::SCALE;
    if (Trinity::Input::KeyPressed(Code::KeyCode::TR_KEY_Q))
        m_GizmoOperation = static_cast<ImGuizmo::OPERATION>(-1);
}
```

Gizmo toolbar with icon buttons is a deliberate gap pending icon atlas creation.

**Milestone:** Gizmos render correctly oriented. Transform manipulation updates the entity. Operation switching works.

---

## Phase 23 — Core Panels

### 23.1 — SceneHierarchyPanel

Displays entity tree from the active `Scene`. Communicates selected entity to `InspectorPanel` via the shared `SelectionContext`. Right-click context menu: Rename, Duplicate, Delete. Entity parenting and drag-and-drop reorder are deliberate gaps pending the scene parenting system.

### 23.2 — InspectorPanel

Displays and edits all components on the selected entity using a template-based draw dispatch.

```cpp
template<typename T, typename DrawFn>
static void DrawComponent(const std::string& name, entt::registry& registry,
    entt::entity entity, DrawFn drawFn)
{
    if (!registry.all_of<T>(entity))
        return;

    const ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

    auto& l_Component = registry.get<T>(entity);

    ImGui::PushID(typeid(T).hash_code());
    const bool l_Open = ImGui::TreeNodeEx(name.c_str(), l_Flags);

    if (ImGui::BeginPopupContextItem("##ComponentContext"))
    {
        if (ImGui::MenuItem("Remove Component"))
            registry.remove<T>(entity);
        ImGui::EndPopup();
    }

    if (l_Open)
    {
        drawFn(l_Component);
        ImGui::TreePop();
    }

    ImGui::PopID();
}
```

Components drawn: `TagComponent`, `TransformComponent`, `MeshRendererComponent`, `ModelRendererComponent`, `CameraComponent`, `DirectionalLightComponent`. Add Component button opens a searchable popup.

### 23.3 — ContentBrowserPanel

Displays the asset directory backed by `std::filesystem`. Back button, directory navigation, drag-and-drop mesh instantiation into viewport. Asset type detected by file extension before creating components — fixes the previous `HandleDroppedMeshAsset` bug where the wrong template parameter was used. Thumbnail generation is a deliberate gap.

### 23.4 — RendererStatsPanel

Displays `SceneRendererStats` returned by `SceneRenderer::GetStats()`. GPU timing values show 0.0 until `VkQueryPool` timestamp queries are implemented (deliberate gap).

### 23.5 — LogPanel

Custom `spdlog` sink pushes entries into a ring buffer capped at 1000 entries. Log panel renders with `ImGui::TextColored` per level. Scroll lock and clear button are the only controls.

**Milestone:** All panels functional. Inspector edits components in real-time. Viewport reflects changes immediately.

---

## Phase 24 — Play / Pause / Stop

### 24.1 — Editor State Machine

```cpp
enum class EditorState : uint8_t
{
    Edit = 0,
    Play,
    Pause
};
```

State transitions: `Edit → Play` serialises the active scene to an in-memory buffer and starts the game loop. `Play → Edit` (Stop) deserialises the buffer back, discarding all runtime changes. `Play → Pause` holds delta time at zero. `Pause → Play` resumes time.

### 24.2 — Toolbar

Rendered inside `ViewportPanel::OnRender()` anchored to the top-centre of the panel before the `ImGui::Image` call using `ImGui::SetCursorPos`.

| State | Play | Pause | Stop |
|---|---|---|---|
| Edit | Enabled | Disabled | Disabled |
| Play | Disabled | Enabled | Enabled |
| Pause | Enabled (Resume) | Disabled | Enabled |

### 24.3 — Camera Behaviour Per State

In Edit state: editor fly camera, gizmos enabled. In Play/Pause state: scene `CameraComponent` view, gizmos disabled. If no `CameraComponent` exists, warning displays in viewport and editor camera is used as fallback.

**Milestone:** Play, pause, and stop work correctly. Scene state saved and restored. Runtime changes do not persist after stop.

---

## Phase 25 — Theme and Visual Polish

### 25.1 — Dark Theme

A consistent dark theme with a neutral base and blue accent. All colours defined once in `ImGuiTheme::ApplyDarkTheme()`. Key values:

- Window background: `(0.13, 0.13, 0.14)`
- Frame background: `(0.20, 0.20, 0.22)`
- Accent (interactive elements, selection): `(0.27, 0.52, 0.93)` — blue
- Text: `(0.90, 0.90, 0.92)`
- Disabled text: `(0.45, 0.45, 0.48)`
- Tab selected: `(0.20, 0.20, 0.22)`
- Menu bar: `(0.10, 0.10, 0.11)`
- Docking preview: accent at 50% alpha

### 25.2 — Font Loading

Two weights loaded at startup from `Trinity-Forge/assets/fonts/`:

```cpp
void ImGuiTheme::LoadFonts()
{
    ImGuiIO& l_IO = ImGui::GetIO();
    l_IO.Fonts->AddFontFromFileTTF("assets/fonts/JetBrains Mono-Regular.ttf", 14.0f);
    l_IO.Fonts->AddFontFromFileTTF("assets/fonts/JetBrains Mono-SemiBold.ttf", 15.0f);
    l_IO.Fonts->Build();
}
```

Font files are committed as binary assets. They are not gitignored.

**Milestone:** Consistent dark theme across all panels. Custom fonts loaded. Editor is visually complete.

---

## Phase 26 — CPU Frustum Culling

Adds CPU-side bounding-volume culling against the camera frustum and against each shadow cascade frustum. This is the cheapest culling phase and produces large wins immediately.

### 26.1 — Bounding Volumes on Mesh

Every imported mesh stores an axis-aligned bounding box and a bounding sphere computed at import. `MeshDrawCommand` carries the world-space matrix; the cull pass transforms the local AABB to a world-space sphere using the matrix's translation and the largest scale component.

### 26.2 — Frustum Extraction

`Camera::GetFrustum()` returns six planes extracted from the view-projection matrix. The same procedure runs per cascade for shadow culling, using each cascade's tightened ortho matrix.

### 26.3 — Culling Service

`RenderQueue::Cull` walks the submitted draw list once for the main camera and once per cascade. Surviving commands go into per-pass draw lists. The culling itself uses SIMD where available (`glm::simdMat4` paths) to amortise per-test cost.

### 26.4 — Layer Mask Filtering

Each draw command and each pass carries a `LayerMask`. Passes ignore commands whose mask does not intersect their own. Used to exclude e.g. editor gizmo geometry from shadow casts.

**Milestone:** A scene with 10000 meshes, of which 200 are visible, issues 200 draw calls. Removing the cull step temporarily restores 10000 draw calls and demonstrates the win. Cascade-specific culling reduces shadow draw count for far cascades to near zero.

---
## Phase 27 — GPU-Driven Rendering, Indirect Draws, and LOD

Moves draw submission onto the GPU, integrates a discrete LOD system, and prepares the engine for very high mesh counts.

### 27.1 — LOD Asset

`MeshAsset` becomes a list of `SubMesh` LODs sorted from highest to lowest detail, each with its own vertex/index buffer offsets and a screen-coverage threshold. The mesh cooker produces LODs by simplifying the source mesh through MeshOptimizer; artist-authored LODs override the simplifier when present.

### 27.2 — GPU Visibility Compute

A compute pass runs per camera (main + each cascade) over the full instance buffer. For each instance: transform AABB, frustum-test, occlusion-test against the previous frame's downsampled depth pyramid (Hi-Z), select LOD by screen coverage, write a compacted draw arg into the output indirect buffer with the LOD's offsets, atomically increment a count buffer.

The graphics pass then issues a single `DrawIndexedIndirectCount` per material bucket. Draw count drops from CPU-bounded to GPU-bounded.

### 27.3 — Hi-Z Depth Pyramid

After the geometry pass, a small compute chain downsamples the GBuffer depth into a mipmapped texture using max-of-four reduction. This pyramid is the previous frame's input to the visibility compute. Two-pass occlusion culling (rough cull + render + refine cull + render) is supported behind a setting and used for dense scenes.

### 27.4 — Dithered LOD Transition

Crossfade between adjacent LODs uses screen-space ordered dithering applied in the geometry shader's discard logic. No dual-render-of-both-LODs path is taken — one LOD is selected per pixel based on the dither pattern weighted by the transition fraction.

**Milestone:** A scene with 100000 mesh instances renders at smooth frame rate with GPU culling and LOD switching active. CPU draw submission cost is constant regardless of instance count. LOD transitions are visually smooth.

---
## Phase 28 — Bindless Descriptors

Promotes the bindless heap from groundwork to active path. After this phase, materials no longer allocate per-instance descriptor sets; they pass texture indices into shaders.

### 28.1 — Bindless Heap

A single `DescriptorSet` with a `VariableDescriptorCount` array of 16384 sampled images, allocated at startup. Texture creation registers the texture into the next free index of this heap; texture destruction frees the index after `MaxFramesInFlight` frames have elapsed.

### 28.2 — Material Refactor

`Material` holds an array of texture indices instead of an array of `Texture` references in its descriptor set. The material parameter UBO is extended to carry these indices alongside the float / vector parameters. Set 2 collapses to a single UBO binding; the bindless heap covers all texture access from set 0.

### 28.3 — Shader Convention

Shaders gain a global include `bindless.glsl` declaring:

```glsl
layout(set = 0, binding = BINDLESS_TEXTURES_BINDING) uniform texture2D u_BindlessTextures[];
layout(set = 0, binding = BINDLESS_TEXTURES_BINDING) uniform textureCube u_BindlessTexturesCube[];

vec4 SampleBindless(uint index, vec2 uv)
{
    return texture(sampler2D(u_BindlessTextures[nonuniformEXT(index)], u_DefaultSampler), uv);
}
```

Material access becomes `SampleBindless(material.AlbedoIndex, v_UV)`.

### 28.4 — GPU-Driven Path Synergy

With bindless live, the indirect-draw path no longer needs to switch pipelines per material. A single mega-draw for the geometry pass becomes possible: every mesh, every material, one `DrawIndexedIndirectCount`. Pipeline switches collapse to one per BlendMode + one per AlphaTested variant.

**Milestone:** A scene with 5000 unique materials renders with a single geometry pass draw call (per blend mode). Validation confirms `descriptorBindingPartiallyBound` and `runtimeDescriptorArray` features are exercised correctly. Texture hot-swap (replacing a material's albedo at runtime) updates the heap entry without recreating any descriptor set.

---
## Phase 29 — PSO Cache Polish, Multi-Threaded Command Recording

Closes the remaining performance gaps in the renderer's hot path: pipeline creation no longer hitches the frame, and command recording uses all available CPU cores.

### 29.1 — PSO Cache Polish

The `VkPipelineCache` introduced in Phase 3 is given two refinements: a header recording the device UUID and driver version (a mismatch invalidates the cache), and an offline pre-warmer that, on first run, walks every shader permutation × pipeline state combination known to the engine and forces compilation. After this pre-warm the user never again pays a pipeline-compile hitch on this hardware.

### 29.2 — Job System

A small thread-pool job system is introduced under `Trinity/Threading/`. It is the foundation for multi-threaded recording and is also consumed by other systems (asset import, physics).

### 29.3 — Multi-Threaded Command Recording

`SceneRenderer::Render` schedules per-pass recording onto job threads. Each pass records into a `VulkanCommandList` allocated from a per-thread, per-frame command pool. The main thread waits for all per-pass jobs and submits the command lists in the graph's execution order. Cross-pass barriers are still injected by the graph; per-pass body execution is the part that parallelises.

### 29.4 — Per-Thread Allocators

Each worker thread has its own descriptor pool slice and its own staging-buffer ring slice. No mutex is taken in the per-pass recording path.

**Milestone:** A scene heavy in draw calls saturates multiple cores during the record phase. Frame time drops in proportion to thread count up to the number of passes. First run after a clean build pays a one-time pipeline pre-warm cost; subsequent runs do not.

---
## Phase 30 — Streaming and Residency Budgets

Adds the systems necessary for scenes larger than GPU memory.

### 30.1 — Streaming Manager

`StreamingManager` owns a request queue and a residency budget per resource class (textures, meshes, audio). Each frame it walks active scene objects, computes their on-screen importance, requests load of resources whose importance has risen above a threshold, and requests unload of resources below a lower threshold (hysteresis prevents flapping).

### 30.2 — Texture Mip Streaming

Textures load with their lowest-resolution mips first via the upload queue. Higher mips are loaded as the camera approaches. The bindless heap holds a pointer to a residency tracker; a shader sampling a not-yet-resident mip transparently falls back to the next-coarsest available mip.

### 30.3 — Mesh Streaming

Meshes load by LOD chain. The lowest LOD loads with the asset; higher LODs stream in based on screen coverage. Combined with Phase 27's GPU LOD selection, the engine cleanly handles "approach a mesh and watch it gain detail" without pop.

### 30.4 — Async Asset Decompression

Compressed asset blobs (LZ4 or Zstandard) are decompressed on job-system worker threads, with the result handed to the upload queue. The main thread is never blocked on I/O or decompression.

### 30.5 — Memory Budget Enforcement

If the textures budget is exceeded, the manager evicts the lowest-importance mips first. If the meshes budget is exceeded, the manager evicts the highest-LOD mesh data. Eviction rate is rate-limited; sustained over-budget is reported as a warning in the stats panel.

**Milestone:** A test scene authored to exceed available VRAM by 4× streams correctly: nearby assets are at full quality, distant assets are at lower mips, the camera can fly around freely without stutters, and the streaming budget panel shows the manager keeping under cap.

---
## Phase 31 — ECS, Scene System, and Asset Pipeline

This phase makes the scene graph production-quality and gets the Assimp import layer into a working state. Every gap deferred from Phase 5, Phase 11, and Phase 15 that touches ECS or assets is resolved here.

### 31.1 — UUID-Based Entity Identity

Every entity carries a stable `UUIDComponent` containing a 64-bit random UUID generated at creation time. Deserialization assigns the stored UUID rather than generating a new one. This closes the entity ID collision risk that exists after deserialization in the current codebase.

```cpp
// UUIDComponent.h
#pragma once
#include <cstdint>

namespace Trinity
{
	struct UUIDComponent
	{
		uint64_t UUID = 0;
	};
}
```

A global `UUIDGenerator` produces non-repeating values using `std::mt19937_64` seeded from `std::random_device`.

### 31.2 — Entity Parenting

`TransformComponent` gains a parent UUID field. World-space transform is computed by walking the parent chain. The scene stores a flat EnTT registry; hierarchy is logical, not structural.

```cpp
struct TransformComponent
{
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
	uint64_t ParentUUID = 0;	// 0 = no parent

	glm::mat4 GetLocalMatrix() const;
	glm::mat4 GetWorldMatrix(const entt::registry& registry) const;
};
```

`SceneHierarchyPanel` renders a tree view by walking parent/child relationships rather than a flat list. Drag-and-drop reparenting activates here.

### 31.3 — Prefab System

A prefab is a yaml-cpp document describing a single entity and all its components, stored as a `.prefab` file in the asset directory. Instantiating a prefab creates a new entity, assigns a fresh UUID, and copies all component data. Prefab files are treated as first-class assets and appear in the Content Browser.

```
Trinity-Engine/src/Trinity/Scene/
|-- Scene.h/.cpp
|-- Entity.h/.cpp
|-- Prefab.h/.cpp
|-- Components/
|   |-- UUIDComponent.h
|   |-- TagComponent.h
|   |-- TransformComponent.h
|   |-- MeshRendererComponent.h
|   |-- ModelRendererComponent.h
|   |-- CameraComponent.h
|   |-- DirectionalLightComponent.h
|   |-- RigidbodyComponent2D.h       <- gap, filled in Phase 20
|   |-- ColliderComponent2D.h        <- gap, filled in Phase 20
|   |-- RigidbodyComponent3D.h       <- gap, filled in Phase 21
|   |-- ColliderComponent3D.h        <- gap, filled in Phase 21
|   |-- AudioSourceComponent.h       <- gap, filled in Phase 22
|   |-- AudioListenerComponent.h     <- gap, filled in Phase 22
|   └-- ScriptComponent.h            <- gap, filled in Phase 23
└-- SceneSerializer.h/.cpp
```

### 31.4 — Scene Serialisation with yaml-cpp

`SceneSerializer` writes every entity's UUID, tag, transform, and all present components to a `.trinity` yaml file. It reads the same format back. Component fields map one-to-one to yaml keys. Unknown keys are ignored silently so forward compatibility is preserved.

```yaml
Scene: MyScene
Entities:
  - UUID: 12345678901234567
    Tag: Cube
    Transform:
      Position: [0.0, 0.0, 0.0]
      Rotation: [0.0, 0.0, 0.0]
      Scale: [1.0, 1.0, 1.0]
      Parent: 0
    MeshRenderer:
      MeshUUID: 98765432109876543
      MaterialUUID: 11111111111111111
```

### 31.5 — Asset Registry

A singleton `AssetRegistry` maps UUID to asset metadata (type, source path, cooked path). UUIDs are generated once when an asset is first imported and stored in a `.meta` sidecar file next to the source file. Re-scanning the asset directory reads `.meta` files rather than regenerating UUIDs, which closes the unstable UUID regeneration bug in the current codebase.

```
Trinity-Engine/src/Trinity/Asset/
|-- AssetRegistry.h/.cpp
|-- AssetHandle.h
|-- AssetMetadata.h
|-- AssetImporter.h/.cpp
|-- Importers/
|   |-- MeshImporter.h/.cpp
|   |-- TextureImporter.h/.cpp
|   └-- SceneImporter.h/.cpp
```

### 31.6 — Functional Assimp Import Layer

`MeshImporter` calls Assimp with `aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs`. It walks the Assimp scene tree and produces an `ImportedModelAsset` containing one `SubMesh` per Assimp mesh node.

Auto-detection at drag-and-drop inspects the file extension. Files matching `.fbx .obj .gltf .glb .dae` produce a `ModelRendererComponent`. The previous `HandleDroppedMeshAsset` wrong-template-parameter bug is fixed by routing through the asset type detected at import time rather than at drop time.

**Milestone:** Entities have stable UUIDs. Scene saves and loads with yaml-cpp. Parent/child hierarchy renders correctly in the hierarchy panel. Dragging an FBX into the viewport instantiates a model with the correct component type.

---

## Phase 32 — Animation

This phase introduces skeletal animation. It depends on Phase 18 because the mesh import layer must be functional and producing tangent data before animation can be layered on top.

### 32.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Animation/
|-- Skeleton.h/.cpp
|-- Bone.h
|-- AnimationClip.h/.cpp
|-- Animator.h/.cpp
|-- AnimatorComponent.h
```

### 32.2 — Skeleton and Bone

Assimp's `aiBone` array is walked during mesh import when `AI_SCENE_FLAGS_INCOMPLETE` is not set and bones are present. Each `Bone` stores its name, inverse bind-pose matrix, and index into the per-vertex weight list. The `Skeleton` stores the bone hierarchy as a flat array with parent index per bone.

Vertex data for skinned meshes extends `Geometry::Vertex` with `BoneIndices` (glm::ivec4) and `BoneWeights` (glm::vec4). A separate `SkinnedVertex` type is used so unskinned meshes pay no memory cost.

### 32.3 — AnimationClip

Each clip stores per-bone keyframe tracks for position, rotation, and scale. Keyframes are stored as sorted arrays of `(time, value)` pairs. Interpolation is linear for position and scale, and slerp for rotation.

```cpp
struct AnimationClip
{
	std::string Name;
	float DurationSeconds = 0.0f;
	float TicksPerSecond = 25.0f;
	std::vector<BoneTrack> BoneTracks;
};
```

### 32.4 — Animator

`Animator` owns a reference to a `Skeleton` and a current `AnimationClip`. Each frame it advances the playback time, samples all bone tracks, and writes the resulting palette of `MaxBones` (128) world-space matrices into a GPU uniform buffer. The geometry pass binds this buffer when a `AnimatorComponent` is present on the entity.

The compute skinning path is a deliberate gap — skinning runs on the CPU in this phase and is moved to a compute pre-pass in a future renderer iteration.

### 32.5 — Editor Integration

`InspectorPanel` gains an `AnimatorComponent` drawer showing clip name, playback time scrubber, play/pause toggle, and loop checkbox. A dedicated Animation panel is a deliberate gap pending a curve editor.

**Milestone:** A skinned FBX loads, plays its embedded animation clip, and deforms correctly in the viewport.

---

## Phase 33 — 2D Physics (Box2D)

### 33.1 — Dependency

Box2D is added as a Git submodule under `Trinity-Engine/vendor/box2d`. It is built as a static library via `add_subdirectory`. No Box2D header ever appears outside `Trinity/Physics/2D/`.

### 33.2 — Folder Structure

```
Trinity-Engine/src/Trinity/Physics/
|-- PhysicsAPI.h                    <- abstract base, no Box2D includes
|-- 2D/
|   |-- PhysicsWorld2D.h/.cpp
|   |-- Rigidbody2D.h/.cpp
|   |-- Collider2D.h/.cpp
|   └-- ContactListener2D.h/.cpp
└-- 3D/
    └-- (gap, filled in Phase 21)
```

### 33.3 — Abstract Interface

`PhysicsAPI` is intentionally thin. It exposes only what the scene and component layer needs.

```cpp
// PhysicsAPI.h
#pragma once
#include <cstdint>

namespace Trinity
{
	class PhysicsAPI
	{
	public:
		virtual ~PhysicsAPI() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void Step(float fixedDeltaTime) = 0;
		virtual void SetGravity(float x, float y) = 0;
	};
}
```

`PhysicsWorld2D` implements `PhysicsAPI` and owns a `b2World`. It is created by `Application` alongside the renderer, stepped at a fixed 1/60 s timestep independent of render delta time, and destroyed before the renderer shuts down.

### 33.4 — Components

```cpp
struct Rigidbody2DComponent
{
	enum class BodyType : uint8_t { Static = 0, Kinematic, Dynamic };
	BodyType Type = BodyType::Dynamic;
	bool FixedRotation = false;
	float GravityScale = 1.0f;
	void* RuntimeBody = nullptr;	// b2Body*, opaque outside Physics/2D/
};

struct BoxCollider2DComponent
{
	glm::vec2 Offset = glm::vec2(0.0f);
	glm::vec2 Size = glm::vec2(0.5f);
	float Density = 1.0f;
	float Friction = 0.5f;
	float Restitution = 0.0f;
	void* RuntimeFixture = nullptr;
};

struct CircleCollider2DComponent
{
	glm::vec2 Offset = glm::vec2(0.0f);
	float Radius = 0.5f;
	float Density = 1.0f;
	float Friction = 0.5f;
	float Restitution = 0.0f;
	void* RuntimeFixture = nullptr;
};
```

`RuntimeBody` and `RuntimeFixture` are `void*` typed at the component level so Box2D types never leak into the scene header. They are only ever cast inside `Physics/2D/` translation units.

### 33.5 — Scene Integration

When Play is entered, `Scene::OnPhysicsStart2D()` iterates all entities with `Rigidbody2DComponent` and creates corresponding `b2Body` instances, then attaches fixtures from any collider components on the same entity. Each physics step writes the simulated position and angle back into `TransformComponent`. When Stop is entered, `Scene::OnPhysicsStop2D()` destroys all `b2Body` instances and nulls the `RuntimeBody` pointers.

### 33.6 — Contact Callbacks

`ContactListener2D` implements `b2ContactListener`. On `BeginContact` and `EndContact` it resolves the two entities from `b2Body::GetUserData` and dispatches a `PhysicsContactEvent` into the engine event queue. Lua scripts subscribe to this event in Phase 23.

**Milestone:** A stack of dynamic box entities falls under gravity, collides, and comes to rest correctly in Play mode. Stop restores all transforms to their pre-play values.

---

## Phase 34 — 3D Physics (PhysX 5)

### 34.1 — Dependency

PhysX 5 is cloned as a Git submodule under `Trinity-Engine/vendor/physx`. The CMake integration uses PhysX's provided `CMakeLists.txt` with `PX_BUILDSNIPPETS = OFF`, `PX_BUILDPUBLICSAMPLES = OFF`, `NV_USE_STATIC_WINCRT = OFF`. Windows static libraries are linked. No PhysX header ever appears outside `Trinity/Physics/3D/`.

### 34.2 — Folder Structure

```
Trinity-Engine/src/Trinity/Physics/
|-- 3D/
|   |-- PhysicsWorld3D.h/.cpp
|   |-- Rigidbody3D.h/.cpp
|   |-- Collider3D.h/.cpp
|   |-- CharacterController3D.h/.cpp    <- stub
|   └-- PhysicsAllocator3D.h/.cpp
```

### 34.3 — PhysicsWorld3D

Owns `PxFoundation`, `PxPhysics`, `PxScene`, and `PxDefaultCpuDispatcher`. Foundation and Physics are created once at engine startup. Scene is created when Play is entered and destroyed on Stop, matching the Box2D lifecycle.

```cpp
class PhysicsWorld3D : public PhysicsAPI
{
public:
	void Initialize() override;
	void Shutdown() override;
	void Step(float fixedDeltaTime) override;
	void SetGravity(float x, float y) override {}
	void SetGravity3D(float x, float y, float z);

private:
	physx::PxFoundation* m_Foundation = nullptr;
	physx::PxPhysics* m_Physics = nullptr;
	physx::PxScene* m_Scene = nullptr;
	physx::PxDefaultCpuDispatcher* m_Dispatcher = nullptr;
	physx::PxDefaultAllocator m_Allocator;
	physx::PxDefaultErrorCallback m_ErrorCallback;
};
```

### 34.4 — Components

```cpp
struct Rigidbody3DComponent
{
	enum class BodyType : uint8_t { Static = 0, Kinematic, Dynamic };
	BodyType Type = BodyType::Dynamic;
	float Mass = 1.0f;
	float LinearDamping = 0.0f;
	float AngularDamping = 0.05f;
	bool DisableGravity = false;
	void* RuntimeActor = nullptr;	// PxRigidActor*, opaque outside Physics/3D/
};

struct BoxCollider3DComponent
{
	glm::vec3 Offset = glm::vec3(0.0f);
	glm::vec3 Size = glm::vec3(0.5f);
	float StaticFriction = 0.5f;
	float DynamicFriction = 0.5f;
	float Restitution = 0.6f;
	void* RuntimeShape = nullptr;
};

struct SphereCollider3DComponent
{
	glm::vec3 Offset = glm::vec3(0.0f);
	float Radius = 0.5f;
	float StaticFriction = 0.5f;
	float DynamicFriction = 0.5f;
	float Restitution = 0.6f;
	void* RuntimeShape = nullptr;
};

struct CapsuleCollider3DComponent
{
	glm::vec3 Offset = glm::vec3(0.0f);
	float Radius = 0.5f;
	float HalfHeight = 1.0f;
	float StaticFriction = 0.5f;
	float DynamicFriction = 0.5f;
	float Restitution = 0.6f;
	void* RuntimeShape = nullptr;
};
```

A triangle-mesh collider type is a deliberate gap pending PhysX cooking integration.

### 34.5 — Scene Integration

`Scene::OnPhysicsStart3D()` walks all `Rigidbody3DComponent` entities, creates `PxRigidStatic` or `PxRigidDynamic` actors, attaches shapes from collider components, and adds actors to the `PxScene`. Each step calls `scene->simulate(fixedDeltaTime)` followed by `scene->fetchResults(true)` and writes back positions and orientations to `TransformComponent` via `PxRigidDynamic::getGlobalPose()`.

### 34.6 — Character Controller (Stub)

`CharacterController3DComponent` is declared with a `void* RuntimeController` field. `PhysicsWorld3D` contains a `PxControllerManager*` initialised to `nullptr`. The controller implementation — `PxCapsuleController`, step-offset, slope limit, `move()` call — is a deliberate gap flagged with a `// TODO: PhysX character controller` comment.

**Milestone:** A dynamic sphere with a `SphereCollider3DComponent` falls onto a static plane with a `BoxCollider3DComponent`, bounces, and comes to rest. Simulation is stable at 60 Hz fixed step.

---

## Phase 35 — Audio (OpenAL Soft)

### 35.1 — Dependency

OpenAL Soft is added as a Git submodule under `Trinity-Engine/vendor/openal-soft`. It is built as a static library with `ALSOFT_EXAMPLES = OFF`, `ALSOFT_UTILS = OFF`, `ALSOFT_TESTS = OFF`. No OpenAL header ever appears outside `Trinity/Audio/`.

### 35.2 — Folder Structure

```
Trinity-Engine/src/Trinity/Audio/
|-- AudioEngine.h/.cpp
|-- AudioBus.h/.cpp
|-- AudioMixer.h/.cpp
|-- AudioClip.h/.cpp
|-- AudioSource.h/.cpp
|-- AudioListener.h/.cpp
|-- Platform/
|   └-- OpenAL/
|       |-- OpenALAudioEngine.h/.cpp
|       |-- OpenALAudioSource.h/.cpp
|       └-- OpenALUtils.h
```

### 35.3 — AudioEngine Abstract Interface

```cpp
// AudioEngine.h
#pragma once
#include <memory>
#include <string>

namespace Trinity
{
	class AudioClip;
	class AudioSource;
	class AudioBus;

	class AudioEngine
	{
	public:
		virtual ~AudioEngine() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void Update() = 0;

		virtual std::shared_ptr<AudioClip> LoadClip(const std::string& path) = 0;
		virtual std::shared_ptr<AudioSource> CreateSource() = 0;
		virtual AudioBus& GetMasterBus() = 0;

		static std::unique_ptr<AudioEngine> Create();
	};
}
```

`AudioEngine::Create()` returns an `OpenALAudioEngine`. The factory uses the same compile-time define pattern as `RendererAPI::Create()`.

### 35.4 — AudioClip

Represents a decoded PCM audio buffer. Supports `.wav` and `.ogg` at load time. `.wav` is decoded with a minimal header parser. `.ogg` is decoded with `stb_vorbis` which is already present via the stb submodule. Each clip is uploaded to an OpenAL buffer object on load. Buffer lifetime matches the clip object.

```cpp
struct AudioClip
{
	std::string Path;
	uint32_t SampleRate = 0;
	uint32_t ChannelCount = 0;
	uint32_t SampleCount = 0;
	float DurationSeconds = 0.0f;
	uint32_t ALBuffer = 0;	// opaque outside Audio/Platform/OpenAL/
};
```

### 35.5 — Bus System

`AudioMixer` owns a tree of `AudioBus` nodes. The root is the master bus. Standard children at startup: `SFX`, `Music`, `Voice`, `Ambient`. Each bus has a linear volume in `[0, 1]` and a mute flag. Bus volume is applied multiplicatively down the chain to the final AL source gain.

```cpp
class AudioBus
{
public:
	void SetVolume(float volume);
	float GetVolume() const;
	void SetMute(bool mute);
	bool IsMuted() const;
	AudioBus& CreateChildBus(const std::string& name);
	AudioBus* FindChildBus(const std::string& name);

private:
	std::string m_Name;
	float m_Volume = 1.0f;
	bool m_Muted  = false;
	std::vector<std::unique_ptr<AudioBus>> m_Children;
	AudioBus* m_Parent = nullptr;
};
```

### 35.6 — AudioSource

Wraps an `ALuint` source handle. Exposes `Play()`, `Pause()`, `Stop()`, `SetLoop(bool)`, `SetVolume(float)`, `SetPitch(float)`, `SetPosition(glm::vec3)`, `SetVelocity(glm::vec3)`. Assigns to a bus at creation time; defaults to the SFX bus.

3D positional audio uses OpenAL's distance model (`AL_INVERSE_DISTANCE_CLAMPED`). Reference distance, rolloff, and max distance are configurable per source.

### 35.7 — Components

```cpp
struct AudioSourceComponent
{
	std::string ClipPath;
	std::string BusName = "SFX";
	float Volume = 1.0f;
	float Pitch = 1.0f;
	float MinDistance = 1.0f;
	float MaxDistance = 50.0f;
	bool Loop = false;
	bool PlayOnStart = false;
	void* RuntimeSource = nullptr;	// AudioSource*, opaque outside Audio/
};

struct AudioListenerComponent
{
	// Marks the entity whose transform is used as the AL listener position and orientation.
	// Only one listener is active at a time; the first found in the registry wins.
};
```

### 35.8 — Scene Integration

On Play start, `Scene::OnAudioStart()` iterates all `AudioSourceComponent` entities, creates an `AudioSource` for each, uploads clip if not already cached, and calls `Play()` if `PlayOnStart` is true. Each frame `AudioEngine::Update()` walks `AudioSourceComponent` entities, syncs AL source position from `TransformComponent`, and syncs AL listener position from the entity bearing `AudioListenerComponent`.

**Milestone:** A scene with two spatial audio sources plays looping sounds that attenuate correctly with distance as the editor camera moves in Play mode. The master bus volume slider in the editor affects all sources simultaneously.

---

## Phase 36 — Lua Scripting (sol2)

### 36.1 — Dependency

sol2 is already present as a Git submodule. The Lua runtime is pulled in as a submodule under `Trinity-Engine/vendor/lua`. No Lua or sol2 header ever appears in a public engine header that Forge includes directly.

### 36.2 — Folder Structure

```
Trinity-Engine/src/Trinity/Scripting/
|-- ScriptEngine.h/.cpp
|-- LuaState.h/.cpp
|-- ScriptComponent.h
|-- Bindings/
|   |-- InputBindings.h/.cpp
|   |-- TransformBindings.h/.cpp
|   |-- PhysicsBindings2D.h/.cpp
|   |-- PhysicsBindings3D.h/.cpp
|   |-- AudioBindings.h/.cpp
|   |-- SceneBindings.h/.cpp
|   └-- RendererBindings.h/.cpp
```

### 36.3 — ScriptEngine

Owns a single `sol::state`. Initialises standard libraries, registers all bindings on startup, and exposes a per-frame `Update(float deltaTime)` entry point. The global environment is shared; per-script sandboxing is a deliberate gap.

```cpp
class ScriptEngine
{
public:
	static void Initialize();
	static void Shutdown();

	static void LoadScript(const std::string& path);
	static void ReloadScript(const std::string& path);

	static void OnRuntimeStart(Scene* scene);
	static void OnRuntimeStop();
	static void OnUpdate(float deltaTime);

	static sol::state& GetState();

private:
	static sol::state s_LuaState;
	static Scene* s_ActiveScene;
};
```

### 36.4 — ScriptComponent

```cpp
struct ScriptComponent
{
	std::string ScriptPath;
	bool Active = true;
};
```

Each entity with a `ScriptComponent` is expected to define three Lua functions named `OnStart`, `OnUpdate`, and `OnStop`. The script file is loaded once per entity at Play start. `OnUpdate` is called each frame with `deltaTime` and the entity handle.

### 36.5 — Engine API Exposed to Lua

All bindings use sol2 `new_usertype`. No raw Lua C API is used directly.

**Input:**
```lua
if Input.KeyDown(Key.W) then ... end
if Input.MouseButtonPressed(Mouse.Left) then ... end
local pos = Input.MousePosition()
```

**Transform (via entity handle):**
```lua
local t = entity:GetTransform()
t.Position = Vector3(0, 1, 0)
t.Rotation = t.Rotation + Vector3(0, dt, 0)
entity:SetTransform(t)
```

**Physics 2D:**
```lua
local rb = entity:GetRigidbody2D()
rb:ApplyForce(Vector2(0, 500))
rb:SetLinearVelocity(Vector2(5, 0))
```

**Physics 3D:**
```lua
local rb = entity:GetRigidbody3D()
rb:ApplyImpulse(Vector3(0, 10, 0))
```

**Audio:**
```lua
local src = entity:GetAudioSource()
src:Play()
src:SetVolume(0.5)
```

**Scene (entity queries):**
```lua
local other = Scene.FindEntityByTag("Player")
local distance = Vector3.Distance(self:GetTransform().Position, other:GetTransform().Position)
```

**Contact callbacks (registered at OnStart):**
```lua
function OnStart(entity)
    Physics.OnContact(entity, function(other)
        Log.Info("Collided with " .. other:GetTag())
    end)
end
```

### 36.6 — Hot-Reload

`ScriptEngine` maintains a `std::filesystem::file_time_type` per loaded script. A background file-watcher thread polls for changes every 200 ms. When a change is detected, the affected script is reloaded by clearing the entity's Lua environment, re-executing the file, and calling `OnStart` again. The reload happens at the start of the next frame, never mid-frame. In Edit mode, no scripts run. Hot-reload is a Play-mode-only feature.

**Milestone:** An entity with a `ScriptComponent` pointing at a `.lua` file moves under physics forces controlled from Lua, reacts to keyboard input, plays an audio cue on collision, and hot-reloads correctly when the file is modified on disk while in Play mode.

---

## Phase 37 — Build Pipeline

This phase produces a standalone, distributable game build from inside Trinity Forge, independent of the CMake build system. It is the final phase of the roadmap.

### 37.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Build/
|-- BuildPipeline.h/.cpp
|-- AssetCooker.h/.cpp
|-- AssetManifest.h/.cpp
|-- PlatformProfile.h
|-- Cookers/
|   |-- MeshCooker.h/.cpp
|   |-- TextureCooker.h/.cpp
|   |-- AudioCooker.h/.cpp
|   |-- SceneCooker.h/.cpp
|   └-- ShaderCooker.h/.cpp
└-- Platform/
    |-- WindowsBuildTarget.h/.cpp
    |-- MacOSBuildTarget.h/.cpp       <- stub
    └-- XboxBuildTarget.h/.cpp        <- stub
```

### 37.2 — Build Pipeline Overview

The pipeline is invoked from Forge via a `Build` menu entry or `Ctrl+B`. It runs entirely on the main thread using a progress-reporting callback so Forge can display a modal progress window. It does not call CMake. The game executable shipped in the output folder is a pre-compiled `TrinityRuntime` binary that is built separately once per platform and bundled inside the engine installation.

The pipeline has five sequential stages:

1. **Validate** — Check that a startup scene is assigned in project settings, all referenced asset paths resolve, and no required components are missing.
2. **Cook assets** — Convert every source asset into a compact binary format optimised for the target platform. Results are written to a temporary `CookedAssets/` directory.
3. **Build manifest** — Produce an `AssetManifest.bin` listing every cooked asset's UUID, cooked path, type, and byte size. The runtime uses this manifest for deterministic asset loading without filesystem enumeration.
4. **Package** — Copy the runtime executable, all cooked assets, the manifest, and any platform-specific redistributables into the output folder.
5. **Report** — Log total asset count, total size, and per-asset-type size breakdown to the Forge log panel.

### 37.3 — Platform Profile

```cpp
// PlatformProfile.h
#pragma once
#include <string>

namespace Trinity
{
	enum class BuildPlatform : uint8_t
	{
		Windows = 0,
		MacOS,          // stub
		Xbox,           // stub
	};

	struct PlatformProfile
	{
		BuildPlatform Platform = BuildPlatform::Windows;
		std::string OutputDirectory;
		std::string ExecutableName = "Game";
		bool DebugBuild = false;
		bool StripDebugInfo = true;
	};
}
```

### 37.4 — Asset Cookers

Each cooker implements a common interface.

```cpp
class AssetCooker
{
public:
	virtual ~AssetCooker() = default;
	virtual bool Cook(const AssetMetadata& source, const std::string& outputPath, const PlatformProfile& profile) = 0;
	virtual std::string GetCookedExtension() const = 0;
};
```

**MeshCooker** — Reads the source asset (`.fbx`, `.obj`, etc.) via Assimp (already cached in the asset registry), serialises vertex and index data into a compact binary interleaved format, and writes a `.trimesh` file. No Assimp processing happens at runtime.

**TextureCooker** — Reads source image via stb_image, generates power-of-two mip levels, and writes a `.tritex` file containing a simple header followed by raw mip data. No stb_image at runtime.

**AudioCooker** — Copies `.wav` files verbatim. Transcodes `.ogg` to `.wav` for platforms that lack Vorbis decode. Writes `.triaudio` files.

**SceneCooker** — Reads the `.trinity` yaml scene file, resolves all asset UUID references to their cooked paths in the manifest, and writes a `.triscene` binary file. The runtime scene loader reads `.triscene` directly without yaml-cpp.

**ShaderCooker** — Copies pre-compiled `.spv` files for Vulkan. For Xbox stub: placeholder. For macOS stub: placeholder.

### 37.5 — Output Layout (Windows)

```
OutputDirectory/
|-- Game.exe                    <- TrinityRuntime.exe renamed
|-- Assets/
|   |-- AssetManifest.bin
|   |-- Meshes/
|   |   └-- <UUID>.trimesh
|   |-- Textures/
|   |   └-- <UUID>.tritex
|   |-- Audio/
|   |   └-- <UUID>.triaudio
|   |-- Scenes/
|   |   └-- <UUID>.triscene
|   └-- Shaders/
|       └-- <name>.vert.spv
|       └-- <name>.frag.spv
└-- (no DLLs — all dependencies statically linked)
```

All dependencies — OpenAL Soft, PhysX, Box2D, SDL3, Vulkan loader — are statically linked into the runtime so the output folder requires no separate DLL installation.

### 37.6 — Forge Build Panel

The Build panel is registered with `PanelManager` alongside the existing panels.

```
Build Settings
──────────────────────────────
Platform: [ Windows ▼ ]
Output Directory: [ Browse... ] C:/MyGame/Build
Executable Name: [ Game ]
Debug Build: [ ]
Strip Debug Info: [x]

Startup Scene: [ Browse... ] Scenes/Main.trinity

──────────────────────────────
[ Build ]
```

Pressing Build opens a modal progress window showing the current stage name and a progress bar. On completion it shows total build time, output folder size, and a button to open the output folder in Windows Explorer (`ShellExecuteW` on Windows).

### 37.7 — macOS and Xbox Stubs

`MacOSBuildTarget` and `XboxBuildTarget` each implement `IBuildTarget` with every method returning a `BuildResult::NotSupported` error. They appear in the platform dropdown but trigger an error dialog when selected. This ensures the architecture is correct without committing to platform-specific toolchain integration before those platforms are active.

**Milestone:** Clicking Build in Forge with Windows selected produces an output folder containing a working `Game.exe` that loads the startup scene, renders it through the full Vulkan pipeline, and runs physics, audio, and Lua scripts — all from cooked binary assets with no source files present.

---

## Phase 38 — Temporal Anti-Aliasing and Velocity Buffer

The first item in the post-processing chain that pays for the velocity attachment in the GBuffer. Required infrastructure for FSR2 / DLSS / XeSS later.

### 38.1 — Jittered Projection

Each frame, the camera's projection matrix is offset by a sub-pixel jitter from a Halton(2,3) sequence. The jitter is exposed to shaders via the frame UBO. SubFrame reset happens when the jitter loop wraps (default 8 samples).

### 38.2 — Velocity Computation

The geometry pass writes per-pixel screen-space motion vectors into the velocity attachment, computed as the difference between current and previous clip-space positions, undoing the jitter. Skinned and animated meshes write correct velocity using their previous bone palette / object transform.

### 38.3 — VulkanTaaPass

Runs after the lighting pass. Reads the lit current-frame colour, the velocity buffer, and the previous frame's TAA history. For each pixel: reproject using velocity, fetch history sample with Catmull-Rom filtering, perform variance-clipping against the current-frame's neighbourhood, blend current and clipped history with a weight chosen by neighbourhood-disocclusion measure. Output replaces both the lit colour going into bloom and the next frame's history.

### 38.4 — Disocclusion Heuristics

Velocity-based disocclusion (where the previous-frame UV is out of bounds) and depth-based disocclusion (where the reprojected depth differs from the current depth) both reduce history weight. Areas with new geometry retain noise for a frame rather than ghost.

**Milestone:** A moving scene shows clean edges with no shimmering. Sub-pixel detail (foliage, fine geometry) is resolved cleanly. Disocclusion artefacts are bounded and recover within a frame or two.

---

## Phase 39 — Screen-Space Reflections

Adds SSR for surfaces with sufficient roughness to benefit. Falls back to the IBL specular term outside SSR's reach.

### 39.1 — VulkanSsrPass

Compute pass at half resolution. Reads GBuffer depth, normal, MR. Per pixel with roughness below a threshold: compute reflection ray from view direction and surface normal, march the ray through the Hi-Z depth pyramid produced by Phase 27, accumulate hit colour from the lit output. Output is a single `RGBA16F` half-res texture.

### 39.2 — Bilateral Upsample

Same upsample machinery as GTAO produces a full-resolution SSR target.

### 39.3 — Composite

The lighting pass blends SSR with the IBL specular term using a hit-confidence weight. Pixels where the ray exited the screen or hit nothing inherit the IBL term entirely. Roughness controls a blur applied to the SSR sample (mip selection from the lit-output mip chain).

**Milestone:** Shiny surfaces (metal, water, polished floor) reflect actual scene content. Reflections degrade gracefully into IBL at screen edges and at roughness boundaries.

---

## Phase 40 — Volumetric Fog

Adds froxel-based volumetric fog with shadow integration.

### 40.1 — Froxel Grid

A `160×90×128` 3D texture (`R11G11B10F` for in-scattering, `R8` for extinction) is allocated. The grid spans the camera frustum with exponential Z-distribution.

### 40.2 — Light Injection Compute

One compute pass per frame walks every froxel: compute world position, query the cluster light list for that froxel's position, accumulate in-scattered light from each contributing light (shadowed appropriately via the shadow atlas), apply phase function (Henyey-Greenstein with configurable anisotropy).

### 40.3 — Ray-March Integration

A second compute pass marches each (x,y) column of the froxel grid front-to-back, accumulating in-scattering and transmittance. Output is a froxel-grid-sized 3D texture sampled by the lighting pass and the transparent pass.

### 40.4 — Density Sources

Density per froxel is sourced from a global fog density value, optional artist-placed density volumes (boxes / spheres of fog), and an optional 3D noise texture for non-uniform fog.

**Milestone:** A scene with a directional sun shining through a foggy interior shows correct god rays and shadow shafts. Fog density volumes produce localised mist. Frame cost is consistent regardless of fog complexity.

---

## Phase 41 — Decals

Adds deferred decal projection for bullet holes, sprayed signage, blood splatters, and similar.

### 41.1 — Decal as Box

A decal is a unit cube mesh with a material assigned. The decal pass renders these cubes as box draws after the GBuffer is populated, but before the lighting pass. The fragment shader reconstructs world position from the GBuffer depth, projects it into decal local space, samples the decal's textures if inside the unit cube, and writes the result into the GBuffer attachments (additive blend on albedo, replace on normal where roughness is below a threshold).

### 41.2 — Sort and Layer

Decals carry a layer index that controls their write order. Static decals are batched into a single mesh per scene at cook time; dynamic decals are submitted at runtime.

### 41.3 — GBuffer Mask

A per-pixel mask flag in the GBuffer A alpha channel marks pixels that have received a decal. Lighting and post-processing read this only for diagnostic purposes.

**Milestone:** Spawning a bullet-hole decal at runtime produces a hole that correctly drapes over the underlying geometry, shades correctly under all lighting, and survives across cameras and reflections.

---

## Phase 42 — Particle System

Adds a CPU-driven particle simulation with a GPU-rendered output. GPU simulation is left as a future optimisation.

### 42.1 — Emitter Component

`ParticleEmitterComponent` holds emitter parameters (rate, lifetime, initial velocity, gravity, drag, colour over life curve, size over life curve, texture / atlas). Authored in the editor through curves and ramps.

### 42.2 — Simulation

CPU update walks all live particles, integrates velocity, applies forces, samples life curves, kills expired particles. Output is a per-emitter vertex buffer of particle quads.

### 42.3 — Render

A single `ParticlePass` renders all emitters as instanced quads after the forward transparent pass. Particles use the same lighting code as forward transparency for soft lighting; unlit particles bypass it. Soft-particle depth fade reads the GBuffer depth.

### 42.4 — Sorting

Particles within a single emitter are sorted by camera distance per frame. Cross-emitter sorting is deferred to OIT (Phase 44).

**Milestone:** A campfire emitter produces sparks, a smoke emitter produces drifting volumetric-looking smoke, a magic emitter produces lit additive sparks. The same emitter authoring works in the editor and in Play mode.

---

## Phase 43 — Color Grading and Final Post Chain

Closes the post-processing chain: lens distortion, chromatic aberration, vignette, film grain, motion blur, depth of field, color grading via 3D LUT.

### 43.1 — Pass Order

After tonemap, before swapchain present:
1. Motion blur (per-object via velocity buffer + camera-vector velocity)
2. Depth of field (Bokeh, hexagonal kernel, focus distance + aperture exposed in `RenderPipelineSettings`)
3. Lens distortion + chromatic aberration
4. Vignette + film grain
5. Color grading via 3D LUT (`32×32×32` `RGBA8` 3D texture sampled in HCL or sRGB)

### 43.2 — Authoring

A `PostProcessVolumeComponent` carries all post settings. Volumes are blended by priority and weight, allowing zone-specific grading (e.g. a horror level dim corridor versus an outdoor sunlit area).

**Milestone:** Walking between two post-process volumes produces a smooth transition between their colour grades and DoF settings. Motion blur correctly streaks fast-moving objects without smearing static ones.

---

## Phase 44 — Order-Independent Transparency

Replaces the sorted alpha-blend transparency with an OIT method that handles overlapping translucent surfaces correctly.

### 44.1 — Method

Weighted-Blended OIT (McGuire/Bavoil) is the production choice: cheap, no per-pixel storage, no sort. A second `RGBA16F` accumulation target plus an `R8` reveal target are added as render-graph transient resources. The forward transparent shader writes weighted accumulation; a final composite pass divides and blends onto the lit colour.

### 44.2 — Per-Material Opt-In

Materials choose between `BlendMode::Translucent` (sorted, current path, no OIT) and `BlendMode::OIT` (new path). Both coexist; OIT is used for surfaces where overlap is visually important (smoke, glass shards).

**Milestone:** Two intersecting glass panes at arbitrary angles render correctly without sort artefacts. Particles with `BlendMode::OIT` blend cleanly through one another.

---

## Phase 45 — GPU Profiler Panel and Debug Visualisations

The final tooling phase before the advanced rendering items. Produces the Forge panels that consume the GPU timing and stats infrastructure already built.

### 45.1 — GPU Profiler Panel

`GpuProfilerPanel` displays per-pass GPU time as a stacked bar chart over the last N frames, with hover tooltips showing pass name, average time, peak time, and current frame's draw count. Clicking a pass jumps to that pass's output texture in the debug-view dropdown.

### 45.2 — Debug View Switcher

A dropdown in the menu bar selects what `SceneRenderer::GetFinalOutput` returns: final shaded image, GBuffer albedo, GBuffer normal (decoded), GBuffer MR/AO, GBuffer depth (linear), shadow atlas, SSAO output, light cluster count visualisation, cascade visualisation, motion vectors, TAA history confidence.

### 45.3 — Statistics Panel Expansion

The existing stats panel grows with: triangles per pass, instances per pass, lights per cluster (avg/max), descriptor allocations per frame, transient memory MB, persistent texture memory MB, shadow atlas occupancy %, draw call breakdown by material.

### 45.4 — Render Graph Visualiser

`RenderGraphPanel` renders the compiled graph as a Graphviz-style node diagram inside ImGui. Each node shows pass name, queue, GPU time. Each edge shows the resource that creates the dependency. Clicking a resource node shows its description and current memory size.

**Milestone:** A developer profiling a frame can see exactly which pass costs what, can swap any GBuffer or intermediate to the viewport for inspection, and can read off memory/light/draw counts from the stats panel without leaving the editor.

---

## Phase 46 — Hardware Ray Tracing Foundation

Adds the abstract types and the Vulkan implementations for `VK_KHR_ray_tracing_pipeline` and acceleration structures. Introduces a single ray-traced shadow pass as the first consumer.

### 46.1 — Acceleration Structure Abstractions

```cpp
class BottomLevelAccelerationStructure { ... };
class TopLevelAccelerationStructure { ... };

struct RayTracingPipelineSpecification
{
    std::shared_ptr<Shader> RayGenShader;
    std::vector<std::shared_ptr<Shader>> MissShaders;
    std::vector<std::shared_ptr<Shader>> HitGroupShaders;
    std::vector<std::shared_ptr<DescriptorSetLayout>> DescriptorSetLayouts;
    std::vector<PushConstantRange> PushConstants;
    uint32_t MaxRayRecursionDepth = 1;
};
```

`RendererAPI::CreateBLAS` and `CreateTLAS` plus `CommandList::BuildBLAS`, `BuildTLAS`, `TraceRays` round out the surface.

### 46.2 — BLAS Building

Static meshes have their BLAS built once at load. Skinned meshes have their BLAS rebuilt or refit each frame after compute skinning. Both happen on the async-compute queue.

### 46.3 — TLAS Building

A single TLAS is built per frame containing every visible mesh instance. TLAS build cost is bounded; very high instance counts require batching, which is supported by skipping BLAS instances flagged `RayTracingExcluded`.

### 46.4 — VulkanRayTracedShadowPass

The first feature consumer. A single ray per pixel from the surface point toward the directional sun, accepting any-hit as occluded. Output is a `R8` shadow factor at full or half resolution. At half resolution, a denoise pass smooths the result.

The lighting pass selects ray-traced shadows for the directional sun when `RenderPipelineSettings::RayTracedShadowsEnabled` is true and the device supports ray tracing; otherwise it uses cascade shadows from Phase 11.

**Milestone:** On RT-capable hardware, a scene's directional shadow is produced by a ray-traced pass with no cascade artefacts. On non-RT hardware the engine falls back transparently to cascade shadows.

---

## Phase 47 — Real-Time Global Illumination

Adds dynamic diffuse global illumination using DDGI (Dynamic Diffuse Global Illumination — irradiance probes updated by ray tracing). DDGI is chosen over screen-space methods for stability and over voxel-cone-tracing for memory cost.

### 47.1 — Probe Volume

A regular grid of probes covers the playable region (default 16×8×16 = 2048 probes). Each probe stores a small octahedrally-mapped irradiance texture (8×8 = 64 texels) and a depth texture (16×16). All probes are packed into two atlases.

### 47.2 — Probe Update Pass

A ray-tracing pass dispatches `RaysPerProbe` rays from each probe per frame, recording a hit colour per ray. A compute pass projects the rays onto each probe's octahedral basis, blending into the existing probe contents with a configurable hysteresis (default 0.97).

### 47.3 — Lighting Pass Integration

For each lit pixel, the lighting pass samples the eight nearest probes, weights by trilinear interpolation, weights again by per-probe visibility (using the depth atlas to reject probes occluded from the surface), and accumulates indirect diffuse.

### 47.4 — Fallback

When ray tracing is unavailable, DDGI is bypassed and the engine uses the IBL diffuse term alone, as in Phase 12. The artist-facing `RenderPipelineSettings::GIMode` toggles between `Off`, `Baked`, and `RealtimeDDGI`.

**Milestone:** A scene with a moving directional light shows visibly bouncing indirect light: a red wall lit by sun tints a nearby white wall pink in real time. Probe count and ray budget produce visible quality / cost trade-offs from the settings.

---

## Phase 48 — Mesh Shaders and Variable Rate Shading

Adopts mesh shaders (`VK_EXT_mesh_shader`) and VRS (`VK_KHR_fragment_shading_rate`) for very high triangle-density scenes and shading-rate cost reductions.

### 48.1 — Meshlet Asset

The mesh cooker produces meshlets (small clusters of ~64 triangles each) alongside the existing mesh data. Meshlets carry a bounding cone for back-face culling, a bounding sphere for frustum culling, and a vertex/index range.

### 48.2 — Mesh Shader Geometry Pass

A `VulkanMeshShaderGeometryPass` is added as an alternative to the existing `VulkanGeometryPass`. The mesh shader path: the task shader culls meshlets per-cluster, the mesh shader produces vertex output for surviving meshlets. Selection between mesh and traditional path is per-mesh based on triangle count.

### 48.3 — Variable Rate Shading

`VulkanRendererAPI` exposes `EnableVariableRateShading` and a per-pass attachment for the shading-rate image. A simple heuristic image is generated from the previous frame's TAA confidence: high-confidence regions shade at half rate. More aggressive heuristics are exposed but off by default.

**Milestone:** On supporting hardware, a scene with extremely high triangle density (e.g. a pre-tessellated cathedral) renders faster through the mesh shader path than through the traditional pipeline. VRS reduces shading cost in steady regions of the image without producing visible artefacts.

---

## Phase 49 — DirectX 12 Backend

Implements the second backend. The renderer abstraction's whole purpose is validated only by this phase.

### 49.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Renderer/DirectX12/
|-- DirectX12RendererAPI.h/.cpp
|-- DirectX12Device.h/.cpp
|-- DirectX12Swapchain.h/.cpp
|-- DirectX12CommandList.h/.cpp
|-- DirectX12CommandQueue.h/.cpp
|-- DirectX12DescriptorAllocator.h/.cpp
|-- DirectX12PipelineCache.h/.cpp
|-- Resources/
|   └-- ... mirror of the Vulkan folder
└-- DirectX12Utilities.h/.cpp
```

### 49.2 — Mapping Strategy

All abstract types map cleanly onto D3D12. The few asymmetries are handled inside the backend:
- Render graph barriers translate `VkImageMemoryBarrier2` to `D3D12_RESOURCE_BARRIER` (resource state mapping table)
- Descriptor sets become `ID3D12DescriptorHeap` ranges referenced via root signatures
- Bindless heap is the natural D3D12 model; the abstraction matches
- `vkCmdDrawIndexedIndirectCount` becomes `ExecuteIndirect` with the matching command signature
- Ray tracing maps to `ID3D12StateObject` and `DispatchRays`
- Mesh shaders map directly

### 49.3 — Backend Selection

`RendererSpecification::Backend` is honoured at runtime. A user can switch between Vulkan and D3D12 by relaunching the engine. Both backends share 100% of the SceneRenderer / pass code.

### 49.4 — Validation

The full test scene from Phase 12 is rendered identically by both backends, verified by perceptual diff against reference images. Any pass that does not match within a small epsilon flags a bug to fix in the backend mapping.

**Milestone:** Trinity runs on D3D12 with the same feature set, the same shaders (compiled to DXIL via DXC instead of SPIR-V via shaderc — the abstract `Shader` accepts a blob in the appropriate IL), and the same editor.

---

## Phase 50 — HDR Display Output

The final renderer phase. Closes the colour pipeline by enabling actual HDR output to compatible monitors.

### 50.1 — HDR Swapchain

`VulkanSwapchain` and `DirectX12Swapchain` detect HDR-capable colour spaces (`VK_COLOR_SPACE_HDR10_ST2084_EXT`, `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`). A user-facing setting under `RenderPipelineSettings` selects between SDR sRGB (default), HDR10 (Rec.2020 PQ), and scRGB (Rec.709 linear FP16).

### 50.2 — Tonemap Variants

`VulkanTonemapPass` gains output-encoding variants. SDR uses ACES filmic to BT.709 sRGB. HDR10 uses an ACEScg-to-BT.2020 transform plus PQ encoding to a maximum nits target (default 1000). scRGB writes linear FP16.

### 50.3 — Reference Workflow

The colour-grading 3D LUT path (Phase 43) stays in ACEScg space so that the same project content correctly grades for both SDR and HDR output. UI (ImGui) is paper-white-clamped so the editor remains readable on HDR displays.

**Milestone:** On an HDR display, the editor and any rendered scene show specular highlights brighter than UI white and visibly above the SDR clipping point. Switching between SDR and HDR output paths preserves the artist's grade on the parts of the image that fit within SDR's range.

---
## Deliberate Gaps — Renderer

The original Phase 1–24 plan listed many renderer items as deliberate gaps. The expanded plan above promotes nearly all of them to first-class phases. The remaining gaps are listed here.

| Gap | Insertion Point | Dependency |
|---|---|---|
| GPU-resident animation skinning compute | New compute pass before geometry | Phase 32 animation complete |
| Subsurface scattering (separable SSSS) | Lighting pass extension | Phase 12 lighting complete |
| Hair / fur rendering (TressFX-style) | New forward pass after deferred resolve | Phase 16 forward transparency complete |
| Eye shading | Material variant inside lighting pass | Phase 12 lighting complete |
| Cloth simulation rendering | New forward pass | Phase 34 3D physics complete |
| Multi-view / VR | `RendererAPI` extension | Long-term, requires VR layer integration |
| Texture virtual / sparse tiled streaming | `StreamingManager` extension | Phase 30 base streaming complete |
| Per-pass async upload coalescing | `VulkanUploadQueue` | Phase 5 upload queue complete |

## Deliberate Gaps — Editor

| Gap | Location | Dependency |
|---|---|---|
| Asset picker popup in Inspector | `InspectorPanel` | Asset registry UUID system — unblocked in Phase 31 |
| Texture thumbnail generation | `ContentBrowserPanel` | Async texture loading |
| Gizmo toolbar icon buttons | `ViewportPanel` | Icon texture atlas |
| Entity parenting and hierarchy tree | `SceneHierarchyPanel` | Phase 31 scene parenting |
| Drag-and-drop entity reorder | `SceneHierarchyPanel` | Phase 31 scene parenting |
| Undo / redo | `ForgeLayer` | Command pattern implementation |
| Material editor panel | New panel | Phase 6 material asset system complete |
| Animation panel with curve editor | New panel | Phase 32 animation complete |
| Console command input | `LogPanel` | Phase 36 scripting complete |
| Physics collider shape gizmos | `ViewportPanel` | Visual polish phase after Phase 34 |
| Rigidbody inspector visual aids | `InspectorPanel` | Visual polish phase after Phase 33 and 34 |
| Audio bus mixer panel | New panel | Phase 35 audio complete |
| Post-process volume editor | `InspectorPanel` extension | Phase 43 colour grading complete |
| Shadow atlas debug overlay | `RenderGraphPanel` extension | Phase 11 cascades complete |

## Deliberate Gaps — ECS and Scene

| Gap | Location | Dependency |
|---|---|---|
| Prefab overrides and variant system | `Prefab.h/.cpp` | Base prefab system in Phase 31 complete |
| Scene streaming / additive loading | `Scene.h/.cpp` | Phase 30 streaming complete |
| Per-entity static / dynamic flags | `Scene` + renderer | Phase 7 GBuffer velocity attachment complete |

## Deliberate Gaps — Physics

| Gap | Location | Dependency |
|---|---|---|
| Triangle-mesh 3D collider | `PhysicsWorld3D` | PhysX cooking pipeline |
| PhysX character controller | `CharacterController3D.h/.cpp` | Phase 34 base complete |
| Joints (hinge, fixed, spring) — 2D | `PhysicsWorld2D` | Phase 33 base complete |
| Joints (hinge, fixed, spring) — 3D | `PhysicsWorld3D` | Phase 34 base complete |
| Physics material asset | `AssetRegistry` | Phase 31 asset registry |
| Continuous collision detection | `Rigidbody2DComponent` | Phase 33 base complete |
| Overlap / trigger queries | `PhysicsWorld2D`, `PhysicsWorld3D` | Both physics phases complete |

## Deliberate Gaps — Audio

| Gap | Location | Dependency |
|---|---|---|
| Reverb and environmental effects | `OpenALAudioEngine` | Phase 35 base complete |
| Audio clip streaming for long tracks | `AudioClip` | Replaces full-decode-on-load |
| HRTF binaural audio | `OpenALAudioEngine` | Phase 35 base complete |
| Audio snapshot / preset system | `AudioMixer` | Phase 35 base complete |

## Deliberate Gaps — Scripting

| Gap | Location | Dependency |
|---|---|---|
| Per-script sandboxed Lua environments | `ScriptEngine` | Phase 36 base complete |
| Lua coroutine scheduler | `ScriptEngine` | Phase 36 base complete |
| Visual script graph editor | New panel in Forge | Phase 36 scripting complete |
| C# scripting alternative | New scripting phase | Long-term roadmap item |

## Deliberate Gaps — Build Pipeline

| Gap | Location | Dependency |
|---|---|---|
| macOS build target implementation | `MacOSBuildTarget.h/.cpp` | macOS platform active |
| Xbox GDK build target implementation | `XboxBuildTarget.h/.cpp` | Xbox platform active |
| Incremental cook (only re-cook changed assets) | `AssetCooker` | Phase 37 base complete |
| Code signing for Windows builds | `WindowsBuildTarget` | Phase 37 base complete |
| Installer generation | `WindowsBuildTarget` | Deferred — flat folder is sufficient initially |

---

## Master Execution Summary

| Phase | Area | Deliverable | Validation Criterion |
|---|---|---|---|
| 1 | Foundation | Repo restructure, platform rename, shader build | Project builds, window opens, input works |
| 2 | Renderer | Abstract renderer interface (incl. descriptors, command list, compute) | Headers compile with zero Vulkan includes |
| 3 | Renderer | Vulkan backend skeleton (multi-queue, pipeline cache, bindless features) | Device init, swap chain, validation clean |
| 4 | Renderer | Render graph (transient aliasing, async compute, IRenderPass) | Multi-pass test with cross-queue sync |
| 5 | Renderer | Vulkan resource implementations (incl. compute pipeline, query pool, upload queue) | All resource types validated, async upload streams |
| 6 | Renderer | Descriptor convention + material system | Mesh with PBR material renders into GBuffer |
| 7 | Renderer | Render pass architecture, GBuffer, shadow pass | All six GBuffer attachments populated, atlas cascade 0 lit |
| 8 | Foundation | Application integration | Full frame loop, resize works |
| 9 | Foundation | Shader build system + permutations + hot-reload | Shader edit reloads in next frame |
| 10 | Renderer | Reverse-Z, GPU timing, debug labels, frame UBO ring | Long-distance scene shows correct depth, RenderDoc readable |
| 11 | Renderer | Cascaded shadow maps | Stable filtered shadows, four cascades |
| 12 | Renderer | PBR deferred lighting + skybox + IBL | Recognisably AAA-grade lit scene |
| 13 | Renderer | HDR pipeline (bloom, exposure, ACES tonemap) | Final image is gamma-correct and tone-mapped |
| 14 | Renderer | GTAO | Contact darkening, indirect-only modulation |
| 15 | Renderer | Punctual lights + clustered culling | 100+ lights with shadows |
| 16 | Renderer | Forward transparency | Translucent + refractive materials render correctly |
| 17 | Editor | ImGui + SDL3 + Vulkan backend | ImGui frame renders, validation clean |
| 18 | Editor | Dockspace | Panels dock and persist layout |
| 19 | Editor | Panel registration system | Panels register, open, close, View menu works |
| 20 | Editor | Main menu bar | All menus render, File hooks serialisation |
| 21 | Editor | Viewport panel + editor camera | Scene renders in viewport, camera flies |
| 22 | Editor | ImGuizmo | Gizmos correct in Vulkan NDC, transforms edit |
| 23 | Editor | Core panels | Hierarchy, Inspector, Content Browser, Stats, Log |
| 24 | Editor | Play / Pause / Stop | State machine correct, scene save/restore works |
| 25 | Editor | Theme and fonts | Consistent dark theme, custom fonts loaded |
| 26 | Renderer | CPU frustum culling | 10000-mesh scene draws only visible meshes |
| 27 | Renderer | GPU-driven indirect draws + LOD | 100000-instance scene at smooth frame rate |
| 28 | Renderer | Bindless descriptors | 5000-material scene draws in one call per blend mode |
| 29 | Renderer | PSO cache polish + multi-threaded recording | Record cost scales with thread count, no compile hitch |
| 30 | Renderer | Streaming + residency budgets | 4× over-budget scene streams without stutter |
| 31 | Scene / Assets | ECS parenting, prefabs, yaml-cpp serialisation, Assimp import, asset registry | FBX loads, scene saves and restores, UUIDs stable |
| 32 | Animation | Skeleton import, clip playback, bone palette upload | Skinned FBX plays embedded animation in viewport |
| 33 | Physics 2D | Box2D integration | Dynamic boxes fall and stack correctly |
| 34 | Physics 3D | PhysX 5 integration | Dynamic sphere bounces on static plane at 60 Hz |
| 35 | Audio | OpenAL Soft, spatial audio, bus mixer | Two spatial sources attenuate correctly |
| 36 | Scripting | Lua via sol2, full component access, hot-reload | Lua drives physics, input, audio; hot-reloads |
| 37 | Build | Custom asset cooker and packager | Output folder runs as standalone game |
| 38 | Renderer | TAA + velocity buffer | Clean edges and stable sub-pixel detail in motion |
| 39 | Renderer | Screen-space reflections | Shiny surfaces reflect actual scene content |
| 40 | Renderer | Volumetric fog | God rays through directional sun |
| 41 | Renderer | Decals | Bullet hole decals drape over geometry |
| 42 | Renderer | Particle system | Lit, soft-particled, emitter-driven effects |
| 43 | Renderer | Color grading + final post chain | Per-volume grading, motion blur, DoF |
| 44 | Renderer | Order-independent transparency | Overlapping translucents render correctly |
| 45 | Editor | GPU profiler panel + debug visualisations | Per-pass GPU times graphed, all GBuffer slots viewable |
| 46 | Renderer | Hardware ray tracing foundation | Ray-traced sun shadows on capable hardware |
| 47 | Renderer | Real-time GI (DDGI) | Visibly bouncing indirect light in real time |
| 48 | Renderer | Mesh shaders + VRS | Triangle-dense scene faster through mesh path |
| 49 | Renderer | DirectX 12 backend | Same scene renders identically on D3D12 |
| 50 | Renderer | HDR display output | Specular highlights above SDR white on HDR display |

---

*End of Master Roadmap — 50 Phases*