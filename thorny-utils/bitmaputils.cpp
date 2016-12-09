#include "bitmaputils.h"

#define V_LINE_SIZE 5
#define H_LINE_SIZE 5
#define LINE_MARGIN 20
#define WHITE_THRESHOLD 0.005
#define COLUMN_WIDTH 5
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static int CalcAvgLum(
        uint8_t* pixels,
        int width,
        int height,
        int sub_x,
        int sub_y,
        int sub_w,
        int sub_h)
{
    int i;
    int midBright = 0;

    int x, y;
    for (y = 0; y < sub_h; y++) {
        for (x = 0; x < sub_w; x++) {
            i = ((y + sub_y) * width + sub_x + x) * 4;
            midBright += (MIN(pixels[i+2], MIN(pixels[i + 1],pixels[i]))
                    + MAX(pixels[i+2], MAX(pixels[i + 1],pixels[i])))
                    / 2;
        }
    }
    midBright /= (sub_w * sub_h);

    return midBright;
}

static int IsRectWhite(
        uint8_t* pixels,
        int width,
        int height,
        int sub_x,
        int sub_y,
        int sub_w,
        int sub_h,
        int avg_lum)
{
    int count = 0;

    int x, y;
    for (y = 0; y < sub_h; y++) {
        for (x = 0; x < sub_w; x++) {
            int i = ((y + sub_y) * width + sub_x + x) * 4;
            int minLum = MIN(pixels[i+2],MIN(pixels[i + 1],pixels[i]));
            int maxLum = MAX(pixels[i+2],MAX(pixels[i + 1],pixels[i]));
            int lum = (minLum + maxLum) / 2;
            if ((lum < avg_lum) && ((avg_lum - lum) * 10 > avg_lum)) {
                count++;
            }
        }
    }
    float white = (float) count / (sub_w * sub_h);
    return white < WHITE_THRESHOLD ? 1 : 0;
}

static float GetLeftCropBound(uint8_t* pixels, int width, int height, int avg_lum)
{
    int w = width / 3;
    int whiteCount = 0;
    int x = 0;

    for (x = 0; x < w; x += V_LINE_SIZE) {
        int white = IsRectWhite(
                pixels,
                width,
                height,
                x,
                LINE_MARGIN,
                V_LINE_SIZE,
                height - 2 * LINE_MARGIN,
                avg_lum);
        if (white) {
            whiteCount++;
        } else {
            if (whiteCount >= 1) {
                return (float) (MAX(0, x - V_LINE_SIZE)) / width;
            }
            whiteCount = 0;
        }
    }
    return whiteCount > 0 ? (float) (MAX(0, x - V_LINE_SIZE)) / width : 0;
}

static float GetTopCropBound(uint8_t* pixels, int width, int height, int avg_lum)
{
    int h = height / 3;
    int whiteCount = 0;
    int y = 0;

    for (y = 0; y < h; y += H_LINE_SIZE) {
        int white = IsRectWhite(
                pixels,
                width,
                height,
                LINE_MARGIN,
                y,
                width - 2 * LINE_MARGIN,
                H_LINE_SIZE,
                avg_lum);
        if (white) {
            whiteCount++;
        } else {
            if (whiteCount >= 1) {
                return (float) (MAX(0, y - H_LINE_SIZE)) / height;
            }
            whiteCount = 0;
        }
    }
    return whiteCount > 0 ? (float) (MAX(0, y - H_LINE_SIZE)) / height : 0;
}

static float GetRightCropBound(uint8_t* pixels, int width, int height, int avg_lum)
{
    int w = width / 3;
    int whiteCount = 0;
    int x = 0;

    for (x = width - V_LINE_SIZE; x > width - w; x -= V_LINE_SIZE) {
        int white = IsRectWhite(
                pixels,
                width,
                height,
                x,
                LINE_MARGIN,
                V_LINE_SIZE,
                height - 2 * LINE_MARGIN,
                avg_lum);
        if (white) {
            whiteCount++;
        } else {
            if (whiteCount >= 1) {
                return (float) (MIN(width, x + 2 * V_LINE_SIZE)) / width;
            }
            whiteCount = 0;
        }
    }
    return whiteCount > 0 ? (float) (MIN(width, x + 2 * V_LINE_SIZE)) / width : 1;
}

static float GetBottomCropBound(uint8_t* pixels, int width, int height, int avg_lum)
{
    int h = height / 3;
    int whiteCount = 0;
    int y = 0;
    for (y = height - H_LINE_SIZE; y > height - h; y -= H_LINE_SIZE) {
        int white = IsRectWhite(
                pixels,
                width,
                height,
                LINE_MARGIN,
                y,
                width - 2 * LINE_MARGIN,
                H_LINE_SIZE,
                avg_lum);
        if (white) {
            whiteCount++;
        } else {
            if (whiteCount >= 1) {
                return (float) (MIN(height, y + 2 * H_LINE_SIZE)) / height;
            }
            whiteCount = 0;
        }
    }
    return whiteCount > 0 ? (float) (MIN(height, y + 2 * H_LINE_SIZE)) / height : 1;
}

void CalcBitmapSmartCrop(float* smart_crop, uint8_t* pixels, int width, int height,
		float slice_l, float slice_t, float slice_r, float slice_b)
{
	int avg_lum = CalcAvgLum(pixels, width, height, 0, 0, width, height);
	smart_crop[0] = GetLeftCropBound(pixels, width, height, avg_lum)
	        * (slice_r - slice_l) + slice_l;
	smart_crop[1] = GetTopCropBound(pixels, width, height, avg_lum)
	        * (slice_b - slice_t) + slice_t;
	smart_crop[2] = GetRightCropBound(pixels, width, height, avg_lum)
	        * (slice_r - slice_l) + slice_l;
	smart_crop[3] = GetBottomCropBound(pixels, width, height, avg_lum)
	        * (slice_b - slice_t) + slice_t;
}
