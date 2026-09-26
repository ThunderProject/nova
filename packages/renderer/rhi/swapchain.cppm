module;

#include "result.h"

#include <NRI.h>
#include <Extensions/NRISwapChain.h>
#include <NRIDescs.h>
#include <assert.hpp>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

export module nova.render.rhi.swapchain;

import nova.platform.window;
import nova.render.rhi.core;
import nova.render.rhi.device;

export namespace nova::render::rhi {
    struct swapchain_desc {
        std::uint8_t texture_count{3};
        std::uint8_t queued_frame_count{2};
        nri::SwapChainFormat format{nri::SwapChainFormat::BT709_G22_8BIT};
        nri::SwapChainBits flags{
            nri::SwapChainBits::ALLOW_TEARING |
            nri::SwapChainBits::WAITABLE |
            nri::SwapChainBits::ALLOW_LOW_LATENCY
        };
    };

    struct acquired_image {
        nri::Texture* texture{nullptr};
        nri::Fence* acquire_semaphore{nullptr};
        nri::Fence* release_semaphore{nullptr};
        std::uint32_t index{0};
    };

    class swapchain final {
    public:
         swapchain(const swapchain&) = delete;
        swapchain& operator=(const swapchain&) = delete;

        swapchain(swapchain&& other) noexcept
            :
            m_device(std::exchange(other.m_device, nullptr)),
            m_swapchain(std::exchange(other.m_swapchain, nullptr)),
            m_textures(std::move(other.m_textures)),
            m_sync(std::move(other.m_sync)),
            m_extent(other.m_extent),
            m_acquire_index(std::exchange(other.m_acquire_index, 0))
        {}

        swapchain& operator=(swapchain&& other) noexcept {
            if(this == &other) {
                return *this;
            }

            destroy();

            m_device = std::exchange(other.m_device, nullptr);
            m_swapchain = std::exchange(other.m_swapchain, nullptr);
            m_textures = std::move(other.m_textures);
            m_sync = std::move(other.m_sync);
            m_extent = other.m_extent;
            m_acquire_index = std::exchange(other.m_acquire_index, 0);

            return *this;
        }

        ~swapchain() noexcept {
            destroy();
        }

        [[nodiscard]] static nova::result<swapchain> create(
            device& device,
            const platform::presentation_handle presentation,
            const platform::extent2d extent,
            const swapchain_desc& config = {}
        ) {
            if(extent.empty()) [[unlikely]] {
                return nova::err(std::string{"Cannot create swapchain with zero extent"});
            }

            if(config.texture_count == 0) [[unlikely]] {
                return nova::err(std::string{"Swapchain texture count must be greater than zero"});
            }

            swapchain result(device, extent);

            nri::SwapChainDesc desc{};

            desc.window.wayland.display = presentation.display;
            desc.window.wayland.surface = presentation.surface;
            desc.queue = &device.graphics_queue();
            desc.width = static_cast<nri::Dim_t>(extent.width);
            desc.height = static_cast<nri::Dim_t>(extent.height);
            desc.textureNum = config.texture_count;
            desc.format = config.format;
            desc.flags = config.flags;
            desc.queuedFrameNum = config.queued_frame_count;

            auto res = check(device.swapchain_interface().CreateSwapChain(device.native(), desc, result.m_swapchain));
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            std::uint32_t texture_count = 0;

            const auto* textures = device.swapchain_interface().GetSwapChainTextures(*result.m_swapchain, texture_count);

            if(textures == nullptr || texture_count == 0) [[unlikely]] {
                return nova::err(std::string{"swapchain returned no textures"});
            }

            result.m_textures.assign(textures, textures + texture_count);
            result.m_sync.resize(texture_count);

            for(auto& sync : result.m_sync) {
                res = check(device.core().CreateFence(device.native(), nri::SWAPCHAIN_SEMAPHORE, sync.acquire));
                if(!res) {
                    return nova::err(std::move(res.error()));
                }

                res = check(device.core().CreateFence(device.native(), nri::SWAPCHAIN_SEMAPHORE, sync.release));
                if(!res) {
                    return nova::err(std::move(res.error()));
                }
            }

            return result;
        }

