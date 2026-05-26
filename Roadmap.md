# Trinity Engine — Master Roadmap
### Vulkan Backend, Commercial-Grade Renderer, Multi-Backend Path, Long-Term Engine Systems

---

## Overview

This is the single authoritative plan for Trinity Engine. It documents what is built, what is partial, and what remains, in execution order. Every phase produces a concrete compilable milestone with a clear validation criterion.

The current focus is the renderer. The goal is to reach a feature surface comparable to commercial engines (Unreal-class) covering PBR direct lighting and shadows, a full post-processing chain, GPU-driven scale, real-time global illumination, and ray-traced features, on the existing thick abstraction so every backend (Vulkan now, DirectX 12 and Metal later) inherits the same scene-side code without modification.

Non-renderer systems (animation, physics 2D and 3D, audio, scripting, build pipeline) remain on the roadmap and follow the renderer expansion. The ImGui-based editor (Trinity-Forge with panels and gizmos) is paused; the engine runs as a renderer-only target until a UI strategy is chosen later.

No phase leaves the engine in a broken state. Every decision is encoded here and does not need to be revisited.

---

## Status Legend

| Marker | Meaning |
|---|---|
| ✅ Complete | Feature is built, validated, integrated |
| 🟡 Partial | Skeleton exists, missing capabilities documented inside the phase |
| ⬜ Planned | Not yet started |

---

## Confirmed Decisions

| Decision | Choice |
|---|---|
| Renderer abstraction | Thick — high-level code sees zero Vulkan |
| Rendering approach | Deferred (GeometryBuffer) with a forward path for transparency |
| Backend selection | Runtime polymorphism via factory; Vulkan now, DX12 and Metal later |
| Shader compilation | shaderc at build-time and editor-time, SPIR-V at runtime, SPIRV-Cross reflection |
| Windowing and Input | SDL3 for Desktop, platform-specific stubs for consoles |
| Console platforms | PlayStation and Nintendo gitignored, Xbox stub present |
| ECS | EnTT |
| Math | GLM |
| 3D model import | Assimp |
| Logging | spdlog (TR_CORE_TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL) |
| Scene serialisation | yaml-cpp |
| 2D physics | Box2D |
| 3D physics | PhysX 5 open-source |
| Audio | OpenAL Soft |
| Scripting language | Lua via sol2 |
| Script workflow | External editor + file-watcher hot-reload |
| Build pipeline | Custom asset cooker + packager, independent of CMake |
| Distribution format | Flat folder — executable + cooked assets |
| Build targets | Windows primary, macOS and Xbox as stubs |
| Editor UI | Paused — engine is renderer-first until a UI strategy is chosen |

---

## Platform Matrix

| Platform | Window / Input | Graphics API | Status |
|---|---|---|---|
| Windows | SDL3 | Vulkan | Active development |
| macOS | SDL3 | MoltenVK (Vulkan over Metal) | Stub |
| Xbox | GDK | DirectX 12 | Stub |
| PlayStation | Sony SDK | GNM / GNMX | Gitignored |
| Nintendo Switch | NVN SDK | NVN | Gitignored |

---

# PART I — Completed Foundations

The work below is done or substantially done. Each phase is summarised for historical reference and partial-status callouts.

## Phase 1 — Repository and CMake Restructure ✅ Complete

The repository is split into `Trinity-Engine` (static library) and `Trinity-Forge` (executable). The platform layer is reorganised into `Platform/Window/{Desktop,Xbox,PlayStation,Nintendo}` and `Platform/Input/{Desktop,Xbox,PlayStation,Nintendo}`, with CMake source filters excluding inactive platforms by regex.

`TrinityShaders` is a CMake custom target that compiles every `.vert.glsl`, `.frag.glsl`, and `.comp.glsl` under `shaders/source/` to `.spv` in `shaders/compiled/` via `Vulkan::glslc`. `TrinityEngine` depends on `TrinityShaders`. The compiled directory is injected via `TRINITY_SHADER_DIR` compile definition.

`Trinity_Platform` (`Desktop` / `Xbox` / `PlayStation` / `Nintendo`) and `Trinity_Renderer` (`Vulkan` / `DirectX12` / `Metal`) CMake options route the right source files into the build.

**Milestone:** Project builds cleanly, window opens, input works, validation layers silent on initialisation.

---

## Phase 2 — Abstract Renderer Interface ✅ Complete

Every abstract type the engine uses is defined in `Trinity/Renderer/`. After this phase, all higher-level code is written against these interfaces without knowing anything about Vulkan.

```
Trinity-Engine/src/Trinity/Renderer/
|-- RendererAPI.h/.cpp
|-- Renderer.h/.cpp
|-- CommandList.h
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
|   |-- Material.h
|-- Camera/
|   |-- Camera.h/.cpp
|   |-- EditorCamera.h/.cpp
|-- RenderGraph/...
|-- Passes/...
|-- SceneRenderer.h/.cpp
```

The interface set is intentionally broader than a basic skeleton: descriptor sets, command lists, compute pipelines, query pools, and the render graph are all first-class abstract types from day one. Adding them later forces a rewrite of every pass; declaring them now costs nothing.

**Milestone:** All abstract headers compile with zero Vulkan includes. `Renderer::Initialize` is callable from `Application`.

---

## Phase 3 — Vulkan Backend Skeleton 🟡 Partial

The Vulkan implementation lives in `Trinity/Renderer/Vulkan/`. The following are built:

- `VulkanInstance` — instance + surface + validation + debug messenger
- `VulkanDevice` — physical device pick, queue family scan, logical device creation
- `VulkanSwapchain` — swapchain create + image views + recreate-on-resize, mailbox-preferred FIFO fallback, dynamic-rendering only
- `VulkanAllocator` — VMA wrapper
- `VulkanCommandPool` — per-frame command pools
- `VulkanCommandList` — full abstract CommandList implementation
- `VulkanSyncObjects` — per-frame image-available + render-finished semaphores + in-flight fence, plus persistent timeline semaphores for the upload queue
- `VulkanDescriptorAllocator` — two-tier (persistent + transient ring) allocator
- `VulkanBindlessHeap` — 16k-capacity bindless heap allocated but not yet routed through the production path
- `VulkanPipelineCache` — full disk-persistence with VkPipelineCacheHeader compatibility check
- `VulkanUploadQueue` — batched ring-buffered staging on the transfer queue with timeline semaphore sync and queue-family ownership acquire barriers
- `VulkanTransientPool` — transient image allocator hooked into the render graph
- `VulkanDebug` — `VK_EXT_debug_utils` name/label/marker wrappers

**Partial — closed in Phase 9:**
- `VulkanDevice::CreateLogicalDevice` does not yet enable Vulkan 1.2 / 1.3 feature structs: `dynamicRendering`, `synchronization2`, `bufferDeviceAddress`, `descriptorIndexing` (with `descriptorBindingPartiallyBound`, `runtimeDescriptorArray`, `shaderSampledImageArrayNonUniformIndexing`, `descriptorBindingSampledImageUpdateAfterBind`), `timelineSemaphore`, `scalarBlockLayout`, `shaderInt64`, `multiDrawIndirect`, `drawIndirectCount`, `drawIndirectFirstInstance`. The current device works because Vulkan 1.3 promotes some of these to core, but the explicit feature enable is required for the bindless / GPU-driven / async-compute phases.
- HDR colorspaces (`VK_COLOR_SPACE_HDR10_ST2084_EXT`, `VK_COLOR_SPACE_BT2020_LINEAR_EXT`) are not detected. Picked up in Phase 36.
- Dedicated async-compute and transfer queue families are detected and the upload queue uses them, but no async-compute scheduler exists yet (Phase 31).

**Milestone:** Vulkan initialises and shuts down cleanly, validation silent, pipeline cache persists.

---

## Phase 4 — Render Graph ✅ Complete

The render graph lives in `Trinity/Renderer/RenderGraph/` (abstract) and `Trinity/Renderer/Vulkan/RenderGraph/` (Vulkan implementation). Resources are declared via `RenderGraphResourceHandle` (texture or buffer index) with `RenderGraphTextureDescription` / `RenderGraphBufferDescription`. Passes are built fluently via `AddPass(name).SetType(...).Read(handle, access).Write(handle, access).SetExecuteCallback([](RenderGraphContext& ctx){...})`.

`RenderGraphAccess` covers ColorAttachmentRead/Write, DepthStencilRead/Write, ShaderSampledRead, ShaderStorageRead/Write/ReadWrite, IndirectRead, TransferRead/Write. The base `RenderGraph` runs validation, builds execution order, culls passes whose outputs are not consumed, computes resource lifetimes, and allocates resources via virtual hooks. `VulkanRenderGraph` overrides the hooks to allocate from `VulkanTransientPool`, build `VkImageMemoryBarrier2` between pass transitions, emit debug labels around each pass, and begin/end dynamic-rendering scopes for graphics passes.

Imported resources (the swapchain texture, for example) are bound with `ImportTexture(name, ptr, initialAccess)` and can be re-pointed each frame via `SetImportedTexture(handle, ptr)`.

**Milestone:** Multi-pass render graph compiles, culls unused passes, allocates aliased transient resources, executes with correct barriers, dumps text and DOT visualisations.

---

## Phase 4.5 — CommandList Abstraction ✅ Complete

The abstract `CommandList` is the only object passes ever record into. The full surface is implemented: `Begin/End`, `BeginRendering/EndRendering`, viewport / scissor / depth bias, `BindPipeline`, `BindComputePipeline`, `BindDescriptorSet`, `PushConstants`, `BindVertexBuffer`, `BindIndexBuffer`, `Draw`, `DrawIndexed`, `DrawIndexedIndirect`, `DrawIndexedIndirectCount`, `Dispatch`, `DispatchIndirect`, `TransitionResource`, `BufferBarrier`, `CopyBuffer`, `CopyBufferToTexture`, `BlitTexture`, `GenerateMips`, `WriteTimestamp`, `ResetQueryPool`, and the debug-label trio.

`VulkanCommandList` implements every method by recording to its owned `VkCommandBuffer`, tracking `m_BoundGraphicsPipeline` / `m_BoundComputePipeline` / `m_BoundLayout` / `m_LastBindWasCompute` so descriptor set and push constant bindings route to the correct pipeline bind point.

