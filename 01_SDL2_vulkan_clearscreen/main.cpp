#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>


#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>

//#define VK_PROTOTYPES
#define VK_USE_PLATFORM_XLIB_KHR

#include <vulkan/vulkan.h>

int windowWidth = 800;
int windowHeight = 600;

const char * applicationName = "mySdlVulkanTest";
const char * engineName = applicationName;


/**
 * debug callback; adapted from dbg_callback from vulkaninfo.c in the Vulkan SDK.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location,
             int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData)
{
	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		std::cout << "ERROR: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		std::cout << "WARNING: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		std::cout << "INFO: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		std::cout << "DEBUG: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;

	/*
	* false indicates that layer should not bail-out of an
	* API call that had validation failures. This may mean that the
	* app dies inside the driver due to invalid parameter(s).
	* That's what would happen without validation layers, so we'll
	* keep that behavior here.
	*/
	return false;
}




VkInstance createVkInstance()
{
	VkInstance myInstance;
	VkResult result;

	/*
	 * The VkApplicationInfo struct contains (optional) information
	 * about the application.
	 * It's passed to the VkInstanceCreateInfo struct through a pointer.
	 */
	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,  // Must be this value
		.pNext = nullptr,                       // Must be null (reserved for extensions)
		.pApplicationName = applicationName,    // Application name (UTF8, null terminated string)
		.applicationVersion = 1,                // Application version
		.pEngineName = engineName,              // Engine name (UTF8, null terminated string)
		.engineVersion = 1,                     // Engine version
		.apiVersion = VK_API_VERSION,           // Vulkan version the application expects to use;
		                                        // if = 0, this field is ignored, otherwise, if the implementation
		                                        // doesn't support the specified version, VK_ERROR_INCOMPATIBLE_DRIVER is returned.
	};


	/*
	 * The VkInstanceCreateInfo struct contains information regarding the creation of the VkInstance.
	 */
	VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  // Must be this value
		.pNext = nullptr,                       // Must be null (reserved for extensions)
		.flags = 0,                             // Must be 0 (reserved for future use)
		.pApplicationInfo = &applicationInfo,   // Pointer to a VkApplicationInfo struct (or can be null)
		.enabledLayerCount = 0,                 // Number of global layers to enable
		.ppEnabledLayerNames = nullptr,         // Pointer to array of #enabledLayerCount strings containing the names of the layers to enable
		.enabledExtensionCount = 0,             // Number of global extensions to enable
		.ppEnabledExtensionNames = nullptr,     // Pointer to array of #enabledExtensionCount strings containing the names of the extensions to enable
	};



	/*
	 * Layers:
	 * We list all the layers the current implementation supports,
	 * and we log them to the console.
	 */
	std::vector<VkLayerProperties> layerPropertiesVector;

	{
		uint32_t supportedLayersCount = 0;

		// Calling vkEnumerateInstanceLayerProperties with a null pProperties will yeld
		// the number of supported layer in pPropertyCount ("supportedLayersCount" here).
		result = vkEnumerateInstanceLayerProperties(&supportedLayersCount, nullptr);
		assert(result == VK_SUCCESS);

		// Allocate a vector to keep all the VkLayerProperties structs.
		layerPropertiesVector.resize(supportedLayersCount);

		// Get the LayerProperties.
		result = vkEnumerateInstanceLayerProperties(&supportedLayersCount, layerPropertiesVector.data());
		assert(result == VK_SUCCESS);

		// Log all the layers on the console.
		std::cout << "Found " << layerPropertiesVector.size() << " layers:" << std::endl;

		for(const auto & layer : layerPropertiesVector)
		{
			std::cout << "     Name        : " << layer.layerName             << std::endl;
			std::cout << "     Spec.Version: " << layer.specVersion           << std::endl;
			std::cout << "     Impl.Version: " << layer.implementationVersion << std::endl;
			std::cout << "     Description : " << layer.description           << std::endl;
			std::cout << std::endl;
		}
		std::cout << "------------------------------------------------------------------\n" << std::endl;
	}



	/*
	 * Extensions:
	 * Each layer has its own set of extensions; for each layer, we query the extensions it provides
	 * and we log them to the console.
	 * Note that the Vulkan implementation could provide extensions, as other implicitly enabled layers;
	 * we query those extensions too.
	 */

	// Helper lambda to query extensions for a certain layer name and print them to console
	const auto queryAndPrintExtensions = [&](const char * layerName)
	{
		uint32_t propertyCount = 0;

		// Query the number of extensions
		result = vkEnumerateInstanceExtensionProperties(layerName, &propertyCount, nullptr);
		assert(result == VK_SUCCESS);

		// Allocate a vector to store the extension properties
		std::vector<VkExtensionProperties> extPropertiesVector(propertyCount);

		// Query the actual properties
		result = vkEnumerateInstanceExtensionProperties(layerName, &propertyCount, extPropertiesVector.data());
		assert(result == VK_SUCCESS);

		// Log them to console
		std::cout << "Found " << extPropertiesVector.size() << " extensions for layer " << (layerName==nullptr ? "(null)" : layerName) << std::endl;

		for(const auto & ext : extPropertiesVector) {
			std::cout << "     Name        : " << ext.extensionName << std::endl;
			std::cout << "     Spec.Version: " << ext.specVersion   << std::endl;
			std::cout << std::endl;
		}

		return extPropertiesVector;
	};

	// Passing a null pointer as the name of the layer, we get the Vulkan implementation's (global) extensions,
	// and the extensions of the implicitly enabled layers.
	auto globalExtensionsVector = queryAndPrintExtensions(nullptr);

	for(const auto & layer : layerPropertiesVector)
		queryAndPrintExtensions(layer.layerName);

	std::cout << "------------------------------------------------------------------\n" << std::endl;



	/*
	 * Layers enable:
	 * Here we can populate a vector of layer names we want to enable,
	 * and pass them through the ppEnabledLayerNames pointers inside VkInstanceCreateInfo.
	 */
	std::vector<const char *> layersNamesToEnable;
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_threading");       // Enable all the standard validation layers that come with the VulkanSDK.
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_param_checker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_device_limits");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_object_tracker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_image");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_mem_tracker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_draw_state");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_swapchain");
	layersNamesToEnable.push_back("VK_LAYER_GOOGLE_unique_objects");

	// Check if all the layer names we want to enable are present
	// in the VkLayerProperties we collected before in layerPropertiesVector.
	for(const auto & layerName : layersNamesToEnable)
	{
		auto itr = std::find_if(layerPropertiesVector.begin(), layerPropertiesVector.end(),
			[&](const VkLayerProperties & extProp){
				return strcmp(layerName, extProp.layerName) == 0;
			}
		);

		if(itr == layerPropertiesVector.end()) {
			std::cout << "ERROR: Layer " << layerName << " was not found." << std::endl;
			exit(1);
		}
	}

	// We pass the pointer and size of the extension names vector to the VkInstanceCreateInfo struct
	instanceCreateInfo.enabledLayerCount = (uint32_t)layersNamesToEnable.size();
	instanceCreateInfo.ppEnabledLayerNames = layersNamesToEnable.data();



	/*
	 * Extensions enable:
	 * Here we can populate a vector of extension names we want to enable,
	 * and pass them through the ppEnabledExtensionNames pointers inside VkInstanceCreateInfo.
	 */
	std::vector<const char *> extensionsNamesToEnable;
	extensionsNamesToEnable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);	// TODO: add support for other windowing systems

	// Check if all the extension names we want to enable are present
	// in the VkExtensionProperties we collected before in globalExtensionsVector.
	for(const auto & extName : extensionsNamesToEnable)
	{
		auto itr = std::find_if(globalExtensionsVector.begin(), globalExtensionsVector.end(),
			[&](const VkExtensionProperties & extProp){
				return strcmp(extName, extProp.extensionName) == 0;
			}
		);

		if(itr == globalExtensionsVector.end()) {
			std::cout << "ERROR: extension " << extName << " was not found in the global extensions vector." << std::endl;
			exit(1);
		}
	}

	// We pass the pointer and size of the extension names vector to the VkInstanceCreateInfo struct
	instanceCreateInfo.enabledExtensionCount = (uint32_t)extensionsNamesToEnable.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensionsNamesToEnable.data();



	/*
	 * Debug Extension:
	 * Can't find documentation for this; this code is taken from vulkaninfo.c in the Vulkan SDK.
	 */
	VkDebugReportCallbackCreateInfoEXT dbg_info;
	memset(&dbg_info, 0, sizeof(dbg_info));
	dbg_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	dbg_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
					 VK_DEBUG_REPORT_WARNING_BIT_EXT |
					 VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	dbg_info.pfnCallback = debugCallback;
	instanceCreateInfo.pNext = &dbg_info;



	/*
	 * Here the magic happens: the VkInstance gets created from
	 * the structs we filled before.
	 */
	result = vkCreateInstance(&instanceCreateInfo, nullptr, &myInstance);

	if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
		std::cout << "ERROR: Cannot create Vulkan instance, VK_ERROR_INCOMPATIBLE_DRIVER." << std::endl;
		exit(1);
	} else if(result != VK_SUCCESS) {
		std::cout << "ERROR: Cannot create Vulkan instance, <error name here>" << std::endl;
		exit(1);
	}

	// YAY! We have our VkInstance!
	return myInstance;
}





















