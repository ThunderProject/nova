module;
#include "logger.h"
#include "result.h"
#include <NRI.h>
#include <Extensions/NRISwapChain.h>
#include <Extensions/NRIDeviceCreation.h>
#include <NRIDescs.h>
#include <array>
#include <assert.hpp>
#include <magic_enum/magic_enum.hpp>
#include <ranges>
#include <algorithm>
export module nova.render.rhi.device;

import nova.render.rhi.core;

namespace nova::render::rhi {
    [[nodiscard]] std::uint64_t adapter_score(const nri::AdapterDesc& adapter) noexcept {
        auto score = adapter.videoMemorySize;

        if(adapter.architecture == nri::Architecture::DISCRETE) {
            score += std::uint64_t{1} << 62;
        }

        return score;
    }

    [[nodiscard]] nova::result<nri::AdapterDesc> select_adapter() {
    std::uint32_t adapter_count = 0;

    auto res = check(nri::nriEnumerateAdapters(nullptr, adapter_count));
    if(!res) {
        return nova::err(std::move(res.error()));
    }

    if(adapter_count == 0) [[unlikely]] {
        return nova::err(std::string("No graphics adapter found"));
    }

    std::vector<nri::AdapterDesc> adapters(adapter_count);

    res = check(nri::nriEnumerateAdapters(adapters.data(), adapter_count));
    if(!res) {
        return nova::err(std::move(res.error()));
    }

    auto candidates = adapters | std::views::filter(
        [](const nri::AdapterDesc& adapter) {
            const auto supports_vulkan = 
            (adapter.supportedGraphicsAPIs & nri::GraphicsAPI::VK) != static_cast<uint8_t>(nri::GraphicsAPI::NONE);
            const auto has_graphics_queue = adapter.queueNum[static_cast<std::size_t>(nri::QueueType::GRAPHICS)] != 0;

            return supports_vulkan && has_graphics_queue;
        }
    );

    const auto selected = std::ranges::max_element(candidates, {}, [](const auto& adapter) { return adapter_score(adapter); });

    if(selected == candidates.end()) [[unlikely]] {
        return nova::err("No vulkan capable graphics adapter found");
    }

    return *selected;
    }

    export class device {
    public:
        device(const device&) = delete;
        device& operator=(const device&) = delete;

        device(device&& other) noexcept
            :
            m_device(std::exchange(other.m_device, nullptr)),
            m_core(other.m_core),
            m_swapchain(other.m_swapchain),
            m_graphics_queue(std::exchange(other.m_graphics_queue, nullptr)),
            m_compute_queue(std::exchange(other.m_compute_queue, nullptr)),
            m_copy_queue(std::exchange(other.m_copy_queue, nullptr)) 
        {}

        device& operator=(device&& other) noexcept {
            if(this == &other) {
                return *this;
            }

            destroy();

            m_device = std::exchange(other.m_device, nullptr);
            m_core = other.m_core;
            m_swapchain = other.m_swapchain;
            m_graphics_queue = std::exchange(other.m_graphics_queue, nullptr);
            m_compute_queue = std::exchange(other.m_compute_queue, nullptr);
            m_copy_queue = std::exchange(other.m_copy_queue, nullptr);

            return *this;
        }

        ~device() noexcept {
            destroy();
        }

        [[nodiscard]] static nova::result<device> create() {
            auto adapter = select_adapter();
            if(!adapter) {
                return nova::err(std::move(adapter.error()));
            }

            device inst;
            auto res = inst.init(*adapter);
            if(!res) {
                return nova::err(std::move(res.error()));
            }
            return inst;
        }

        [[nodiscard]] nri::Device& native() noexcept {
            DEBUG_ASSERT(m_device != nullptr);
            return *m_device;
        }

        [[nodiscard]] const nri::Device& native() const noexcept {
            DEBUG_ASSERT(m_device != nullptr);
            return *m_device;
        }

        [[nodiscard]] nri::CoreInterface& core() noexcept {
            return m_core;
        }

        [[nodiscard]] const nri::CoreInterface& core() const noexcept {
            return m_core;
        }

        [[nodiscard]] nri::SwapChainInterface& swapchain_interface() noexcept {
            return m_swapchain;
        }

        [[nodiscard]] const nri::SwapChainInterface& swapchain_interface() const noexcept {
            return m_swapchain;
        }

        [[nodiscard]] nri::Queue& graphics_queue() noexcept {
            DEBUG_ASSERT(m_graphics_queue != nullptr);
            return *m_graphics_queue;
        }

        [[nodiscard]] nri::Queue* compute_queue() noexcept {
            return m_compute_queue;
        }

        [[nodiscard]] nri::Queue* copy_queue() noexcept {
            return m_copy_queue;
        }

        [[nodiscard]] bool has_compute_queue() const noexcept {
            return m_compute_queue != nullptr;
        }