`RenderGraphPass::SetExecuteCallback` signature is `std::function<void(RenderGraphContext&)>` and the context exposes `GetCommandList()` returning an abstract `CommandList&`.

**Milestone — verified:** `grep -r "vkCmd\|vulkan/vulkan.h\|VulkanRendererAPI\|dynamic_cast<Vulkan" Trinity-Engine/src/Trinity/Renderer/SceneRenderer.cpp Trinity-Engine/src/Trinity/Renderer/Passes/*.cpp` returns nothing.

---

## Phase 5 — Vulkan Resource Implementations ✅ Complete

Every abstract resource has a concrete Vulkan implementation:

- `VulkanBuffer` — `VkBuffer + VmaAllocation`, routes GPU buffers through `VulkanUploadQueue`, mapped writes for `CPUToGPU`
- `VulkanTexture` — `VkImage + VkImageView + VmaAllocation`, handles 2D/3D/cube/array/multisampled, `GetOpaqueHandle()` returns the view as `uint64_t`
- `VulkanFramebuffer` — container of attachment textures (no `VkFramebuffer` under dynamic rendering)
- `VulkanShader` — loads SPIR-V from path or blob, creates one `VkShaderModule` per stage
- `VulkanPipeline` — graphics, dynamic state for viewport / scissor / depth bias, uses `VkPipelineRenderingCreateInfo`
- `VulkanComputePipeline` — compute counterpart
- `VulkanSampler` — straight `VkSampler` from spec
- `VulkanDescriptorSetLayout` — built from binding spec
- `VulkanDescriptorSet` — lazy / batched `VkWriteDescriptorSet`, `Flush` issues `vkUpdateDescriptorSets`
- `VulkanQueryPool` — timestamp / occlusion / pipeline-statistics

**Partial — closed in Phase 20:** `VulkanShader` does not yet run SPIRV-Cross reflection. Pipeline specs require manual `DescriptorSetLayoutSpecification` from C++ that must match the GLSL by hand. This is a known bug source.

**Milestone:** All resource types create and destroy without validation errors, VMA reports no leaks, descriptor allocator round-trips across multiple frames.

---

## Phase 6 — Material Skeleton 🟡 Partial

`Material.h` defines `MaterialProperties` (BaseColor, Metallic, Roughness, AmbientOcclusion, EmissiveColor, EmissiveStrength, AlphaCutoff, AlphaMode), `MaterialTextureSlot` (UUID + shared_ptr<Texture> + Enabled flag), `MaterialAlphaMode` (Opaque / Masked / Blend), and a thin `Material` class wrapping the properties.

`MaterialComponent` (in `Scene/Components/`) carries an `AssetHandle MaterialAssetUUID`, a `MaterialData` shared_ptr, an `OverrideProperties` block, and a `UseOverrideProperties` flag. Scene serialisation round-trips all of this through yaml-cpp.

**Partial — closed in Phase 11:** the runtime material has no `Bind(cmd)`, no descriptor set caching, no shader-driven parameter system, no sort key, no blend mode beyond `AlphaMode`. The current renderer treats the material as inline push-constant data only. The real shader-asset + descriptor-set-2 + parameter UBO architecture is Phase 11.

**Milestone — current:** A `MaterialComponent` is editable in C++, serialises to and from yaml.
**Milestone — Phase 11:** A material binds itself, supports textures, and sorts correctly.

---

## Phase 7 — Geometry Pass Skeleton 🟡 Partial

`Trinity/Renderer/Passes/SceneRenderPassContext.h` declares `SceneRenderGraphResources` (Albedo, Normal, MetallicRoughnessAO, Depth handles) and `SceneRenderPassContext` (Width, Height, Stats*, ActiveCamera*, SceneData*, DrawList*).

`Trinity/Renderer/Passes/GeometryPass.h/.cpp` is the live pass. It owns a pipeline, a descriptor set layout (albedo + normal as combined image samplers), a sampler, default 1×1 white and default-normal textures, and the geometry shader. `DeclareResources(graph, resources)` declares the four GBuffer textures in the graph. `AddToGraph(graph, resources, context)` chains the writes and binds the execute callback. `Execute` walks the draw list, allocates a transient descriptor set per draw (writing albedo / normal samplers), and records `BindPipeline → BindDescriptorSet → BindVertexBuffer → BindIndexBuffer → PushConstants → DrawIndexed`.

**Partial — closed in Phases 9 + 13:**
- As of current codebase checkpoint: `geometry_pass.frag.glsl` writes 3 outputs and `GeometryPass.cpp` binds 3 color attachments (Albedo, Normal, MetallicRoughnessAO); MRT count alignment is in place as the baseline for follow-up phase work.
- The pass writes only `Albedo + Normal + MetallicRoughnessAO + Depth` (4 attachments). The full 6-attachment GBuffer (Emissive, Velocity) lands in Phase 13.
- Allocating a transient descriptor set per draw per frame does not scale to 1k+ draws. Replaced by bindless in Phase 30.

**Milestone — current:** A cube renders into the GBuffer at the right transform.
**Milestone — Phase 13:** All six GBuffer attachments populate from the geometry pass.

---

## Phase 8 — Application Integration ✅ Complete

`Application::Application` initialises the window, the renderer (with `RendererBackend::Vulkan`, `MaxFramesInFlight=2`, validation on in `TRINITY_DEBUG`), and the asset registry.

`Application::~Application` calls in order: `Renderer::WaitIdle → LayerStack::Shutdown → AssetRegistry::Clear → Renderer::Shutdown → Window::Shutdown`.

`Application::Run` loop: `Time::Update → DesktopInput::BeginFrame → Window::OnUpdate → drain EventQueue → check ShouldClose/Minimized → for-each Layer::OnUpdate(dt) → Renderer::BeginFrame → for-each Layer::OnRender → Renderer::EndFrame → Renderer::Present`. The ImGui begin/end calls present in the original Phase 8 specification are absent (ImGui is removed).

Window resize routes through `Application::OnEvent → Renderer::OnWindowResize(width, height) → VulkanRendererAPI::OnWindowResize → VulkanSwapchain::Recreate`.

**Milestone:** Full frame loop runs, resize works, validation clean at runtime, scene renders to swapchain via `SceneRenderer::SetRenderTarget(Renderer::GetSwapchainTexture())`.

---

## Phase F-Scene — Scene / ECS / Asset / Platform ✅ Complete (basic form)

**Scene system** (`Trinity/Scene/`): `Scene` owns an `entt::registry` and a `unordered_map<UUID, entt::entity>` for stable lookup. `CreateEntity`, `CreateEntityWithUUID`, `DestroyEntity` (cascades through hierarchy). `Entity` is a handle wrapper with templated `AddComponent / GetComponent / HasComponent / RemoveComponent` in `Entity.inl`. `SceneSerializer` round-trips every component through yaml-cpp.

**Components**: `UUIDComponent`, `TagComponent`, `TransformComponent` (Position / Rotation / Scale / ParentUUID, with `GetLocalMatrix()` and `GetWorldMatrix(scene)`), `MeshComponent`, `MaterialComponent`, `CameraComponent`, `TextureComponent`.

**Asset registry** (`Trinity/Asset/`): singleton, UUID-stable across re-scans via `.meta` sidecar files. `ImportAsset(path)` detects type by extension (`.fbx/.obj/.gltf/.glb/.dae` → Mesh, `.png/.jpg/.jpeg/.tga` → Texture, `.tscene` → Scene). `LoadMesh` / `LoadTexture` cache by handle. `MeshImporter` is Assimp-backed.

**Platform layer** (`Trinity/Platform/`): SDL3-backed `DesktopWindow` (gamepad hot-plug, raw-mouse-delta, cursor lock), `DesktopInput` (static state, per-frame edge clear, key + mouse + scroll + gamepad), `SDLEventTranslator`.

**Utilities**: spdlog wrapper with GLM formatters and `TR_CORE_*` / `TR_*` macros, `Time` for frame delta.

**Geometry**: `Geometry::Vertex` (Position / Normal / UV / Tangent), primitive generators (Triangle / Quad / Cube).

**Gaps deferred to later phases:** prefab system, scene streaming, layer masks, static/dynamic flags, animated component, physics components, audio components, script components, asset cookers.

---

# PART II — Renderer Expansion (Commercial-Grade Path)

The phases below are ordered to deliver a usable renderer at every milestone while building toward Unreal-class capability. Priority order, as confirmed:

1. PBR materials + direct lighting + shadows (Phases 9–18, 19, 20)
2. Post-processing pipeline (Phases 21–27)
3. Performance and scale (Phases 28–32)
4. Global illumination (Phases 33–34)
5. Advanced rendering (Phases 35–38)

---

## Phase 9 — Renderer Hot-Fixes and Vulkan Feature Enable ⬜ Planned

Pre-pass cleanup blocking everything else.

### 9.1 — VulkanDevice Feature Enable

`VulkanDevice::CreateLogicalDevice` is updated to chain the following feature structs into `VkDeviceCreateInfo.pNext`:

```cpp
VkPhysicalDeviceVulkan13Features l_Features13{};
l_Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
l_Features13.dynamicRendering = VK_TRUE;
l_Features13.synchronization2 = VK_TRUE;

VkPhysicalDeviceVulkan12Features l_Features12{};
l_Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
l_Features12.bufferDeviceAddress = VK_TRUE;
l_Features12.descriptorIndexing = VK_TRUE;
l_Features12.descriptorBindingPartiallyBound = VK_TRUE;
l_Features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
l_Features12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
l_Features12.runtimeDescriptorArray = VK_TRUE;
l_Features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
l_Features12.timelineSemaphore = VK_TRUE;
l_Features12.scalarBlockLayout = VK_TRUE;
l_Features12.shaderInt64 = VK_TRUE;
l_Features12.drawIndirectCount = VK_TRUE;
l_Features12.pNext = &l_Features13;

VkPhysicalDeviceFeatures2 l_Features2{};
l_Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
l_Features2.features.samplerAnisotropy = VK_TRUE;
l_Features2.features.multiDrawIndirect = VK_TRUE;
l_Features2.features.drawIndirectFirstInstance = VK_TRUE;
l_Features2.features.shaderInt16 = VK_TRUE;
l_Features2.pNext = &l_Features12;
```

