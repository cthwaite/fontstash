#pragma once
#include <cmath>

// Based on Exponential blur, Jani Huhtanen, 2006

namespace fontstash {

    static constexpr int APREC = 16;
    static constexpr int ZPREC = 7;

    static void blurCols(unsigned char* dst, int w, int h, int dstStride, int alpha)
    {
        int x, y;
        for (y = 0; y < h; y++) {
            int z = 0; // force zero border
            for (x = 1; x < w; x++) {
                z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
                dst[x] = (unsigned char)(z >> ZPREC);
            }
            dst[w-1] = 0; // force zero border
            z = 0;
            for (x = w-2; x >= 0; x--) {
                z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
                dst[x] = (unsigned char)(z >> ZPREC);
            }
            dst[0] = 0; // force zero border
            dst += dstStride;
        }
    }

    static void blurRows(unsigned char* dst, int w, int h, int dstStride, int alpha)
    {
        int x, y;
        for (x = 0; x < w; x++) {
            int z = 0; // force zero border
            for (y = dstStride; y < h*dstStride; y += dstStride) {
                z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
                dst[y] = (unsigned char)(z >> ZPREC);
            }
            dst[(h-1)*dstStride] = 0; // force zero border
            z = 0;
            for (y = (h-2)*dstStride; y >= 0; y -= dstStride) {
                z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
                dst[y] = (unsigned char)(z >> ZPREC);
            }
            dst[0] = 0; // force zero border
            dst++;
        }
    }


    static void blur(unsigned char* dst, int w, int h, int dstStride, int blur)
    {
        int alpha;
        float sigma;

        if (blur < 1)
            return;
        // Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
        sigma = static_cast<float>(blur) * 0.57735f; // 1 / sqrt(3)
        alpha = (int)((1<<APREC) * (1.0f - std::expf(-2.3f / (sigma+1.0f))));
        blurRows(dst, w, h, dstStride, alpha);
        blurCols(dst, w, h, dstStride, alpha);
        blurRows(dst, w, h, dstStride, alpha);
        blurCols(dst, w, h, dstStride, alpha);
    //  blurrows(dst, w, h, dstStride, alpha);
    //  blurcols(dst, w, h, dstStride, alpha);
    }
}