        [[nodiscard]] bool has_copy_queue() const noexcept {
            return m_copy_queue != nullptr;
        }

        [[nodiscard]] const nri::DeviceDesc& description() const noexcept {
            DEBUG_ASSERT(m_device != nullptr);
            return m_core.GetDeviceDesc(*m_device);
        }

        [[nodiscard]] nova::result<nova::ok> wait_idle() {
            DEBUG_ASSERT(m_device != nullptr);
            return check(m_core.DeviceWaitIdle(m_device));
        }
    private:
        device() noexcept = default;

        [[nodiscard]] nova::result<nova::ok> init(const nri::AdapterDesc& adapter) {
            nova::logger::info("Render adapter selected: {}", adapter.name);

            std::array<nri::QueueFamilyDesc, 3> queue_families{};
            std::array<float, 3> priorities{1.0f, 0.75f, 0.5f};

            std::uint32_t queue_family_count = 0;

            queue_families[queue_family_count++] = {
                .queuePriorities = priorities.data(),
                .queueNum = 1,
                .queueType = nri::QueueType::GRAPHICS
            };

            if(adapter.queueNum[static_cast<std::size_t>(nri::QueueType::COMPUTE)] != 0) {
                queue_families[queue_family_count++] = {
                    .queuePriorities = &priorities[1],
                    .queueNum = 1,
                    .queueType = nri::QueueType::COMPUTE
                };
            }

            if(adapter.queueNum[static_cast<std::size_t>(nri::QueueType::COPY)] != 0) {
                queue_families[queue_family_count++] = {
                    .queuePriorities = &priorities[2],
                    .queueNum = 1,
                    .queueType = nri::QueueType::COPY
                };
            }

            nri::DeviceCreationDesc desc{};
            desc.graphicsAPI = nri::GraphicsAPI::VK;
            desc.robustness = nri::Robustness::VK;
            desc.adapterDesc = &adapter;
            desc.queueFamilies = queue_families.data();
            desc.queueFamilyNum = queue_family_count;
#ifndef NDEBUG
            desc.enableNRIValidation = true;
            desc.enableGraphicsAPIValidation = true;
#endif
             auto res = check(nri::nriCreateDevice(desc, m_device));
             if(!res) {
                 return nova::err(std::move(res.error()));
             }

            if(auto result = load_interfaces(); !result) {
                destroy();
                return nova::err(std::move(result.error()));
            }

            if(auto result = acquire_queues(); !result) {
                destroy();
                return nova::err(std::move(result.error()));
            }

            const auto& device_desc = description();

            nova::logger::info(
                "RHI device initialized: {} MiB VRAM",
                device_desc.adapterDesc.videoMemorySize / (1024ULL * 1024ULL)
            );

            nova::logger::info(
                "RHI queues: graphics=yes compute={} copy={}",
                has_compute_queue(),
                has_copy_queue()
            );

            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> load_interfaces() {
            DEBUG_ASSERT(m_device != nullptr);

            auto res = check(nri::nriGetInterface(*m_device, "CoreInterface", sizeof(nri::CoreInterface), &m_core));
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            res = check(nri::nriGetInterface(*m_device, "SwapChainInterface", sizeof(nri::SwapChainInterface), &m_swapchain));
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> acquire_queues() {
            DEBUG_ASSERT(m_core != nullptr);

            auto res = check(m_core.GetQueue(*m_device, nri::QueueType::GRAPHICS, 0, m_graphics_queue));
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            const auto& desc = description();
            if(desc.adapterDesc.queueNum[static_cast<std::size_t>(nri::QueueType::COMPUTE)] != 0) {
                res = check(m_core.GetQueue(*m_device, nri::QueueType::COMPUTE, 0, m_compute_queue));
                if(!res) {
                    return nova::err(std::move(res.error()));
                }
            }

            if(desc.adapterDesc.queueNum[static_cast<std::size_t>(nri::QueueType::COPY)] != 0) {
                res = check(m_core.GetQueue(*m_device, nri::QueueType::COPY, 0, m_copy_queue));
                if(!res) {
                    return nova::err(std::move(res.error()));
                }
            }

            return nova::ok{};
        }

        void destroy() noexcept {
            if(m_device == nullptr) {
                return;
            }
            if(m_core.DeviceWaitIdle != nullptr) {
                auto _ = m_core.DeviceWaitIdle(m_device);
            }

            nri::nriDestroyDevice(m_device);
            m_device = nullptr;
            m_graphics_queue = nullptr;
            m_compute_queue = nullptr;
            m_copy_queue = nullptr;
        }

        nri::Device* m_device{nullptr};
        nri::CoreInterface m_core{};
        nri::SwapChainInterface m_swapchain{};

        nri::Queue* m_graphics_queue{nullptr};
        nri::Queue* m_compute_queue{nullptr};
        nri::Queue* m_copy_queue{nullptr};
    };
}