A `VulkanDeviceFeatures` struct records what was successfully enabled. Optional features (`VK_KHR_ray_tracing_pipeline`, `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`, `VK_EXT_mesh_shader`, `VK_KHR_fragment_shading_rate`, `VK_EXT_swapchain_colorspace`) are detected and surfaced via `Supports*()` queries on `VulkanRendererAPI`.

### 9.2 — Geometry Pass MRT Fix

`geometry_pass.frag.glsl` is rewritten to write all three color attachments declared by `GeometryPass::DeclareResources`:

```glsl
#version 450

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_WorldNormal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_WorldTangent;

layout(set = 0, binding = 0) uniform sampler2D u_Albedo;
layout(set = 0, binding = 1) uniform sampler2D u_Normal;

layout(push_constant) uniform PushBlock
{
    mat4 Model;
    mat4 ViewProjection;
    vec4 BaseColor;
    vec4 MaterialData;
    vec4 EmissiveColorStrength;
    vec4 TextureFlags;
} u_Push;

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out vec4 o_MaterialRoughnessAO;

void main()
{
    vec3 l_Normal = normalize(v_WorldNormal);
    vec4 l_BaseColor = u_Push.BaseColor;
    if (u_Push.TextureFlags.x > 0.5) l_BaseColor *= texture(u_Albedo, v_UV);

    o_Albedo = l_BaseColor;
    o_Normal = vec4(l_Normal * 0.5 + 0.5, 1.0);
    o_MaterialRoughnessAO = vec4(u_Push.MaterialData.x, u_Push.MaterialData.y, u_Push.MaterialData.z, 1.0);
}
```

The vertex shader push-constant block is updated to declare the full block matching the C++ `PushBlock` struct.

### 9.3 — HDR Color Format

The GBuffer Normal attachment is already `RGBA16F`; the Albedo attachment moves from `RGBA8` to `RGBA8_SRGB` so the deferred lighting pass in Phase 15 reads gamma-correct albedo. The MR/AO attachment stays `RGBA8`.

**Milestone:** Validation layers silent with all feature flags enabled. Geometry pass writes all three color attachments and the depth attachment. Push-constant ranges align between C++ and GLSL.

---

## Phase 10 — Frame UBO, Reverse-Z, GPU Timing, Debug Labels ⬜ Planned

Foundation infrastructure every subsequent pass relies on.

### 10.1 — Frame UBO

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

Bound at set 0 binding 0. Allocated as a ring of `MaxFramesInFlight` uniform buffers in `SceneRenderer::Implementation`. Updated by `BeginScene` from the camera and submitted scene data.

### 10.2 — Reverse-Z Projection

`Camera::RecalculateProjectionMatrix` switches to reverse-Z infinite-far:

```cpp
m_ProjectionMatrix = glm::infinitePerspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip);
m_ProjectionMatrix[2][2] = 0.0f;
m_ProjectionMatrix[2][3] = -1.0f;
m_ProjectionMatrix[3][2] = m_NearClip;
m_ProjectionMatrix[1][1] *= -1.0f;
```

Depth clear value becomes 0.0, depth test becomes `Greater`, depth attachment format stays `Depth32F`. Long-distance scenes preserve precision in the far plane.

### 10.3 — GPU Timing

`SceneRenderer` owns a `QueryPool` of `MaxPasses * MaxFramesInFlight * 2` timestamp slots. `VulkanRenderGraph::OnExecutePassBegin` writes a start timestamp; `OnExecutePassEnd` writes an end timestamp. `EndFrame` reads back the previous frame's timestamps and populates `SceneRendererStats::*Ms` fields. Per-pass GPU time is queryable via `Stats.PassTimings[name]`.

### 10.4 — Debug Labels

`VulkanRenderGraph` already emits `BeginDebugLabel(pass.GetName(), pass.GetDebugColor())` around each pass. This phase adds named-object debug names to every `Buffer`, `Texture`, `Pipeline`, `Sampler`, `DescriptorSet` created with a non-empty `DebugName` field via `VulkanDebug::SetName(handle, name)`. RenderDoc captures become self-documenting.

**Milestone:** A scene with depth from 0.1m to 100km shows correct z-fighting-free depth, RenderDoc capture is fully named, per-pass GPU times appear in stats.

---

## Phase 11 — Real Material System ⬜ Planned

Replaces the inline push-constant material with a proper shader-asset-driven material with descriptor set 2 binding.

### 11.1 — Descriptor Set Convention

| Set | Lifetime | Contents |
|---|---|---|
| 0 | Per frame | Frame UBO, light buffer SSBO, cluster grid SSBO, environment IBL textures, shadow atlas, bindless texture heap |
| 1 | Per object | Object UBO (model, previous-model, custom data), bound once per draw via dynamic offset |
| 2 | Per material | Material parameter UBO + textures, persistent allocation |
| 3 | Per pass | Pass-specific scratch (SSAO history, bloom mip slot, etc.) |

### 11.2 — MaterialAsset

```cpp
class MaterialAsset
{
public:
    AssetHandle Id = InvalidAsset;
    std::string Name;
    AssetHandle ShaderId = InvalidAsset;
    BlendMode Blend = BlendMode::Opaque;
    bool DoubleSided = false;
    bool CastShadow = true;
    bool ReceiveShadow = true;
    std::unordered_map<std::string, MaterialParameter> Parameters;
    std::unordered_map<std::string, AssetHandle> TextureReferences;
};
```

`BlendMode`: `Opaque`, `Masked`, `Translucent`, `Additive`. `MaterialParameter` is a tagged union of `float`, `vec2`, `vec3`, `vec4`, `int`, `uint`, `bool`. Shader reflection (Phase 20) populates the parameter list from the SPIRV-Cross output.

### 11.3 — Runtime Material

```cpp
class Material
{
public:
    Material(const std::shared_ptr<MaterialAsset>& asset);
    void SetParameter(const std::string& name, const MaterialValue& value);
    void SetTexture(const std::string& name, const std::shared_ptr<Texture>& texture);
    void Bind(CommandList& commandList) const;
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

Each material owns one persistent descriptor set bound at set 2. Parameter writes mark the UBO dirty; flushes happen lazily before bind. The sort key combines shader id, blend mode, and material id for opaque sort and back-to-front sort.

### 11.4 — MaterialLibrary

A singleton owned by `SceneRenderer` that caches materials by UUID, hot-reloads `.material` files in editor builds, and serves the three default materials (`DefaultLit`, `DefaultUnlit`, `DefaultTransparent`).

### 11.5 — MeshDrawCommand Update

`MeshDrawCommand` switches from inline `MaterialProperties` to `std::shared_ptr<Material> MaterialRef`. The GeometryPass binds `MaterialRef` once per material-change in the sorted draw list and issues per-object push constants only for transform + object index.

**Milestone:** A mesh with an assigned material containing albedo / normal / MR / AO textures renders correctly through the GeometryPass. Materials hot-reload without restart. Sort order reduces material switches in a multi-material scene.

---

## Phase 12 — RenderPass Class Architecture ⬜ Planned

Replaces the inline-callback pattern in `GeometryPass::AddToGraph` with a polymorphic `RenderPass` base. `SceneRenderer` holds an ordered list of `unique_ptr<RenderPass>` and walks them each frame.

```cpp
class RenderPass
{
public:
    virtual ~RenderPass() = default;
    virtual const char* GetName() const = 0;
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Setup(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context) = 0;
    virtual bool IsEnabled(const RenderPipelineSettings& settings) const { return true; }
};
```

`SceneRenderer::Initialize` creates the active pass list:

```cpp
m_Implementation->Passes.emplace_back(std::make_unique<GeometryPass>());
m_Implementation->Passes.emplace_back(std::make_unique<ShadowPass>());
m_Implementation->Passes.emplace_back(std::make_unique<LightingPass>());
m_Implementation->Passes.emplace_back(std::make_unique<SkyboxPass>());
m_Implementation->Passes.emplace_back(std::make_unique<BloomPass>());
m_Implementation->Passes.emplace_back(std::make_unique<TonemapPass>());
```

Each pass implements `Setup` to add itself to the graph. `SceneRenderer::Render` calls `Setup` on every enabled pass, then `Graph::Compile()` (cached against a structure hash), then `Graph::Execute()`.

**Milestone:** Adding or removing a pass is a one-line change in the constructor list. The existing GeometryPass is refactored to inherit `RenderPass`. The frame structure becomes trivially extensible.

---

## Phase 13 — GBuffer Expansion ⬜ Planned

Promotes the GBuffer to full 6-attachment production layout.

| Attachment | Format | Contents |
|---|---|---|
| GBuffer A | `RGBA8_SRGB` | Albedo RGB + opacity flag in A |
| GBuffer B | `RGBA16F` | World-space Normal RGB + per-pixel material flags packed in A |
| GBuffer C | `RGBA8` | Metallic R + Roughness G + AO B + specular factor A |
| GBuffer D | `R11G11B10F` | Emissive RGB |
| GBuffer Velocity | `RG16F` | Screen-space motion vectors |
| GBuffer Depth | `Depth32F` | Scene depth (reverse-Z) |

`SceneRenderGraphResources` gains `Emissive` and `Velocity` handles. The geometry pass writes all six. The velocity computation runs in the vertex shader by sampling the current and previous `ViewProjection` matrices (the latter from the Frame UBO's `PreviousViewProjection`) against the current and previous model matrices.

Vertex shader output gains `v_CurrentClipPosition` and `v_PreviousClipPosition`; fragment shader derives velocity from those after the perspective divide.

Material flags packed in Normal.A: bit 0 = double-sided, bit 1 = unlit, bit 2 = alpha-tested, bits 3–7 = material variant id.

**Milestone:** All six attachments populate. Velocity is non-zero for moving objects, zero for static. Emissive shows up in the GBuffer view.

---

## Phase 14 — Cascaded Shadow Maps ⬜ Planned

Adds the shadow pass and the cascaded shadow map system for the directional sun light.

### 14.1 — Shadow Atlas

A single `4096 × 4096` `Depth32F` shadow atlas. Cascades 0–3 occupy quadrants. The atlas is declared in `SceneRenderGraphResources` as a persistent texture (not transient — its depth values feed the lighting pass).

### 14.2 — Cascade Computation

`CSMComputeService::ComputeCascadeMatrices(camera, sunDirection, settings)` returns per-cascade `LightViewProjection` matrices plus split distances. Splits follow a logarithmic-uniform blend (PSSM):

```cpp
const float l_LogSplit = m_NearClip * std::pow(m_FarClip / m_NearClip, static_cast<float>(it_Cascade + 1) / settings.CascadeCount);
const float l_UniformSplit = m_NearClip + (m_FarClip - m_NearClip) * static_cast<float>(it_Cascade + 1) / settings.CascadeCount;
const float l_Split = glm::mix(l_UniformSplit, l_LogSplit, settings.SplitBlendFactor);
```

Each cascade's view matrix looks down the sun direction; the ortho matrix snaps to texel size for shimmer-free shadows.

### 14.3 — ShadowPass

`ShadowPass : RenderPass` declares the shadow atlas as a depth-write attachment, sets up four cascade viewport rectangles, and records depth-only draws using a stripped-down vertex shader (`shadow_pass.vert.glsl`) and an empty fragment shader. The pass walks the draw list once per cascade, frustum-culls against that cascade's ortho frustum, sets `vkCmdSetDepthBias` dynamically per cascade (so bias scales with cascade size), and issues `DrawIndexed`.

An alpha-tested shadow variant exists (`shadow_pass.alphatested.frag.spv`) that samples the material's albedo and discards.

### 14.4 — Shadow Sampling

In the lighting pass (Phase 15), shadow sampling uses Percentage-Closer Filtering with a 5×5 kernel (Vogel disk for non-uniformity, blue-noise rotated). Cascade selection happens per-fragment by comparing view-space depth against the cascade split distances; a small blend region between cascades hides the seam.

**Milestone:** Stable, filtered shadows from a single directional light, four cascades, no Peter-Panning, no shimmer when the camera moves slowly.

---

## Phase 15 — PBR Deferred Lighting + Skybox + IBL ⬜ Planned

The headline phase of the lighting-and-shadows priority. Turns the GBuffer into a fully lit AAA-grade image.

### 15.1 — LightingPass

`LightingPass : RenderPass` reads GBuffer A/B/C/D + Depth + Shadow Atlas + IBL cubemap + BRDF LUT, writes the `SceneColor` attachment (`RGBA16F`).

A full-screen triangle vertex shader emits `gl_Position` from `gl_VertexIndex`; the fragment shader reconstructs world position from depth and the inverse view-projection, samples the GBuffer, evaluates the BRDF, applies sun-light + IBL, and outputs HDR colour.

### 15.2 — PBR BRDF

Cook-Torrance microfacet model:

```glsl
vec3 FresnelSchlick(float l_CosTheta, vec3 l_F0) { return l_F0 + (1.0 - l_F0) * pow(1.0 - l_CosTheta, 5.0); }

