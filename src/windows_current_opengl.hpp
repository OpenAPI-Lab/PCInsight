#pragma once
#include "pcinsight/types.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <memory>
namespace pcinsight::detail {
struct CurrentWglApi {
    decltype(&wglGetCurrentContext) context{};
    decltype(&wglGetCurrentDC) dc{};
    decltype(&wglGetProcAddress) proc{};
    decltype(&glGetString) string{};
    decltype(&glGetIntegerv) integer{};
    std::shared_ptr<void> library;
    Support failure_support{Support::unsupported};
};
CurrentWglApi load_windows_current_wgl();
void fill_windows_current_opengl(std::vector<GpuInfo>&, const CurrentWglApi&);
void fill_windows_current_opengl(std::vector<GpuInfo>&);
}
#endif
