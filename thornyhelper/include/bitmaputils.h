#ifndef __BITMAP_SMART_CROP_H__
#define __BITMAP_SMART_CROP_H__

#include <stdint.h>

constexpr int SMART_CROP_W = 400;
constexpr int SMART_CROP_H = 400;

void CalcBitmapSmartCrop(float* smart_crop, uint8_t* pixels, int width, int height,
    		float slice_l, float slice_t, float slice_r, float slice_b);

#endif //__BITMAP_SMART_CROP_H__
