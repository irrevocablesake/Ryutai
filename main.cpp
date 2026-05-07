#define VOLK_IMPLEMENTATION
#include<volk/volk.h>

#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/quaternion.hpp>

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include<iostream>
#include<vector>
#include<array>

class Renderer {

	public:
		//validation functions
		void validateResult( VkResult result, std::string message = "ERROR!") {
			if (result != VK_SUCCESS) {
				std::cerr << "ERROR: " << message << std::endl;
				exit(result);
			}
		}

		void validateResult(bool result, std::string message = "ERROR!") {
			if (!result) {
				std::cerr << "ERROR " << message << std::endl;
				exit(result);
			}
		}

		void validateSwapchain( VkResult result ) {
			if (result < VK_SUCCESS ) {
				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					deviceIF.logical.swapchainConfiguration.updateSwapchain = true;
					return;
				}

				std::cerr << "ERROR: Swapchain Validation Failed" << std::endl;
				exit(result);
			}
		}

		//vulkan functions
		void setupLibraries() {
			validateResult(SDL_Init( SDL_INIT_VIDEO ));
			validateResult(SDL_Vulkan_LoadLibrary(NULL));
			volkInitialize();
		}

		void setupInstance() {
			VkApplicationInfo applicationInfo{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Rytutai",
				.apiVersion = VK_API_VERSION_1_4
			};

			extensionsIF.instance.extensions = SDL_Vulkan_GetInstanceExtensions( &extensionsIF.instance.count );

			VkInstanceCreateInfo instanceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &applicationInfo,
				.enabledExtensionCount = extensionsIF.instance.count,
				.ppEnabledExtensionNames = extensionsIF.instance.extensions
			};

			validateResult( vkCreateInstance(&instanceCreateInfo, nullptr, &windowIF.vkInstance ));
			volkLoadInstance(windowIF.vkInstance);
		}

		void pickPhysicalDeviceAndQueueIndex() {
			validateResult(vkEnumeratePhysicalDevices( windowIF.vkInstance, &deviceIF.physical.count, nullptr ));

			deviceIF.physical.devices.resize(deviceIF.physical.count);
			deviceIF.queue.properties.resize(deviceIF.physical.count);

			validateResult( vkEnumeratePhysicalDevices( windowIF.vkInstance, &deviceIF.physical.count, deviceIF.physical.devices.data() ) );

			for (uint32_t i = 0; i < deviceIF.physical.count; i++) {
				VkPhysicalDeviceProperties2 properties{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
				};

				vkGetPhysicalDeviceProperties2(deviceIF.physical.devices[i], &properties);
				deviceIF.physical.properties.push_back(properties);

				uint32_t queueFamiliesCount;
				vkGetPhysicalDeviceQueueFamilyProperties2(deviceIF.physical.devices[i], &queueFamiliesCount, nullptr);
				deviceIF.queue.properties[i].resize(queueFamiliesCount);

				for (uint32_t j = 0; j < queueFamiliesCount; j++) {
					deviceIF.queue.properties[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
				}

				vkGetPhysicalDeviceQueueFamilyProperties2(deviceIF.physical.devices[i], &queueFamiliesCount, deviceIF.queue.properties[i].data());
			}

			for (uint32_t i = 0; i < deviceIF.physical.count; i++) {

				bool found = false;
				for (uint32_t j = 0; j < deviceIF.queue.properties[i].size(); j++) {
					
					bool cond1 = deviceIF.physical.properties[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
					bool cond2 = deviceIF.queue.properties[i][j].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
					if (cond1 && cond2 ) {
						deviceIF.physical.index = i;
						deviceIF.physical.device = deviceIF.physical.devices[i];

						deviceIF.queue.index = j;

						found = true;
						break;
					}
				}
				
				if (found) {
					break;
				}
			}

			validateResult( SDL_Vulkan_GetPresentationSupport( windowIF.vkInstance, deviceIF.physical.device, deviceIF.queue.index ));
		}

		void setupLogicalDevice() {

			float priorities = 1.0;

			VkDeviceQueueCreateInfo queueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = deviceIF.queue.index,
				.queueCount = 1,
				.pQueuePriorities = &priorities
			};

			VkPhysicalDeviceVulkan13Features enabledVk13Features{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
				.synchronization2 = VK_TRUE,
				.dynamicRendering = VK_TRUE
			};

			VkDeviceCreateInfo deviceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = &enabledVk13Features,
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &queueCreateInfo,
				.enabledExtensionCount = extensionsIF.device.extensions.size(),
				.ppEnabledExtensionNames = extensionsIF.device.extensions.data()
			};

			validateResult(vkCreateDevice(deviceIF.physical.device, &deviceCreateInfo, nullptr, &deviceIF.logical.device));
			
			volkLoadDevice(deviceIF.logical.device);
			vkGetDeviceQueue(deviceIF.logical.device, deviceIF.queue.index, 0, &deviceIF.queue.queue );
		}

		void setupVMA() {
			VmaVulkanFunctions vkFunctions{
				.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
				.vkGetDeviceProcAddr = vkGetDeviceProcAddr
			};

			VmaAllocatorCreateInfo allocatorCreateInfo{
				.physicalDevice = deviceIF.physical.device,
				.device = deviceIF.logical.device,
				.pVulkanFunctions = &vkFunctions,
				.instance = windowIF.vkInstance
			};

			validateResult( vmaCreateAllocator( &allocatorCreateInfo, &vmaAllocator ) );
		}
		
		void setupWindowAndSurface(){
			windowIF.window = SDL_CreateWindow(
				windowIF.windowName.c_str(),
				windowIF.windowWidth,
				windowIF.windowHeight,
				windowIF.windowFlags
			);

			validateResult( SDL_GetWindowSize( windowIF.window, &windowIF.dimensions.x, &windowIF.dimensions.y  ) );

			validateResult( SDL_Vulkan_CreateSurface( windowIF.window, windowIF.vkInstance, nullptr, &windowIF.surface )  );
			validateResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( deviceIF.physical.device, windowIF.surface, &windowIF.surfaceCapabilities ));
		}

		void setupSwapchain() {
			deviceIF.logical.swapchainConfiguration.swapchainCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = windowIF.surface,
				.minImageCount = windowIF.surfaceCapabilities.minImageCount,
				.imageFormat = deviceIF.logical.swapchainConfiguration.imageFormat,
				.imageColorSpace = deviceIF.logical.swapchainConfiguration.imageColorSpace,
				.imageExtent = {
					.width = windowIF.surfaceCapabilities.currentExtent.width,
					.height = windowIF.surfaceCapabilities.currentExtent.height
				},
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = VK_PRESENT_MODE_FIFO_KHR
			};

			validateResult( vkCreateSwapchainKHR( deviceIF.logical.device, &deviceIF.logical.swapchainConfiguration.swapchainCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchain ));

			validateResult( vkGetSwapchainImagesKHR( deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, nullptr  ) );
			deviceIF.logical.swapchainConfiguration.swapchainImages.resize(deviceIF.logical.swapchainConfiguration.imageCount);
			deviceIF.logical.swapchainConfiguration.swapchainImageViews.resize(deviceIF.logical.swapchainConfiguration.imageCount);

			validateResult(vkGetSwapchainImagesKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, deviceIF.logical.swapchainConfiguration.swapchainImages.data()));

			for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.imageCount; i++) {
				VkImageViewCreateInfo imageViewCreateInfo{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = deviceIF.logical.swapchainConfiguration.swapchainImages[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = deviceIF.logical.swapchainConfiguration.imageFormat,
					.subresourceRange {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1
					}
				};

				validateResult( vkCreateImageView( deviceIF.logical.device, &imageViewCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchainImageViews[i]));
			}
		}

		void setupSync2() {
			VkSemaphoreCreateInfo semaphoreCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
			};

			VkFenceCreateInfo fenceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT
			};

			for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.maxFramesInFlight; i++) {
				validateResult(vkCreateFence( deviceIF.logical.device, &fenceCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.fences[i]));
				validateResult(vkCreateSemaphore(deviceIF.logical.device, &semaphoreCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.presentSemaphore[i]));
			}

			deviceIF.logical.swapchainConfiguration.renderSemaphore.resize(deviceIF.logical.swapchainConfiguration.imageCount);
			for ( VkSemaphore& semaphore : deviceIF.logical.swapchainConfiguration.renderSemaphore ) {
				validateResult(vkCreateSemaphore(deviceIF.logical.device, &semaphoreCreateInfo, nullptr, &semaphore));
			}
		}

		void setupCommandBuffers() {
			VkCommandPoolCreateInfo commandPoolCreateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = deviceIF.queue.index
			};

			validateResult( vkCreateCommandPool( deviceIF.logical.device, &commandPoolCreateInfo, nullptr, &deviceIF.logical.commandPool ) );
		
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = deviceIF.logical.commandPool,
				.commandBufferCount = deviceIF.logical.swapchainConfiguration.maxFramesInFlight
			};

			validateResult( vkAllocateCommandBuffers( deviceIF.logical.device, &commandBufferAllocateInfo, deviceIF.logical.swapchainConfiguration.commandBuffers.data()  ) );
		}

		void loadAndCompileShaders() {
			slang::createGlobalSession(slangGlobalSession.writeRef());

			auto slangTargets{
				std::to_array< slang::TargetDesc >({{
					.format{SLANG_SPIRV},
					.profile{slangGlobalSession->findProfile("spirv_1_4")}
				}})
			};

			auto slangOptions{
				std::to_array < slang::CompilerOptionEntry>({{
					slang::CompilerOptionName::EmitSpirvDirectly,
					{
						slang::CompilerOptionValueKind::Int, 1
					}
				}})
			};

			slang::SessionDesc slangSessionDesc{
				.targets{
					slangTargets.data()
				},
				.targetCount{
					SlangInt(slangTargets.size())
				},
				.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
				.compilerOptionEntries{
					slangOptions.data()
				},
				.compilerOptionEntryCount{
					uint32_t(slangOptions.size())
				}
			};

			Slang::ComPtr< slang::ISession > slangSession;
			slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

			Slang::ComPtr< slang::IModule > slangModule{
				slangSession->loadModuleFromSource("Modern Vulkan SLANG Shader", "assets/shaders/shader.slang", nullptr, nullptr)
			};

			Slang::ComPtr< ISlangBlob > spirv;
			slangModule->getTargetCode(0, spirv.writeRef());

			VkShaderModuleCreateInfo shaderModuleCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = spirv->getBufferSize(),
				.pCode = (uint32_t*)spirv->getBufferPointer()
			};

			validateResult(vkCreateShaderModule(deviceIF.logical.device, &shaderModuleCreateInfo, nullptr, &shaderModule));

		}

		void loadTextures() {

		}

		void setup() {

		}

	private:
		struct ExtensionsIF {
			struct Instance {
				uint32_t count;
				const char* const* extensions;
			} instance;

			struct Device {
				const std::vector< const char* > extensions{
					VK_KHR_SWAPCHAIN_EXTENSION_NAME
				};
			} device;
		} extensionsIF;

		struct WindowIF {
			VkInstance vkInstance;

			SDL_Window* window;
			std::string windowName = "Ryutai";
			int windowWidth = 100;
			int windowHeight = 100;
			SDL_WindowFlags windowFlags{
				SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN
			};

			glm::ivec2 dimensions{};

			VkSurfaceKHR surface;
			VkSurfaceCapabilitiesKHR surfaceCapabilities;
		} windowIF;

		struct DeviceIF {
			struct physical {
				uint32_t count;
				std::vector< VkPhysicalDevice > devices;
				std::vector< VkPhysicalDeviceProperties2 > properties;
				uint32_t index;
				VkPhysicalDevice device;
			} physical;

			struct Queue {
				VkQueue queue;
				std::vector < std::vector< VkQueueFamilyProperties2 > > properties;
				uint32_t index;
			} queue;

			struct Logical {
				VkDevice device;
				VkCommandPool commandPool;

				struct SwapchainConfiguration {
					VkSwapchainCreateInfoKHR swapchainCreateInfo;
					VkSwapchainKHR swapchain;
					std::vector<VkImage> swapchainImages;
					std::vector<VkImageView> swapchainImageViews;
					VkColorSpaceKHR imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
					VkFormat imageFormat = VK_FORMAT_B8G8R8A8_SRGB;

					bool updateSwapchain{ false };
					uint32_t imageCount;

					static constexpr uint32_t maxFramesInFlight{ 2 };
					std::array< VkFence, maxFramesInFlight > fences;
					std::array< VkSemaphore, maxFramesInFlight > presentSemaphore;
					std::vector< VkSemaphore > renderSemaphore;
					std::array< VkCommandBuffer, maxFramesInFlight > commandBuffers{};
				} swapchainConfiguration;
			} logical;
		} deviceIF;

		VmaAllocator vmaAllocator;

		Slang::ComPtr< slang::IGlobalSession > slangGlobalSession;
		VkShaderModule shaderModule{};

		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet;
};

class Ryutai {
	public:
		void start() {
			renderer.setup();
		}

	private:
		Renderer renderer;
};

int main() {
	Ryutai fluidSimulation{};
	fluidSimulation.start();

	return 0;
}