float DistributionGGX(vec3 l_Normal, vec3 l_Half, float l_Roughness)
{
    float l_A = l_Roughness * l_Roughness;
    float l_A2 = l_A * l_A;
    float l_NdotH = max(dot(l_Normal, l_Half), 0.0);
    float l_Denominator = (l_NdotH * l_NdotH * (l_A2 - 1.0) + 1.0);
    return l_A2 / (PI * l_Denominator * l_Denominator);
}

float GeometrySchlickGGX(float l_NdotV, float l_Roughness)
{
    float l_K = (l_Roughness + 1.0);
    l_K = (l_K * l_K) / 8.0;
    return l_NdotV / (l_NdotV * (1.0 - l_K) + l_K);
}
```

Energy-conserving combining diffuse Lambert (× `1 - F`) and specular GGX. Metallic mapping: `F0 = mix(vec3(0.04), albedo, metallic)`, `albedo *= (1 - metallic)` for diffuse.

### 15.3 — Image-Based Lighting

Three textures bound at set 0:

- `EnvironmentRadiance` — cubemap, pre-filtered per mip for roughness lookup (split-sum approximation)
- `EnvironmentIrradiance` — cubemap, low-resolution diffuse-convolved environment
- `BrdfLUT` — `RG16F` 2D LUT (NdotV × roughness → scale + bias)

Pre-filtering runs at IBL asset cook time via dedicated compute shaders; at runtime the lighting pass samples them.

### 15.4 — SkyboxPass

`SkyboxPass : RenderPass` reads the environment radiance cubemap and writes into the `SceneColor` attachment at maximum depth (depth test `Equal` with depth value `0.0` under reverse-Z, so the skybox fills any pixel that nothing else wrote). Vertex shader emits a full-screen triangle; fragment shader samples the cubemap using the view direction reconstructed from `gl_FragCoord` and the inverse view-projection.

### 15.5 — DirectionalLightComponent + EnvironmentLighting

```cpp
struct DirectionalLightComponent
{
    glm::vec3 Direction = { 0.0f, -1.0f, 0.0f };
    glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
    bool CastShadows = true;
};

struct EnvironmentLighting
{
    std::shared_ptr<Texture> RadianceCube;
    std::shared_ptr<Texture> IrradianceCube;
    std::shared_ptr<Texture> BrdfLut;
    float SkyIntensity = 1.0f;
};
```

`SceneRenderData::Environment` holds the IBL set. `SceneRenderData::SunLight` holds the directional light data.

**Milestone:** A scene with a textured PBR mesh, a directional light, a skybox, and an IBL probe renders a visually correct, energy-conserving lit image. Shadows from the directional light are present. The reference image is comparable to Sponza-class rendering in commercial engines.

---

## Phase 16 — HDR Pipeline (Bloom, Exposure, ACES Tonemap) ⬜ Planned

Closes the loop on the priority-1 deliverable: a lit scene that looks correct on screen.

### 16.1 — BloomPass

Iterative downsample then upsample on the `SceneColor` HDR target. Six mip levels, each downsample uses a 13-tap filter (Call of Duty SIGGRAPH 2014), upsample uses a 9-tap tent filter. Threshold cutoff in the first downsample (soft knee, controlled by `BloomThreshold` and `BloomSoftKnee` settings).

### 16.2 — Auto Exposure

Compute pass reduces the `SceneColor` into a 256-bucket log-luminance histogram. A follow-up compute reduces the histogram into an average luminance, smoothed across frames via exponential decay. The `Exposure` field of the Frame UBO is updated for the next frame.

Settings: `MinLogLuminance = -10`, `MaxLogLuminance = 2`, `AdaptationSpeed = 1.5`.

### 16.3 — TonemapPass

ACES filmic tonemap on the composited `SceneColor + Bloom`. Operator is the Narkowicz approximation:

```glsl
vec3 ACES(vec3 l_Color)
{
    const float l_A = 2.51;
    const float l_B = 0.03;
    const float l_C = 2.43;
    const float l_D = 0.59;
    const float l_E = 0.14;
    return clamp((l_Color * (l_A * l_Color + l_B)) / (l_Color * (l_C * l_Color + l_D) + l_E), 0.0, 1.0);
}
```

Output is sRGB (gamma 2.2 encode applied here unless writing to an sRGB-format swapchain).

**Milestone:** Final image is gamma-correct and tonemapped. Bright objects bloom convincingly without ringing. The eye adapts when transitioning from a dark interior to a sunlit exterior.

---

## Phase 17 — GTAO (Ground-Truth Ambient Occlusion) ⬜ Planned

Adds high-quality screen-space AO.

### 17.1 — GTAOPass

Compute pass reading GBuffer Normal + Depth + Hi-Z (Phase 29 dependency, but a stub mip-0 Hi-Z works until then). Implements the GTAO algorithm (Jiménez SIGGRAPH 2016): per-pixel horizon-sweep AO with cosine-weighted falloff, two directions per sample, four samples per direction. Output is a single-channel `R8` AO mask.

### 17.2 — Spatial + Temporal Filter

Spatial bilateral filter (depth + normal aware) smooths the AO mask. Temporal accumulation with velocity-based reprojection further denoises across frames; the TAA velocity attachment (Phase 21) provides reprojection. Until TAA lands, the temporal accumulator runs with zero velocity (which is correct for static scenes).

### 17.3 — Lighting Pass Integration

The lighting pass samples the AO mask and modulates the indirect (IBL) term only. Direct lighting is unaffected. `MaterialProperties.AmbientOcclusion` continues to act on the IBL contribution and multiplies with the GTAO result.

**Milestone:** Contact darkening around objects and in crevices is visible and stable. AO does not affect direct light, so dark crevices in sunlight remain bright where the sun hits.

---

## Phase 18 — Punctual Lights + Clustered Culling ⬜ Planned

Scales the renderer from one directional light to hundreds of point and spot lights.

### 18.1 — PointLightComponent + SpotLightComponent

```cpp
struct PointLightComponent
{
    glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
    float Intensity = 100.0f;
    float Radius = 10.0f;
    bool CastShadows = false;
};

