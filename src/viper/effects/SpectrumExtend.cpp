#include "SpectrumExtend.h"
#include "../constants.h"
#include <cstdio>

static const float SPECTRUM_HARMONICS[10] = {
        0.02f,
        0.0f,
        0.02f,
        0.0f,
        0.02f,
        0.0f,
        0.02f,
        0.0f,
        0.02f,
        0.0f,
};

SpectrumExtend::SpectrumExtend() {
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->referenceFreq = 7600;
    this->enabled = false;
    this->exciter = 0.0;
    Reset();
}

void SpectrumExtend::Process(double *sampleL, double *sampleR) {
    if (!this->enabled) return;

    double tmp;

    tmp = this->highpass[0].ProcessSample(*sampleL);
    tmp = this->harmonics[0].Process(tmp);
    tmp = this->lowpass[0].ProcessSample(tmp * this->exciter);
    *sampleL = *sampleL + tmp;

    tmp = this->highpass[1].ProcessSample(*sampleR);
    tmp = this->harmonics[1].Process(tmp);
    tmp = this->lowpass[1].ProcessSample(tmp * this->exciter);
    *sampleR = *sampleR + tmp;

}

void SpectrumExtend::Reset() {
    this->highpass[0].RefreshFilter(MultiBiquad::FilterType::HIGH_PASS, 0.0, (float) this->referenceFreq, this->samplingRate,
                                    0.717, false);
    this->highpass[1].RefreshFilter(MultiBiquad::FilterType::HIGH_PASS, 0.0, (float) this->referenceFreq, this->samplingRate,
                                    0.717, false);

    this->lowpass[0].RefreshFilter(MultiBiquad::FilterType::LOW_PASS, 0.0, (float) this->samplingRate / 2.0f - 2000.0f,
                                   this->samplingRate, 0.717, false);
    this->lowpass[1].RefreshFilter(MultiBiquad::FilterType::LOW_PASS, 0.0, (float) this->samplingRate / 2.0f - 2000.0f,
                                   this->samplingRate, 0.717, false);

    this->harmonics[0].Reset();
    this->harmonics[1].Reset();

    this->harmonics[0].SetHarmonics(SPECTRUM_HARMONICS);
    this->harmonics[1].SetHarmonics(SPECTRUM_HARMONICS);
}

void SpectrumExtend::SetEnable(bool enable) {
    if (this->enabled != enable) {
        if (enable) {
            Reset();
        }
        this->enabled = enable;
    }
}

void SpectrumExtend::SetExciter(float value) {
    this->exciter = value;
}

void SpectrumExtend::SetReferenceFrequency(uint32_t freq) {
    if (this->samplingRate / 2 - 100 < freq) {
        freq = this->samplingRate / 2 - 100;
    }
    this->referenceFreq = freq;
    Reset();
}

void SpectrumExtend::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        if (samplingRate / 2 - 100 < this->referenceFreq) {
            this->referenceFreq = samplingRate / 2 - 100;
        }
        Reset();
    }
}