        [[nodiscard]] nova::result<acquired_image, nri::Result> acquire() {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_swapchain != nullptr);
            DEBUG_ASSERT(!m_textures.empty());
            DEBUG_ASSERT(m_textures.size() == m_sync.size());

            auto& acquire_sync = m_sync[m_acquire_index];

            std::uint32_t texture_index = 0;

            const auto res = m_device->swapchain_interface().AcquireNextTexture(
                *m_swapchain,
                *acquire_sync.acquire,
                texture_index
            );

            if(res != nri::Result::SUCCESS) [[unlikely]] {
                return nova::err(res);
            }

            DEBUG_ASSERT(texture_index < m_textures.size());
            DEBUG_ASSERT(texture_index < m_sync.size());

            auto& image_sync = m_sync[texture_index];

            acquired_image image{
                .texture = m_textures[texture_index],
                .acquire_semaphore = acquire_sync.acquire,
                .release_semaphore = image_sync.release,
                .index = texture_index
            };

            m_acquire_index = (m_acquire_index + 1) % m_sync.size();

            return image;
        }

        [[nodiscard]] nova::result<nova::ok, nri::Result> present(const acquired_image& image) {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_swapchain != nullptr);
            DEBUG_ASSERT(image.release_semaphore != nullptr);
            DEBUG_ASSERT(image.index < m_textures.size());
            DEBUG_ASSERT(image.texture == m_textures[image.index]);

            const auto res = m_device->swapchain_interface().QueuePresent(*m_swapchain, *image.release_semaphore);

            if(res != nri::Result::SUCCESS) [[unlikely]] {
                return nova::err(res);
            }

            return nova::ok{};
        }

        [[nodiscard]] nri::SwapChain& native() noexcept {
            DEBUG_ASSERT(m_swapchain != nullptr);
            return *m_swapchain;
        }

        [[nodiscard]] const nri::SwapChain& native() const noexcept {
            DEBUG_ASSERT(m_swapchain != nullptr);
            return *m_swapchain;
        }

        [[nodiscard]] nri::Texture& texture(const std::size_t index) noexcept {
            DEBUG_ASSERT(index < m_textures.size());
            return *m_textures[index];
        }

        [[nodiscard]] const nri::Texture& texture(const std::size_t index) const noexcept {
            DEBUG_ASSERT(index < m_textures.size());
            return *m_textures[index];
        }

        [[nodiscard]] std::size_t texture_count() const noexcept {
            return m_textures.size();
        }

        [[nodiscard]] platform::extent2d extent() const noexcept {
            return m_extent;
        }

        [[nodiscard]] nri::Format format() const noexcept {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(!m_textures.empty());

            return m_device->core().GetTextureDesc(*m_textures.front()).format;
        }
    private:
        struct image_sync {
            nri::Fence* acquire{nullptr};
            nri::Fence* release{nullptr};
        };

        swapchain(device& device, const platform::extent2d extent) noexcept
            :
            m_device(&device),
            m_extent(extent)
        {}

        void destroy() noexcept {
            if(m_device == nullptr) {
                return;
            }

            for(auto& sync : m_sync) {
                if(sync.acquire != nullptr) {
                    m_device->core().DestroyFence(sync.acquire);
                    sync.acquire = nullptr;
                }

                if(sync.release != nullptr) {
                    m_device->core().DestroyFence(sync.release);
                    sync.release = nullptr;
                }
            }

            m_sync.clear();
            m_textures.clear();

            if(m_swapchain != nullptr) {
                m_device->swapchain_interface().DestroySwapChain(m_swapchain);
                m_swapchain = nullptr;
            }

            m_device = nullptr;
            m_acquire_index = 0;
        }

        device* m_device{nullptr};
        nri::SwapChain* m_swapchain{nullptr};

        std::vector<nri::Texture*> m_textures;
        std::vector<image_sync> m_sync;

        platform::extent2d m_extent{};
        std::size_t m_acquire_index{0};
    };
}