struct SpotLightComponent
{
    glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
    float Intensity = 100.0f;
    float Range = 15.0f;
    float InnerConeAngle = 25.0f;
    float OuterConeAngle = 35.0f;
    bool CastShadows = false;
};
```

`SceneRenderData::PointLights` and `SceneRenderData::SpotLights` are filled from the scene each frame.

### 18.2 — Light Buffer SSBO

A persistent SSBO at set 0 binding 1 holds all light data in a packed format. Updated each frame via the upload queue when count or content changes.

### 18.3 — Clustered Culling

A 32 × 18 × 32 froxel grid (configurable). Z slices use logarithmic distribution between near and far. A compute pass per frame:

1. Build froxel AABBs (only when camera or projection changes)
2. For each froxel, list lights whose bounding sphere intersects it
3. Write an index list and offset table into a cluster SSBO

The lighting pass samples the cluster SSBO with `(uv.x * GridX, uv.y * GridY, log(viewDepth) → slice)`, walks the per-cluster light index list, and accumulates contributions.

Shadow casting for point and spot lights uses a dynamic shadow atlas allocator: the available shadow atlas area not consumed by directional cascades is sub-divided per shadow-casting punctual light.

**Milestone:** A scene with 100+ point lights with shadows renders at 60 fps on a mid-range GPU. Cluster visualisation debug view shows correct density.

---

## Phase 19 — Forward Transparency ⬜ Planned

Adds the forward pass for translucent surfaces that need real lighting and shadow reception.

### 19.1 — ForwardTransparentPass

`ForwardTransparentPass : RenderPass` reads SceneColor + Depth + Shadow Atlas + Frame UBO + Light Buffer + Cluster SSBO + IBL textures, writes back into SceneColor with alpha blending. Draws are sorted back-to-front by camera distance.

### 19.2 — Sort and Submit

The `RenderQueue` (lives inside `SceneRenderer`) splits the draw list into opaque, alpha-tested, and translucent buckets. The translucent bucket is sorted back-to-front before submission to the forward transparent pass. The opaque + alpha-tested buckets go through the geometry pass (already sorted by material for state-change minimisation).

### 19.3 — Per-Pixel Refraction Hook

A `MaterialFlag::Refractive` opts a material into reading from a single copy of the lit SceneColor taken once before the transparent pass begins. The forward shader samples this copy with a UV offset (computed from view-space normal × refraction strength) to approximate refraction. This is the engine's path for glass and water without a separate screen-space refraction pass.

### 19.4 — Transparent Shadow Reception

Transparent fragments evaluate shadows the same way opaque fragments do — same shadow atlas, same cascade selection, same filtering kernel.

**Milestone:** A scene mixing opaque and transparent meshes (e.g. a stained glass window in a stone wall) renders with correct PBR shading, correct shadow reception, and correct sort order. Refractive materials show visible refraction.

---

## Phase 20 — Shader Build Pipeline (Permutations + Hot-Reload + Reflection) ⬜ Planned

Closes the long-standing shader-side gap: `VulkanShader` does not yet run reflection, and there is no permutation system.

### 20.1 — SPIRV-Cross Reflection

`VulkanShader` constructor (after building all `VkShaderModule`s) runs SPIRV-Cross on each SPIR-V blob, extracting:

- Descriptor set / binding pairs with type and count
- Push constant range
- Vertex input layout
- Specialisation constants
- Entry point name

The reflection result is cached on the `Shader` base class and exposed via `Shader::GetReflection()`. Pipelines built from the shader can use the reflected layouts directly (zero-config path) or override them.

### 20.2 — Shader Permutation System

Each base shader declares its permutation axes in a `.shaderdef` sidecar:

```yaml
shader: geometry_pass
stages: [vertex, fragment]
permutations:
  - SKINNED: [0, 1]
  - NORMAL_MAP: [0, 1]
  - ALPHA_TEST: [0, 1]
