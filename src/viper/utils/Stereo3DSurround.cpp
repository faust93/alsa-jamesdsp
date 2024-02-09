#include "Stereo3DSurround.h"

Stereo3DSurround::Stereo3DSurround() {
    this->middleImage = 1.0;
    this->stereoWiden = 0.0;
    this->coeffLeft = 0.5;
    this->coeffRight = 0.5;
}

void Stereo3DSurround::Process(double *samplesL, double *samplesR, uint32_t size) {
    if (size == 0) return;

    for (uint32_t i = 0; i < size; i++) {
        float a = this->coeffLeft * (samplesL[i] + samplesR[i]);
        float b = this->coeffRight * (samplesR[i] - samplesL[i]);
        samplesL[i] = a - b;
        samplesR[i] = a + b;
    }
}

inline void Stereo3DSurround::ConfigureVariables() {
    float tmp = this->stereoWiden + 1.0f;

    float x = tmp + 1.0f;
    float y;
    if (x < 2.0) {
        y = 0.5;
    } else {
        y = 1.0f / x;
    }

    this->coeffLeft = this->middleImage * y;
    this->coeffRight = tmp * y;
}

void Stereo3DSurround::SetMiddleImage(float middleImage) {
    this->middleImage = middleImage;
    this->ConfigureVariables();
}

void Stereo3DSurround::SetStereoWiden(float stereoWiden) {
    this->stereoWiden = stereoWiden;
    this->ConfigureVariables();
}
