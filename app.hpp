#pragma once
#include "lve_window.hpp"
#include "lve_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_swap_chain.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace lve {
    class App {
        public:
            static constexpr int WIDTH = 1600;
            static constexpr int HEIGHT = 900;

            App();
            ~App();
            App(const App &) = delete;
            App &operator = (const App &) = delete;

            void run();
        private:
            void createPipelineLayout();
            void createPipeline();
            void createCommandBuffers();
            void drawFrame();

            LveWindow lveWindow{WIDTH, HEIGHT, "Vulkan Test!"};
            LveDevice lveDevice{lveWindow};
            LveSwapChain lveSwapChain{lveDevice, lveWindow.getExtent()};
            std::unique_ptr<LvePipeline> lvePipeline;

            VkPipelineLayout pipelineLayout;
            std::vector<VkCommandBuffer> commandBuffers;
    };
}