```

The CMake `TrinityShaders` target expands the cross-product into `geometry_pass.SKINNED1_NORMAL_MAP1_ALPHA_TEST0.frag.spv` files. At runtime, `ShaderLibrary::Get(name, defines)` hashes the active define set and looks up the cached SPIR-V blob, falling back to in-process shaderc compilation for editor-only combinations.

### 20.3 — Hot-Reload

In `TRINITY_DEBUG` builds, `ShaderHotReloader` runs a file-watcher thread on `shaders/source/` polling at 200ms intervals. On change, the affected shader recompiles via shaderc, and every pipeline that references it is recreated. Hot-reload happens at frame begin, never mid-frame.

**Milestone:** Editing a `.glsl` file triggers an automatic recompile and pipeline rebuild within one frame. Adding a permutation axis to a shader does not break existing call sites. Reflection-driven descriptor set layouts eliminate the C++ / GLSL drift bug.

---

## Phase 21 — TAA + Velocity Buffer ⬜ Planned

First post-processing-pipeline-priority phase. Replaces jagged edges and sub-pixel shimmer with a stable image.

### 21.1 — Jittered Projection

The Frame UBO's `Jitter` field is set each frame from a Halton(2,3) sequence remapped to ±0.5 pixel offsets. `Projection` is unjittered (used for matrix math); the geometry pass applies the jitter in the vertex shader after the perspective transform.

### 21.2 — Velocity Buffer

Already declared in Phase 13. Used here.

### 21.3 — TAAPass

Reads the current jittered SceneColor + history SceneColor + Velocity + Depth, outputs a resolved SceneColor and a new history. Algorithm:

- Reproject history using velocity
- Clip history to the neighborhood AABB (Karis variance clipping) of the current colour to suppress ghosting on disocclusion
- Blend current × 0.1 + history × 0.9 in YCoCg with luminance-weighted weights
- Apply mild sharpening (Lanczos) to compensate for blur

History is a persistent render-graph texture (lives across frames). On resize, history is invalidated to current frame's pre-resolve image.

**Milestone:** Sub-pixel detail in static scenes is preserved over time. Slow camera motion does not introduce ghosting. Disocclusion (camera reveals previously-hidden geometry) does not show old-frame data.

---

## Phase 22 — Screen-Space Reflections ⬜ Planned

### 22.1 — SSRPass

Compute pass reading SceneColor + Depth + GBuffer Normal + GBuffer MR/AO. For each pixel above a roughness threshold:

1. Compute reflection ray in view space from view direction and normal
2. March the ray against the Hi-Z depth pyramid (Phase 29 dependency; until then, linear march against mip-0)
3. On intersection, sample SceneColor at the hit UV and write into the SSR target
4. Modulate by a Fresnel term and a roughness-derived blur radius

Output is a `RGBA16F` reflection texture composited into the lighting pass as a multiplicative addition to the indirect specular term (replacing the IBL specular where SSR has high confidence).

### 22.2 — Glossy Pre-Filter

A small mip chain of the SceneColor is built once per frame; rougher surfaces sample higher mips for cheap glossy reflections.

**Milestone:** Shiny floor surfaces reflect actual scene content. Roughness controls the apparent glossiness of the reflection. Off-screen content correctly falls back to IBL.

---

## Phase 23 — Volumetric Fog ⬜ Planned

### 23.1 — Froxel Volume

A `160 × 90 × 64` (configurable) 3D texture (`RGBA16F`) holding scattering+absorption per froxel, allocated in the render graph as transient.

### 23.2 — Inscatter Compute

Compute pass per froxel: walk lights affecting this froxel (using the cluster grid from Phase 18), compute Henyey-Greenstein scattering toward the camera, sample shadow maps for shadowed froxels, write the result.

### 23.3 — Integration

Front-to-back integration along view rays, producing a 2D screen-space scattering texture sampled in the lighting pass for the in-scatter term and in the skybox pass for the back-scatter term.

### 23.4 — Fog Volume Component

```cpp
struct FogVolumeComponent
{
    glm::vec3 ScatterColor = { 0.5f, 0.6f, 0.8f };
    float Density = 0.05f;
    float Anisotropy = 0.4f;
    float HeightFalloff = 0.01f;
};
```

**Milestone:** God rays through the directional sun are visible in a foggy scene. Point lights cast volumetric cones. Fog density varies smoothly with the scene's fog volume.

---

## Phase 24 — Decals ⬜ Planned

### 24.1 — Deferred Decals

`DecalComponent` carries an oriented bounding box and a set of material textures (albedo / normal / MR override). A compute or fullscreen pass after the geometry pass projects decals onto the GBuffer by walking each decal, computing affected pixels via OBB intersection with reconstructed world position from depth, and blending into the GBuffer attachments.

### 24.2 — Sort by Layer

Decal layers (`Floor`, `Wall`, `Static`, `Detail`) control which decals affect which surfaces and in which order they composite.

**Milestone:** Bullet hole decals drape correctly over arbitrarily-curved geometry. Decals respect mesh material flags (e.g. they do not project onto sky pixels).

---

## Phase 25 — Particle System ⬜ Planned

### 25.1 — Emitter + Particle Buffer

Particles live in a GPU buffer updated by a compute pass each frame. Emitters spawn new particles, the update compute integrates positions / velocities / lifetimes / colors, and a render compute prepares an indirect draw arg.

### 25.2 — Soft Particle Rendering

A forward pass renders the alive particles as camera-facing billboards (or mesh particles), reading the scene depth buffer to fade alpha near intersections (soft particle blend). Particles inherit cluster-list lighting from the standard cluster grid, so they are lit by the same point lights as the rest of the scene.

### 25.3 — ParticleSystemComponent

```cpp
struct ParticleSystemComponent
{
    AssetHandle EmitterDefinition = InvalidAsset;
    bool Looping = true;
    bool Playing = false;
    float SimulationTime = 0.0f;
};
```

The emitter definition is an asset describing spawn rate, lifetime, velocity distribution, color over lifetime curve, size over lifetime, texture, blend mode.

**Milestone:** A smoke emitter and a sparks emitter render correctly, are lit by scene lights, fade softly against geometry, and play correctly under simulation tick.

---

## Phase 26 — Color Grading + DoF + Motion Blur (Final Post Chain) ⬜ Planned

Closes the post-processing pipeline.

### 26.1 — PostProcessVolumeComponent

```cpp
struct PostProcessVolumeComponent
{
    int Priority = 0;
    float BlendDistance = 1.0f;
    float Exposure = 0.0f;
    AssetHandle ColorLUT = InvalidAsset;
    float MotionBlurStrength = 0.5f;
    float DofFocusDistance = 5.0f;
    float DofAperture = 2.8f;
    float DofFocalLength = 50.0f;
};
```

Multiple volumes blend by priority and proximity to the camera.

### 26.2 — Final Post Pipeline

In order:

1. Depth-of-field (Karis hex-bokeh or PixarKitty circular bokeh)
2. Motion blur (per-object using velocity buffer, configurable strength)
3. Lens distortion + chromatic aberration
4. Vignette + film grain
5. Color grading via 3D LUT (32×32×32 `RGBA8` 3D texture sampled in HCL)

### 26.3 — FinalCompositePass

Composites the lit scene + post chain + UI (when UI returns) and writes to the swapchain.

**Milestone:** Walking between two post-process volumes smoothly transitions colour grade and DoF settings. Motion blur streaks fast-moving objects correctly without smearing static ones.

---

## Phase 27 — Order-Independent Transparency ⬜ Planned

Handles overlapping translucent surfaces correctly.

### 27.1 — Weighted-Blended OIT

McGuire / Bavoil weighted-blended OIT. Two additional render targets: a `RGBA16F` accumulation and an `R8` reveal. The forward transparent shader writes weighted accumulation; a composite pass divides and blends onto SceneColor.

### 27.2 — Per-Material Opt-In

`BlendMode::Translucent` continues to use sorted alpha blend; `BlendMode::OIT` uses the weighted-blended path. Both coexist; OIT is for situations where overlap is visually important (smoke columns, glass shards).

**Milestone:** Two intersecting glass panes render without sort artefacts. Particle systems with `BlendMode::OIT` blend cleanly through one another.

---

## Phase 28 — CPU Frustum Culling ⬜ Planned

The cheapest culling phase and a large win.

### 28.1 — Bounding Volumes

Every imported mesh stores an axis-aligned bounding box and a bounding sphere computed at import time. `MeshDrawCommand` includes the world-space matrix; the cull pass derives the world-space bounding sphere by transforming the AABB centre and multiplying the radius by the largest scale component.

### 28.2 — Frustum Extraction

`Camera::GetFrustum()` returns six planes extracted from the view-projection matrix. The same procedure runs per shadow cascade using the tightened ortho matrix.

### 28.3 — Cull Service

`RenderQueue::Cull` walks the submitted draw list once per active frustum (main camera, each cascade). Surviving commands go into per-pass draw lists. SIMD-accelerated where available.

### 28.4 — Layer Mask Filtering

Each draw command and each pass carries a `LayerMask`. Passes ignore commands whose mask does not intersect their own. Used to exclude e.g. editor gizmo geometry (future) from shadow casts.

**Milestone:** A scene with 10,000 meshes, 200 visible, issues 200 draw calls. Cascade-specific culling cuts shadow draw counts further.

---

## Phase 29 — GPU-Driven Indirect + LOD + Hi-Z ⬜ Planned

Moves draw submission onto the GPU and integrates discrete LOD.

### 29.1 — Mesh LOD Asset

`MeshAsset` becomes a list of `SubMesh` LODs sorted highest-to-lowest detail, each with its own offsets and a screen-coverage threshold. The mesh cooker produces LODs via meshoptimizer's simplifier; artist-authored LODs override the auto-simplifier when present.

### 29.2 — Visibility Compute

A compute pass runs per camera over the full instance buffer:

1. Transform AABB
2. Frustum-test
3. Occlusion-test against the previous frame's Hi-Z
4. Select LOD by screen coverage
5. Write a compacted draw arg into the output indirect buffer
6. Atomically increment the count buffer

The geometry pass then issues a single `DrawIndexedIndirectCount` per material bucket. Draw count drops from CPU-bounded to GPU-bounded.

### 29.3 — Hi-Z Depth Pyramid

After the geometry pass, a compute chain downsamples the GBuffer depth into a mipmapped texture using max-of-four reduction. This pyramid feeds the next frame's visibility compute. SSR (Phase 22) also benefits from it once live.

### 29.4 — Two-Pass Occlusion

Behind a setting: rough cull → render → refine cull → render. Used for very dense scenes.

**Milestone:** A scene with 100,000 instances renders at smooth frame rate. LOD transitions are visually smooth. Removing the GPU cull restores CPU-bound draw counts.

---

## Phase 30 — Bindless Wire-Up ⬜ Planned

The `VulkanBindlessHeap` exists but is unused. This phase routes the production path through it.

### 30.1 — Texture Registration

Every `VulkanTexture` registers itself with the bindless heap at construction, receiving a `uint32_t BindlessIndex`. Unregisters on destruction (rate-limited deferred queue for safe GPU reuse). The heap capacity is 16384.

### 30.2 — Material Indirection

`Material` stores its texture references as bindless indices rather than `shared_ptr<Texture>`. The material parameter UBO holds the indices. Shaders sample via `texture(u_BindlessTextures[material.AlbedoIndex], uv)`.

### 30.3 — Mega-Draw

With bindless live, the indirect-draw path no longer needs to switch pipelines per material. A single mega-draw for the geometry pass becomes possible: every mesh, every material, one `DrawIndexedIndirectCount`. Pipeline switches collapse to one per blend mode + one per alpha-tested variant.

**Milestone:** A scene with 5,000 unique materials renders with one geometry pass draw call per blend mode. Texture hot-swap (replacing a material's albedo at runtime) updates the heap entry without recreating any descriptor set.

---

## Phase 31 — PSO Pre-Warm + Multi-Threaded Recording + Job System ⬜ Planned

Closes the remaining hot-path perf gaps.

### 31.1 — PSO Pre-Warm

On first run after a clean build, walk every shader permutation × pipeline state combination known to the engine and force compilation. After this pre-warm the user never pays a pipeline-compile hitch on this hardware.

### 31.2 — Job System

A small thread-pool job system under `Trinity/Threading/`. Standard work-stealing deque per worker, fiber-free, dependency-tracked via counters. Foundation for multi-threaded recording, asset import, physics, particle simulation.

### 31.3 — Multi-Threaded Recording

`SceneRenderer::Render` schedules per-pass recording onto job threads. Each pass records into a `VulkanCommandList` from a per-thread, per-frame command pool. The main thread waits for all per-pass jobs and submits the command lists in the graph's execution order. Cross-pass barriers are still injected by the graph; per-pass body execution is what parallelises.

### 31.4 — Per-Thread Allocators

Each worker thread has its own descriptor pool slice and staging-buffer ring slice. No mutex is taken in the per-pass recording path.

**Milestone:** A scene heavy in draw calls saturates multiple cores during record. Frame time drops in proportion to thread count up to the number of passes.

---

## Phase 32 — Streaming + Residency Budgets ⬜ Planned

For scenes larger than GPU memory.

### 32.1 — StreamingManager

Owns a request queue and a residency budget per resource class (textures, meshes, audio). Each frame: walk active scene objects, compute on-screen importance, request load when importance crosses a threshold, request unload when it falls below a lower threshold (hysteresis prevents flapping).

### 32.2 — Mip Streaming

Textures load with their lowest-resolution mips first via the upload queue. Higher mips stream in as the camera approaches. A residency tracker in the bindless heap lets shaders sampling a not-yet-resident mip fall back transparently to the next-coarsest available mip.

### 32.3 — Mesh LOD Streaming

Meshes load by LOD chain: lowest LOD loads with the asset, higher LODs stream in based on screen coverage. Combined with GPU LOD selection from Phase 29.

### 32.4 — Async Decompression

LZ4 / Zstandard compressed asset blobs decompress on job threads. Main thread is never blocked.

### 32.5 — Budget Enforcement

Over-budget evicts the lowest-importance mips first (textures) or highest-LOD data (meshes). Rate-limited eviction. Sustained over-budget reported as a stats warning.

**Milestone:** A test scene authored to exceed available VRAM by 4× streams correctly. Camera can fly around freely without stutters.

---

## Phase 33 — Hardware Ray-Traced Shadows ⬜ Planned

First phase that depends on `VK_KHR_ray_tracing_pipeline` / `acceleration_structure`. Falls back to CSM when unsupported.

### 33.1 — Acceleration Structure Builder

`VulkanAccelerationStructure` wraps `VkAccelerationStructureKHR`. Bottom-level AS per mesh built at import time, cached on the mesh asset. Top-level AS rebuilt each frame from instance transforms.

### 33.2 — Ray-Traced Shadow Pass

Inline ray queries from the lighting pass fragment shader (or a separate compute pass writing a shadow mask) cast a shadow ray per fragment toward the sun direction. Hit closest distance is unused; only a binary occlusion test. A small temporal denoiser smooths the binary mask using motion vectors.

### 33.3 — Quality Comparison

When ray-traced shadows are enabled, CSM still computes but only feeds far-cascade light leaking checks. Quality is no longer cascade-bound.

**Milestone:** On supported hardware (RTX-class), sun shadows are ray-traced. Visual quality dramatically improves at contact points and grazing angles. Falls back cleanly to CSM on unsupported hardware.

---

## Phase 34 — Real-Time Global Illumination (DDGI) ⬜ Planned

Priority-3 deliverable. Visibly bouncing indirect light in real time.

### 34.1 — Probe Grid

A 3D grid of irradiance probes covering the play area. Each probe stores irradiance + depth (with variance) in two octahedral 8×8 atlas pages. Default density: one probe per metre, configurable per scene.

### 34.2 — Per-Frame Update

Each frame, a subset of probes shoots N rays (e.g. 64) against the BVH. For each ray, the lighting pass shader is evaluated at the hit point (including direct light + previous frame's probe sampling for second bounce). Hit results update the probe's octahedral atlas via temporal accumulation.

### 34.3 — Sampling in Lighting Pass

The lighting pass samples surrounding probes (trilinear in space + visibility test using depth variance) and uses the result as the diffuse indirect term, replacing the IBL irradiance lookup (or blending with it for areas outside the probe grid).

### 34.4 — Glossy Indirect

For specular indirect, SSR (Phase 22) handles on-screen reflections; off-screen falls back to probe-sampled radiance via the same probe atlas but with a separate radiance page.

**Milestone:** A red wall lit by the sun produces visible red-tinted ambient on a nearby white wall. Indirect light updates in real time as objects move. Probe density is adjustable.

---

## Phase 35 — Mesh Shaders + Variable Rate Shading ⬜ Planned

Modern geometry path for hardware that supports it.

### 35.1 — Meshlet Asset

The mesh cooker produces meshlets (small clusters of ~64 triangles each) alongside the existing mesh data. Meshlets carry a bounding cone for back-face culling, a bounding sphere for frustum culling, and vertex/index ranges.

### 35.2 — Mesh Shader Geometry Pass

A `MeshShaderGeometryPass` is an alternative to the existing `GeometryPass`. Task shader culls meshlets per-cluster; mesh shader produces vertex output for surviving meshlets. Selection between paths is per-mesh based on triangle count.

### 35.3 — Variable Rate Shading

`VulkanRendererAPI::EnableVariableRateShading` exposes per-pass VRS attachments. A simple heuristic image is generated from the previous frame's TAA confidence: high-confidence regions shade at half rate. More aggressive heuristics behind settings.

**Milestone:** On supporting hardware, a triangle-dense scene renders faster through the mesh shader path than the traditional pipeline. VRS reduces shading cost in steady regions without visible artefacts.

---

## Phase 36 — HDR Display Output ⬜ Planned

### 36.1 — Swapchain HDR

`VulkanSwapchain::CreateSwapchain` queries for `VK_COLOR_SPACE_HDR10_ST2084_EXT` and `VK_COLOR_SPACE_BT2020_LINEAR_EXT` support via `VK_EXT_swapchain_colorspace`. When the user enables HDR output and the swapchain reports support, the swapchain is recreated with an HDR format (`A2B10G10R10_UNORM` or `R16G16B16A16_SFLOAT`) and an HDR colorspace.

### 36.2 — HDR-Aware Tonemap

`TonemapPass` branches between SDR (ACES + sRGB encode) and HDR (PQ encode to ST.2084 against the display's reported max luminance) output paths. Color grading LUTs are authored against a reference display luminance and remapped at runtime.

**Milestone:** On an HDR display, specular highlights show brighter than UI white. Switching between SDR and HDR output paths preserves the artist's grade on the parts of the image that fit within SDR range.

---

## Phase 37 — DirectX 12 Backend ⬜ Planned

The abstraction's purpose is validated only by this phase.

### 37.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Renderer/DirectX12/
|-- DirectX12RendererAPI.h/.cpp
|-- DirectX12Device.h/.cpp
|-- DirectX12Swapchain.h/.cpp
|-- DirectX12CommandList.h/.cpp
|-- DirectX12CommandQueue.h/.cpp
|-- DirectX12DescriptorAllocator.h/.cpp
|-- DirectX12PipelineCache.h/.cpp
|-- Resources/...
|-- RenderGraph/DirectX12RenderGraph.h/.cpp
|-- DirectX12Utilities.h/.cpp
```

