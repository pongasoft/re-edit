// dear imgui: Renderer Backend for Metal - re-edit additions

#ifndef RE_GUI_IMPL_METAL_H
#define RE_GUI_IMPL_METAL_H

#include <Metal/Metal.hpp>

#import <QuartzCore/QuartzCore.hpp>

void ImGui_ImplMetal_Layer_SetDevice(void* layer, MTL::Device *iDevice);
void ImGui_ImplMetal_Layer_SetDrawableSize(void* layer, int iWidth, int iHeight);
void ImGui_ImplMetal_Layer_SetPixelFormat(void* layer, int pixelFormat);
CA::MetalDrawable* ImGui_ImplMetal_Layer_GetNextDrawable(void* layer);
void* ImGui_ImplMetal_Layer();
void ImGui_ImplMetal_NSWindow_SetLayer(void *iWindow, void* layer);

#endif // RE_GUI_IMPL_METAL_H