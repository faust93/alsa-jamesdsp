#pragma once

#include <cstdint>

class Stereo3DSurround {
public:
    Stereo3DSurround();

    void Process(double *samplesL, double *samplesR, uint32_t size);
    void SetMiddleImage(float middleImage);
    void SetStereoWiden(float stereoWiden);

private:
    void ConfigureVariables();

    float stereoWiden;
    float middleImage;
    float coeffLeft;
    float coeffRight;
};