### 37.2 — Mapping Strategy

All abstract types map cleanly onto D3D12:

- Render graph barriers translate to `D3D12_RESOURCE_BARRIER` via a state-mapping table
- Descriptor sets become `ID3D12DescriptorHeap` ranges referenced via root signatures
- Bindless heap maps natively to the D3D12 resource binding model
- `DrawIndexedIndirectCount` maps to `ExecuteIndirect` with a matching command signature
- Ray tracing maps to `ID3D12StateObject` and `DispatchRays`
- Mesh shaders map directly

### 37.3 — Backend Selection

`RendererSpecification::Backend` is honoured at startup. Both backends share 100% of the scene-side code.

### 37.4 — Validation

The full test scene from Phase 15 renders identically by both backends, verified by perceptual diff against reference images.

**Milestone:** Vulkan and DX12 render the same scene to within a small per-pixel epsilon.

---

## Phase 38 — Virtualised Geometry (Nanite-Class) ⬜ Planned

Priority-4 long-horizon item.

### 38.1 — Meshlet Hierarchy

Builds on Phase 35's meshlets. Each mesh is decomposed into a DAG of meshlet clusters with multiple LOD levels. Clusters share boundary vertices to avoid cracks; LOD selection is per-cluster based on screen-space error.

### 38.2 — Software Rasteriser

For sub-pixel-size clusters, a compute-based software rasteriser produces depth + visibility data faster than the hardware path. For larger clusters, the hardware mesh-shader path is used.

### 38.3 — Visibility Buffer

A 64-bit visibility buffer (per pixel: cluster id + triangle id) is written. Material shading happens in a deferred resolve pass that decodes the visibility buffer, fetches vertex attributes, interpolates, and runs the material shader.

### 38.4 — Streaming

Cluster data streams from disk based on screen-space importance. The streaming manager from Phase 32 is the foundation.

**Milestone:** A film-quality asset (10M+ triangles) renders at smooth frame rate with no visible LOD transitions. Streaming keeps memory bounded.

---

# PART III — Long-Term Engine Systems

The phases below depend on the renderer being substantially complete but do not block on every renderer phase. Animation in particular needs only basic geometry + mesh + material support and can be picked up earlier if desired.

## Phase 39 — Skeletal Animation ⬜ Planned

### 39.1 — Folder Structure

```
Trinity-Engine/src/Trinity/Animation/
|-- Skeleton.h/.cpp
|-- Bone.h
|-- AnimationClip.h/.cpp
|-- Animator.h/.cpp
|-- AnimatorComponent.h
```

### 39.2 — Skeleton + Bone

Walked from Assimp's `aiBone` array during mesh import when bones are present. Each `Bone` stores name, inverse bind-pose matrix, parent index. The `Skeleton` is a flat array.

A `SkinnedVertex` struct extends `Geometry::Vertex` with `BoneIndices` (ivec4) and `BoneWeights` (vec4). Unskinned meshes pay no cost.

### 39.3 — AnimationClip

Per-bone keyframe tracks for position, rotation, scale. Linear for position / scale, slerp for rotation. Sorted by time.

### 39.4 — Animator

`Animator` references a `Skeleton` and current `AnimationClip`. Each frame: advance time, sample all tracks, write bone-palette matrices to a GPU UBO. The geometry pass binds this UBO when an `AnimatorComponent` is present and selects the `SKINNED=1` permutation of the geometry shader.

### 39.5 — GPU Skinning (deferred to Phase ~39.5)

CPU skinning ships first. A compute-based skinning pre-pass that writes deformed positions to a transient buffer is the upgrade path; defers vertex deformation off the vertex shader for instanced characters.

**Milestone:** A skinned FBX loads, plays its embedded animation, deforms correctly.

---

## Phase 40 — 2D Physics (Box2D) ⬜ Planned

### 40.1 — Dependency

Box2D added as a git submodule under `Trinity-Engine/vendor/box2d`. Static-linked via `add_subdirectory`. No Box2D header appears outside `Trinity/Physics/2D/`.

### 40.2 — Abstract Interface

```cpp
class PhysicsAPI
{
public:
    virtual ~PhysicsAPI() = default;
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Step(float fixedDeltaTime) = 0;
    virtual void SetGravity(float x, float y) = 0;
};
```

### 40.3 — PhysicsWorld2D

Owns a `b2World`, stepped at fixed 1/60 s independent of render delta. Created by `Application` alongside the renderer, destroyed before renderer shutdown.

### 40.4 — Components

`Rigidbody2DComponent` (body type, gravity scale, fixed rotation, linear/angular damping). `BoxCollider2DComponent`, `CircleCollider2DComponent` (offset, size/radius, density, friction, restitution).

**Milestone:** Dynamic boxes fall and stack correctly under gravity at 60 Hz.

---

## Phase 41 — 3D Physics (PhysX 5) ⬜ Planned

### 41.1 — Dependency

PhysX 5 open-source added as a git submodule. Header isolation: no PhysX header outside `Trinity/Physics/3D/`.

### 41.2 — PhysicsWorld3D

Owns `PxScene`, stepped at fixed 1/60 s.

### 41.3 — Components

`Rigidbody3DComponent` (Static / Kinematic / Dynamic, mass, gravity scale, linear/angular damping). `BoxCollider3D`, `SphereCollider3D`, `CapsuleCollider3D`, `MeshCollider3D` (triangle mesh; cooked at asset import).

### 41.4 — Character Controller

`CharacterController3DComponent` wraps PhysX's character controller for player-class movement (deferred from base phase to a dedicated sub-phase).

**Milestone:** A dynamic sphere bounces on a static plane at 60 Hz with realistic restitution and friction.

---

## Phase 42 — Audio (OpenAL Soft) ⬜ Planned

### 42.1 — Dependency

OpenAL Soft as a git submodule. Header-isolated.

### 42.2 — AudioEngine

Owns the OpenAL context, listener, and per-bus volume controls. `Master`, `Music`, `SFX`, `UI`, `Voice` default buses.

### 42.3 — Components

`AudioSourceComponent` (clip handle, loop, gain, pitch, spatial-vs-2D, attenuation model). `AudioListenerComponent` (only one per scene; usually attached to the camera entity).

### 42.4 — Asset Pipeline

`.wav` files load directly. `.ogg` files decode via stb_vorbis. Long tracks support streaming (deferred to a later sub-phase).

**Milestone:** Two spatial sources play looping sounds that attenuate correctly with distance as the camera moves.

---

## Phase 43 — Scripting (Lua via sol2) ⬜ Planned

### 43.1 — Dependencies

sol2 + Lua as git submodules. No Lua or sol2 header in any public engine header (sol2 is used only inside `ScriptEngine.cpp` and the binding `.cpp` files).

### 43.2 — ScriptEngine

Owns a single `sol::state`, initialises standard libraries, registers all bindings, exposes a per-frame `Update(deltaTime)`.

### 43.3 — Bindings

Input, Transform, Rigidbody2D, Rigidbody3D, Audio, Scene queries (`FindEntityByTag`), Renderer queries. Contact callbacks registered via `Physics.OnContact(entity, fn)`.

### 43.4 — ScriptComponent

```cpp
struct ScriptComponent
{
    std::string ScriptPath;
    bool Active = true;
};
```

Scripts declare `OnStart`, `OnUpdate`, `OnStop`. Loaded once per entity at Play start.

### 43.5 — Hot-Reload

File-watcher polls every 200 ms. Changes reload the affected script at next frame begin. Play-mode only.

**Milestone:** A `ScriptComponent` drives an entity's movement under physics forces from Lua, reacts to input, plays an audio cue on collision, hot-reloads.

---

## Phase 44 — Build Pipeline ⬜ Planned

Custom asset cooker + packager that produces a standalone distributable game.

