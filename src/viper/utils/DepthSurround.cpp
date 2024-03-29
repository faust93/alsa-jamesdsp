#include "DepthSurround.h"
#include <cmath>
#include "../constants.h"

DepthSurround::DepthSurround() {
    this->strength = 0;
    this->enabled = false;
    this->strengthAtLeast500 = false;
    this->gain = 0;
    for (auto &prev : this->prev) {
        prev = 0.0f;
    }
    this->SetSamplingRate(VIPER_DEFAULT_SAMPLING_RATE);
    this->RefreshStrength(this->strength);
}

void DepthSurround::Process(double *samplesL, double *samplesR, uint32_t size) {
    if (this->enabled) {
        if (!this->strengthAtLeast500) {
            for (uint32_t i = 0; i < size; i++) {
                float sampleLeft = samplesL[i];
                float sampleRight = samplesR[i];

                this->prev[0] = this->gain * this->timeConstDelay[0].ProcessSample(sampleLeft + this->prev[1]);
                this->prev[1] = this->gain * this->timeConstDelay[1].ProcessSample(sampleRight + this->prev[0]);

                float l = this->prev[0] + sampleLeft;
                float r = this->prev[1] + sampleRight;

                float diff = (l - r) / 2.f;
                float avg = (l + r) / 2.f;
                float avgOut = (float) this->highpass.ProcessSample(diff);
                samplesL[i] = avg + (diff - avgOut);
                samplesR[i] = avg - (diff - avgOut);
            }
        } else {
            for (uint32_t i = 0; i < size; i++) {
                float sampleLeft = samplesL[i];
                float sampleRight = samplesR[i];

                this->prev[0] = this->gain * this->timeConstDelay[0].ProcessSample(sampleLeft + this->prev[1]);
                this->prev[1] = -this->gain * this->timeConstDelay[1].ProcessSample(sampleRight + this->prev[0]);

                float l = this->prev[0] + sampleLeft;
                float r = this->prev[1] + sampleRight;

                float diff = (l - r) / 2.f;
                float avg = (l + r) / 2.f;
                float avgOut = (float) this->highpass.ProcessSample(diff);
                samplesL[i] = avg + (diff - avgOut);
                samplesR[i] = avg - (diff - avgOut);
            }
        }
    }
}

void DepthSurround::RefreshStrength(short strength) {
    this->strengthAtLeast500 = strength >= 500;
    this->enabled = strength != 0;
    if (strength != 0) {
        float gain = (float) pow(10.0, ((strength / 1000.0) * 10.0 - 15.0) / 20.0);
        if (gain > 1.0) {
            gain = 1.0;
        }
        this->gain = (float) gain;
    } else {
        this->gain = 0.0;
    }
}

void DepthSurround::SetSamplingRate(uint32_t samplingRate) {
    this->timeConstDelay[0].SetParameters(samplingRate, 0.02);
    this->timeConstDelay[1].SetParameters(samplingRate, 0.014);
    this->highpass.SetHighPassParameter(800.0f, samplingRate, -11.0f, 0.72f, 0.0);
    for (auto &prev : this->prev) {
        prev = 0.0f;
    }
}

void DepthSurround::SetStrength(short strength) {
    this->strength = strength;
    this->RefreshStrength(strength);
}