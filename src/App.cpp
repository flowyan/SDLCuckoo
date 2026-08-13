#include "App.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

App::App(int argc, char **argv) : m_window(nullptr, &SDL_DestroyWindow), m_gpuDevice(nullptr, &SDL_DestroyGPUDevice) {
}

SDL_AppResult App::Init() {
    SDL_SetAppMetadata("Cuckoo", "1.0.0", "dev.flwn.cuckoo");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Cuckoo");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);

    // Create the window with properties set above
    m_window.reset(SDL_CreateWindowWithProperties(props));
    SDL_DestroyProperties(props);

    if (!m_window) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    std::string iconPath(SDL_GetBasePath());
    iconPath.append("resources/bird.png");
    SDL_Surface* windowIcon = IMG_Load(iconPath.c_str());
    if (!windowIcon) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to load window icon: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetWindowIcon(m_window.get(), windowIcon)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to set window icon: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_DestroySurface(windowIcon);

    SDL_Log("Supported GPU drivers:");
    for (int i = 0; i < SDL_GetNumGPUDrivers(); ++i) {
        SDL_Log("    %s", SDL_GetGPUDriver(i));
    }

    m_gpuDevice.reset(SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        true,
        nullptr // picks the best driver?
    ));

    if (!m_gpuDevice) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("Selected GPU driver: %s", SDL_GetGPUDeviceDriver(m_gpuDevice.get()));

    if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice.get(), m_window.get())) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to claim window for GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
    if (SDL_WindowSupportsGPUPresentMode(m_gpuDevice.get(), m_window.get(), SDL_GPU_PRESENTMODE_MAILBOX))
        presentMode = SDL_GPU_PRESENTMODE_MAILBOX;

    SDL_SetGPUSwapchainParameters(
        m_gpuDevice.get(),
        m_window.get(),
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        presentMode
    );

    if (!SDL_ShowWindow(m_window.get())) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to show window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::Iterate() {
    if (const auto result = OnUpdate(); result != SDL_APP_CONTINUE)
        return result;
    if (const auto result = OnRender(); result != SDL_APP_CONTINUE)
        return result;
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::Event(SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return OnQuit();
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (SDL_GetWindowID(m_window.get()) == event->window.windowID)
                return OnQuit();
        default: return SDL_APP_CONTINUE;
    }
}

void App::Quit(SDL_AppResult result) {
    SDL_WaitForGPUIdle(m_gpuDevice.get());
    SDL_ReleaseWindowFromGPUDevice(m_gpuDevice.get(), m_window.get());
}

SDL_AppResult App::OnRender() {
    auto commandBuffer = SDL_AcquireGPUCommandBuffer(m_gpuDevice.get());

    SDL_GPUTexture *swapchainTexture{};
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, m_window.get(), &swapchainTexture, nullptr, nullptr)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to acquire swapchain texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (swapchainTexture) {
        const std::array colorTargetInfos{
            SDL_GPUColorTargetInfo{
                .texture = swapchainTexture,
                .clear_color = (SDL_FColor){0.0f, 1.0f, 0.0f, 1.0f},
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            }
        };

        const auto renderPass = SDL_BeginGPURenderPass(
            commandBuffer,
            colorTargetInfos.data(),
            colorTargetInfos.size(),
            nullptr
        );

        SDL_EndGPURenderPass(renderPass);
    }

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to submit command buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::OnUpdate() {
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::OnQuit() {
    return SDL_APP_SUCCESS;
}
