// src/intf/video/vid_interface_metal.h
#pragma once

int FBNeo_InitializeVideoMetal(int width, int height);
void FBNeo_ShutdownVideoMetalInterface();
void FBNeo_DrawFrameMetalInterface(const void* frameBuffer, int width, int height);