### 44.1 — Folder Structure

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
|   |-- ShaderCooker.h/.cpp
|   |-- MaterialCooker.h/.cpp
|-- Platform/
|   |-- WindowsBuildTarget.h/.cpp
|   |-- MacOSBuildTarget.h/.cpp
|   |-- XboxBuildTarget.h/.cpp
```

### 44.2 — Cookers

Each implements `bool Cook(metadata, outputPath, profile)`. Mesh cooker writes `.trimesh` (interleaved binary, includes LOD chain + meshlets). Texture cooker writes `.tritex` (BC-compressed, mip-included). Audio cooker writes `.triaudio`. Scene cooker writes `.triscene` (binary yaml, resolves all UUIDs to cooked paths). Shader cooker copies `.spv` files. Material cooker writes `.trimat` (binary parameters + texture bindless indices).

### 44.3 — Pipeline Stages

Validate → Cook → Build manifest → Package → Report. Runs on a worker thread with progress callbacks. Output is a flat folder containing the runtime executable, cooked assets, and `AssetManifest.bin`.

### 44.4 — Runtime Loader

In a cooked build, `AssetRegistry` is replaced with a manifest-driven loader. No Assimp, no stb_image, no yaml-cpp at runtime — only binary cooked-format readers.

**Milestone:** A scene authored in development mode cooks to a flat folder. The folder runs as a standalone game with no engine source dependencies.

---

# PART IV — Deliberate Gaps

Items deliberately omitted from numbered phases. Listed here so they are not forgotten.

## Renderer Gaps

| Gap | Insertion Point | Dependency |
|---|---|---|
| GPU-resident animation skinning compute | Pre-geometry compute pass | Phase 39 animation complete |
| Subsurface scattering (separable SSSS) | Lighting pass extension | Phase 15 lighting complete |
| Hair / fur rendering (TressFX-style) | Forward pass after deferred resolve | Phase 19 forward transparency complete |
| Eye shading | Material variant inside lighting pass | Phase 15 lighting complete |
| Cloth simulation rendering | New forward pass | Phase 41 3D physics complete |
| Multi-view / VR | `RendererAPI` extension | Long-term, needs VR layer integration |
| Texture virtual / sparse tiled streaming | `StreamingManager` extension | Phase 32 streaming complete |
| Per-pass async upload coalescing | `VulkanUploadQueue` extension | Already partially in upload queue |
| Reflection probes (cubemap captures) | Lighting pass extension | Phase 15 IBL complete |
| Planar reflections | Pre-geometry pass | Phase 12 RenderPass arch complete |
| Volumetric clouds | Pre-tonemap pass | Phase 23 fog complete |
| Atmosphere model (Bruneton / Hillaire) | Skybox replacement | Phase 15 skybox complete |
| Lens flare | Post chain | Phase 16 HDR complete |
| Screen-space contact shadows | Lighting pass extension | Phase 15 lighting complete |
| Stochastic transparency | OIT variant | Phase 27 OIT complete |
| Decal mesh / projected text | DecalPass extension | Phase 24 decals complete |
| Cascaded volumetric fog (cascaded froxel grids) | Volumetric fog extension | Phase 23 fog complete |
| Hardware-accelerated reflections (RT) | RT phase extension | Phase 33 RT complete |

## ECS / Scene Gaps

| Gap | Location | Dependency |
|---|---|---|
| Prefab system with overrides | `Prefab.h/.cpp` | Independent |
| Scene streaming / additive loading | `Scene.h/.cpp` | Phase 32 streaming complete |
| Per-entity static / dynamic flags | `Scene` + renderer | Phase 13 velocity attachment complete |
| Entity lifetime / spawn-despawn events | `Scene` | Independent |
| Tag-based query helpers | `Scene` | Independent |

## Physics Gaps

| Gap | Location | Dependency |
|---|---|---|
| Triangle-mesh 3D collider | `PhysicsWorld3D` | PhysX cooking pipeline |
| Character controller 3D | `CharacterController3D.h/.cpp` | Phase 41 base complete |
| Joints (hinge / fixed / spring) — 2D | `PhysicsWorld2D` | Phase 40 base complete |
| Joints (hinge / fixed / spring) — 3D | `PhysicsWorld3D` | Phase 41 base complete |
| Physics material asset | `AssetRegistry` | Independent |
| Continuous collision detection | `Rigidbody2DComponent` | Phase 40 base complete |
| Trigger / overlap queries | both physics worlds | Both physics phases complete |

## Audio Gaps

| Gap | Location | Dependency |
|---|---|---|
| Reverb / environmental effects | `OpenALAudioEngine` | Phase 42 base complete |
| Audio clip streaming | `AudioClip` | Phase 42 base complete |
| HRTF binaural | `OpenALAudioEngine` | Phase 42 base complete |
| Audio snapshot / preset | `AudioMixer` | Phase 42 base complete |

## Scripting Gaps

| Gap | Location | Dependency |
|---|---|---|
| Per-script sandboxed environments | `ScriptEngine` | Phase 43 base complete |
| Lua coroutine scheduler | `ScriptEngine` | Phase 43 base complete |
| Visual scripting graph | New system | Phase 43 complete + UI strategy decided |
| C# scripting alternative | New phase | Long-term |

## Build Pipeline Gaps

| Gap | Location | Dependency |
|---|---|---|
| macOS build target implementation | `MacOSBuildTarget` | macOS platform active |
| Xbox GDK build target implementation | `XboxBuildTarget` | Xbox platform active |
| Incremental cook | `AssetCooker` | Phase 44 base complete |
| Code signing for Windows builds | `WindowsBuildTarget` | Phase 44 base complete |
| Installer generation | `WindowsBuildTarget` | Deferred — flat folder sufficient initially |

---

# PART V — Future Editor Path (Paused)

The ImGui-based editor (`Trinity-Forge` with panels, gizmos, dockspace, viewport-to-texture) is paused. The engine runs as a renderer-only target. When the editor is revived, the path below replaces and modernises the original ImGui plan.

## Editor Phase E1 — UI Strategy Decision ⬜ Pending

Pick one path before any UI work resumes:

| Option | Pros | Cons |
|---|---|---|
| **ImGui (revival)** | Fast iteration, dockspace, well-known | Immediate-mode redraws, not GPU-driven, no styling polish |
| **Custom retained-mode UI** | Full control over look, GPU-driven, themable | Months of foundational work |
| **Slate-style declarative UI** | Modern reactive pattern, clean separation | Steep design surface |
| **CEF (Chromium-embedded) HTML/CSS UI** | Designer-friendly, leverages web tooling | Heavy runtime, complex integration |

Decision blocked until a real game project surfaces requirements.

## Editor Phase E2 — Panel System ⬜ Pending

When UI returns: panel registration, dockspace, viewport-to-texture (using SceneRenderer's existing `SetRenderTarget`), inspector with reflection-driven component editing.

## Editor Phase E3 — Selection + Gizmo + Play/Pause/Stop ⬜ Pending

A new gizmo system (replacing ImGuizmo) with explicit delta-rotation tracking (avoiding the Euler decomposition instability noted in the prior cycle). Play / Pause / Stop state machine with scene serialise-to-memory before play and deserialise on stop.

## Editor Phase E4 — Profiler + RenderGraph Visualiser + Stats ⬜ Pending

Consumes the GPU timing infrastructure already present in Phase 10. RenderGraph visualisation reads `SceneRenderer::DumpRenderGraphDot()`.

---

# Master Execution Summary

| Phase | Area | Deliverable | Status |
|---|---|---|---|
| 1 | Foundation | Repo + CMake + shader build | ✅ |
| 2 | Renderer | Abstract renderer interface | ✅ |
| 3 | Renderer | Vulkan backend skeleton | 🟡 (closed in 9) |
| 4 | Renderer | Render graph | ✅ |
| 4.5 | Renderer | CommandList abstraction | ✅ |
| 5 | Renderer | Vulkan resources | ✅ |
| 6 | Renderer | Material skeleton | 🟡 (closed in 11) |
| 7 | Renderer | Geometry pass skeleton | 🟡 (closed in 9 + 13) |
| 8 | Foundation | Application integration | ✅ |
| F-Scene | Foundation | ECS / Asset / Platform | ✅ |
| 9 | Renderer | Device features + GeometryPass hot-fixes | ⬜ |
| 10 | Renderer | Frame UBO + Reverse-Z + GPU timing + debug labels | ⬜ |
| 11 | Renderer | Real material system | ⬜ |
| 12 | Renderer | RenderPass class architecture | ⬜ |
| 13 | Renderer | GBuffer expansion (6 attachments) | ⬜ |
| 14 | Renderer | Cascaded shadow maps | ⬜ |
| 15 | Renderer | PBR deferred lighting + skybox + IBL | ⬜ |
| 16 | Renderer | HDR pipeline (bloom, exposure, ACES) | ⬜ |
| 17 | Renderer | GTAO | ⬜ |
| 18 | Renderer | Punctual lights + clustered culling | ⬜ |
| 19 | Renderer | Forward transparency | ⬜ |
| 20 | Renderer | Shader build (permutations + reflection + hot-reload) | ⬜ |
| 21 | Renderer | TAA + velocity | ⬜ |
| 22 | Renderer | Screen-space reflections | ⬜ |
| 23 | Renderer | Volumetric fog | ⬜ |
| 24 | Renderer | Decals | ⬜ |
| 25 | Renderer | Particle system | ⬜ |
| 26 | Renderer | Color grading + DoF + motion blur | ⬜ |
| 27 | Renderer | OIT | ⬜ |
| 28 | Renderer | CPU frustum culling | ⬜ |
| 29 | Renderer | GPU-driven indirect + LOD + Hi-Z | ⬜ |
| 30 | Renderer | Bindless wire-up | ⬜ |
| 31 | Renderer | PSO pre-warm + multi-thread record + job system | ⬜ |
| 32 | Renderer | Streaming + residency budgets | ⬜ |
| 33 | Renderer | Hardware ray-traced shadows | ⬜ |
| 34 | Renderer | Real-time GI (DDGI) | ⬜ |
| 35 | Renderer | Mesh shaders + VRS | ⬜ |
| 36 | Renderer | HDR display output | ⬜ |
| 37 | Renderer | DirectX 12 backend | ⬜ |
| 38 | Renderer | Virtualised geometry (Nanite-class) | ⬜ |
| 39 | Engine | Skeletal animation | ⬜ |
| 40 | Engine | 2D physics (Box2D) | ⬜ |
| 41 | Engine | 3D physics (PhysX 5) | ⬜ |
| 42 | Engine | Audio (OpenAL Soft) | ⬜ |
| 43 | Engine | Scripting (Lua via sol2) | ⬜ |
| 44 | Engine | Build pipeline + cooker | ⬜ |
| E1–E4 | Editor | Future editor path | Paused |

---

## Notes on Execution Order

Phases 9–18 are the priority-1 deliverables (PBR + lighting + shadows) and must land in order; each depends on the previous.

Phases 21–27 are priority-2 (post-processing) and can be reordered slightly based on what visual problems are most pressing for the first game project. TAA (21) should land before SSR (22) because SSR benefits from temporal denoising.

Phases 28–32 are perf-and-scale items; they can land before any post-processing phase if the renderer becomes draw-call-bound.

Phases 33–34 are GI (priority 3) and depend on the lighting pass (15) being stable.

Phase 38 (Nanite-class) is priority 4 and a long-horizon item — likely 6+ months once started.

Phases 39–44 (animation, physics, audio, scripting, build) can be interleaved with renderer phases once a real game project needs them. Animation in particular only needs Phases 9–15 to be useful.
