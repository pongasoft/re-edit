// dear imgui: Renderer Backend for Metal - re-edit additions

#include "regui_impl_metal.h"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <AppKit/NSWindow.h>

void ImGui_ImplMetal_Layer_SetDevice(void* layer, MTL::Device *iDevice)
{
  auto metalLayer = (CAMetalLayer*) layer;
  metalLayer.device = (__bridge id<MTLDevice>) iDevice;
}

void ImGui_ImplMetal_Layer_SetPixelFormat(void* layer, int pixelFormat)
{
  auto metalLayer = (CAMetalLayer*) layer;
  metalLayer.pixelFormat = (MTLPixelFormat) pixelFormat;
}

void ImGui_ImplMetal_Layer_SetDrawableSize(void* layer, int iWidth, int iHeight)
{
  auto metalLayer = (CAMetalLayer*) layer;
  metalLayer.drawableSize = CGSizeMake(iWidth, iHeight);
}

CA::MetalDrawable* ImGui_ImplMetal_Layer_GetNextDrawable(void* layer)
{
  auto metalLayer = (CAMetalLayer*) layer;
  id <CAMetalDrawable> metalDrawable = [metalLayer nextDrawable];
  return ( __bridge CA::MetalDrawable*) metalDrawable;
}

void* ImGui_ImplMetal_Layer()
{
  CAMetalLayer *layer = [CAMetalLayer layer];
  return layer;
}

void ImGui_ImplMetal_NSWindow_SetLayer(void *iWindow, void* layer)
{
  auto nswin = (NSWindow *) iWindow;
  auto metalLayer = (CAMetalLayer*) layer;
  nswin.contentView.layer = metalLayer;
  nswin.contentView.wantsLayer = YES;
}