int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);   // Initialize SDL2

	SDL_Window *window;        // Declare a pointer to an SDL_Window
	SDL_SysWMinfo info;

	// Create an application window with the following settings:
	window = SDL_CreateWindow(
		"An SDL2 window",         // const char* title
		SDL_WINDOWPOS_UNDEFINED,  // int x: initial x position
		SDL_WINDOWPOS_UNDEFINED,  // int y: initial y position
		windowWidth,              // int w: width, in pixels
		windowHeight,             // int h: height, in pixels
		SDL_WINDOW_SHOWN          // Uint32 flags: window options, see docs
	);


	SDL_VERSION(&info.version);   // initialize info structure with SDL version info

	if(SDL_GetWindowWMInfo(window,&info))
	{
		const char *subsystem = "an unknown system!";
		switch(info.subsystem)
		{
			case SDL_SYSWM_UNKNOWN:   break;
			case SDL_SYSWM_WINDOWS:   subsystem = "Microsoft Windows(TM)";  break;
			case SDL_SYSWM_X11:       subsystem = "X Window System";        break;
		#if SDL_VERSION_ATLEAST(2, 0, 3)
			case SDL_SYSWM_WINRT:     subsystem = "WinRT";                  break;
		#endif
			case SDL_SYSWM_DIRECTFB:  subsystem = "DirectFB";               break;
			case SDL_SYSWM_COCOA:     subsystem = "Apple OS X";             break;
			case SDL_SYSWM_UIKIT:     subsystem = "UIKit";                  break;
		#if SDL_VERSION_ATLEAST(2, 0, 2)
			case SDL_SYSWM_WAYLAND:   subsystem = "Wayland";                break;
			case SDL_SYSWM_MIR:       subsystem = "Mir";                    break;
		#endif
		#if SDL_VERSION_ATLEAST(2, 0, 4)
			case SDL_SYSWM_ANDROID:   subsystem = "Android";                break;
		#endif
		}

		SDL_Log("This program is running SDL version %d.%d.%d on %s\n",
			(int)info.version.major,
			(int)info.version.minor,
			(int)info.version.patch,
			subsystem);
	}
	else
	{
		// call failed
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get window information: %s\n", SDL_GetError());
	}


	/*
	 * Do all the Vulkan goodies here.
	 */

	VkInstance myInstance = createVkInstance();


	vkDestroyInstance(myInstance, nullptr);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
