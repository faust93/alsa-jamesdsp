# ALSA JamesDSP

ALSA JamesDSP Engine PCM Plugin [pre-EEL2 version](https://github.com/james34602/JamesDSPManager/blob/master/VeryOldOpen_source_edition/)

This plugin based on [gst-plugin-jamesdsp](https://github.com/Audio4Linux/gst-plugin-jamesdsp.git) by @ThePBone

### Features

Supported channels:

* 2

Supported bit depth:

| # bits    | Status      |
| --------- | ----------- |
| 8         | Unsupported |
| 16        | Supported   |
| 24(Int)   | Unsupported |
| 32(Int)   | Supported   |
| 32(Float) | Supported   |

Supported samplerates:

* 44100
* 48000

Effects:

* Analog modelling (12AX7)
* BS2B
* ViPER DDCs
* Limiter
* Compression
* Convolver
* Reverbation (Progenitor2)
* Bass boost
* Stereo widener (Mid/Side)
* IIR filters (NEW! Foobar2000's IIR DSP Filters port)

### Build & Install

Clone repository

```bash
git clone https://github.com/faust93/alsa-jamesdsp
cd alsa-jamesdsp
cmake .
make
sudo cp libasound_module_pcm_jdspfx.so /usr/lib/alsa-lib
```

### Configuration

#### ALSA configuration example

asound.conf:

```
pcm.jdspfx {
    type jdspfx
    ctl_fifo_path "/tmp/.jdspfx.ctl"
    settings_path "/tmp/jdspfx.txt"
    pcm_format "f32"
    slave.pcm "plughw:1"
}
pcm.!default {
    type plug
    slave.pcm jdspfx
}
```

##### ALSA Plugin options

* `ctl_fifo_path` a full path to FIFO pipe used to control DSP parameters in real time
* `settings_path` a full path to the settings file used by the plugin for DSP configuration & persistency
* `pcm_format` used for mandatory lock PCM extplug to the desired sample format for audio processing. Supported formats are:**f32**,**s32**,**s16** (SND_PCM_FORMAT_FLOAT, SND_PCM_FORMAT_S32, SND_PCM_FORMAT_S16). This option can be omitted thus ALSA will use sample format as of original source but in case it's not supported by the DSP it will fail.

#### DSP properties

Settings file

**jdspfx.txt:**

```
FX_ENABLE=1
TUBE_ENABLE=0
TUBE_DRIVE=0
BASS_ENABLE=0
BASS_MODE=0
BASS_FILTERTYPE=0
BASS_FREQ=100
STEREOWIDE_ENABLE=0
STEREOWIDE_MCOEFF=0
STEREOWIDE_SCOEFF=0
BS2B_ENABLE=0
BS2B_FCUT=300
BS2B_FEED=10
COMPRESSOR_ENABLE=0
COMPRESSOR_PREGAIN=12
COMPRESSOR_THRESHOLD=-60
COMPRESSOR_KNEE=30
COMPRESSOR_RATIO=12
COMPRESSOR_ATTACK=1
COMPRESSOR_RELEASE=24
TONE_ENABLE=0
TONE_FILTERTYPE=0
TONE_EQ=1200;50;-200;-500;-500;-500;-500;-450;-250;0;-300;-50;0;0;50
MASTER_LIMTHRESHOLD=0.000000
MASTER_LIMRELEASE=60.000000
DDC_ENABLE=1
DDC_COEFFS=/home/faust/dev/dsp/DDC/V4ARISE.vdc
CONVOLVER_ENABLE=0
CONVOLVER_FILE=/tmp/file.irs
CONVOLVER_GAIN=0.000000
CONVOLVER_BENCH_C0=0.000000
CONVOLVER_BENCH_C1=0.000000
HEADSET_ENABLE=1
HEADSET_OSF=0
HEADSET_REFLECTION_AMOUNT=0.000000
HEADSET_FINALWET=0.000000
HEADSET_FINALDRY=0.000000
HEADSET_REFLECTION_FACTOR=0.500000
HEADSET_REFLECTION_WIDTH=0.000000
HEADSET_WIDTH=0.000000
HEADSET_WET=0.000000
HEADSET_LFO_WANDER=0.100000
HEADSET_BASSBOOST=0.100000
HEADSET_LFO_SPIN=0.000000
HEADSET_DECAY=0.100000
HEADSET_DELAY=0
HEADSET_LPF_INPUT=200
HEADSET_LPF_BASS=50
HEADSET_LPF_DAMP=200
HEADSET_LPF_OUTPUT=200
IIR_ENABLE=0
IIR_FILTER=11
IIR_FREQ=26769
IIR_GAIN=91
IIR_QFACT=0.900000
```

##### Change DSP properties in real time

While processing audio DSP parameters can be tuned in real time using control pipe. Ex:

```bash
echo -n "FX_ENABLE=1" > /tmp/.jdspfx.ctl
echo -n "CONVOLVER_FILE=/tmp/boom.irs"  > /tmp/.jdspfx.ctl
echo -n "CONVOLVER_ENABLE=1" > /tmp/.jdspfx.ctl
echo -n "COMMIT" > /tmp/.jdspfx.ctl
```

There's a simple app as well used to control DSP properties using web browser [alsa-jamesdsp-gui](https://github.com/faust93/alsa-jamesdsp-gui)

##### Properties description

| Property                  | Type  | Values Min/Max   | Description                                                                                                                                          |
| ------------------------- | ----- | ---------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| FX_ENABLE                 | Bool  | 0..1             | Turn ON/OFF JamesDSP processing                                                                                                                      |
| TUBE_ENABLE               | Bool  | 0..1             | Enable analog modelling                                                                                                                              |
| TUBE_DRIVE                | Int   | 0..12000         | Tube drive/strength                                                                                                                                  |
| BASS_ENABLE               | Bool  | 0..1             | Enable bass boost                                                                                                                                    |
| BASS_MODE                 | Int   | 0..3000          | Bass boost mode/strength                                                                                                                             |
| BASS_FREQ                 | Int   | 30..300          | Bass boost cutoff frequency (Hz)                                                                                                                     |
| BASS_FILTERTYPE           | Int   | 0..1             | Bass boost filtertype [Linear phase 2049/4097 lowpass filter]                                                                                        |
| HEADSET_ENABLE            | Bool  | 0..1             | Reverb Enabled                                                                                                                                       |
| HEADSET_OSF               | Int   | 1..4             | Oversample factor: how much to oversample                                                                                                            |
| HEADSET_REFLECTION_AMOUNT | Float | 0..1             | Early reflection amount                                                                                                                              |
| HEADSET_FINALWET          | Float | -70..10          | Final wet mix (dB)                                                                                                                                   |
| HEADSET_FINALDRY          | Float | -70..10          | Final dry mix (dB)                                                                                                                                   |
| HEADSET_REFLECTION_FACTOR | Float | 0.5..2.5         | Early reflection factor                                                                                                                              |
| HEADSET_REFLECTION_WIDTH  | Float | -1..1            | Early reflection width                                                                                                                               |
| HEADSET_WIDTH             | Float | 0..1             | Width of reverb L/R mix                                                                                                                              |
| HEADSET_WET               | Float | -70..10          | Reverb wetness (dB)                                                                                                                                  |
| HEADSET_LFO_WANDER        | Float | 0.1..0.6         | LFO wander amount                                                                                                                                    |
| HEADSET_BASSBOOST         | Float | 0..0.5           | Bass Boost                                                                                                                                           |
| HEADSET_LFO_SPIN          | Float | 0..10            | LFO spin amount                                                                                                                                      |
| HEADSET_DECAY             | Float | 0.1..30          | Time decay                                                                                                                                           |
| HEADSET_DELAY             | Int   | -500..500        | Delay in milliseconds                                                                                                                                |
| HEADSET_LPF_INPUT         | Int   | 200..18000       | Lowpass cutoff for input (Hz)                                                                                                                        |
| HEADSET_LPF_BASS          | Int   | 50..1050         | Lowpass cutoff for bass (Hz)                                                                                                                         |
| HEADSET_LPF_DAMP          | Int   | 200..18000       | Lowpass cutoff for dampening (Hz)                                                                                                                    |
| HEADSET_LPF_OUTPUT        | Int   | 200..18000       | Lowpass cutoff for output (Hz)                                                                                                                       |
| STEREOWIDE_ENABLE         | Bool  | 0..1             | Enable stereo widener                                                                                                                                |
| STEREOWIDE_MCOEFF         | Int   | 0..10000         | Stereo widener M Coeff (1000=1)                                                                                                                      |
| STEREOWIDE_SCOEFF         | Int   | 0..10000         | Stereo widener S Coeff (1000=1)                                                                                                                      |
| BS2B_ENABLE               | Bool  | 0..1             | Enable BS2B                                                                                                                                          |
| BS2B_FCUT                 | Int   | 300..2000        | BS2B cutoff frequency (Hz)                                                                                                                           |
| BS2B_FEED                 | Int   | 10..150          | BS2B crossfeeding level at low frequencies (10=1dB)                                                                                                  |
| COMPRESSOR_ENABLE         | Bool  | 0..1             | Enable Compressor                                                                                                                                    |
| COMPRESSOR_PREGAIN        | Int   | 0..24            | Compressor pregain (dB)                                                                                                                              |
| COMPRESSOR_THRESHOLD      | Int   | -80..0           | Compressor threshold (dB)                                                                                                                            |
| COMPRESSOR_KNEE           | Int   | 0..40            | Compressor knee (dB)                                                                                                                                 |
| COMPRESSOR_RATIO          | Int   | -20..20          | Compressor ratio (1:xx)                                                                                                                              |
| COMPRESSOR_ATTACK         | Int   | 1..1000          | Compressor attack (ms)                                                                                                                               |
| COMPRESSOR_RELEASE        | Int   | 1..1000          | Compressor release (ms)                                                                                                                              |
| TONE_ENABLE               | Bool  | 0..1             | Enable Equalizer                                                                                                                                     |
| TONE_FILTERTYPE           | Int   | 0..1             | Equalizer filter type (Minimum/Linear phase)                                                                                                         |
| TONE_EQ                   | Str   | 1200;50;-200;... | 15-band EQ data, 100=1dB; min: -12dB, max: 12dB                                                                                                      |
| MASTER_LIMTHRESHOLD       | Float | -60..0           | Limiter threshold (dB)                                                                                                                               |
| MASTER_LIMRELEASE         | Float | 1.5..2000        | Limiter release (ms)                                                                                                                                 |
| DDC_ENABLE                | Bool  | 0..1             | Enable DDC                                                                                                                                           |
| DDC_COEFFS                | Str   | file.vdc         | DDC filepath                                                                                                                                         |
| CONVOLVER_ENABLE          | Bool  | 0..1             | Enable Convolver                                                                                                                                     |
| CONVOLVER_BENCH_C0        | Str   |                  | Benchmark data for the convolver                                                                                                                     |
| CONVOLVER_BENCH_C1        | Str   |                  | Benchmark data for the convolver                                                                                                                     |
| CONVOLVER_GAIN            | Float | -80..30          | Convolver gain (dB)                                                                                                                                  |
| CONVOLVER_FILE            | Str   | file.irs/wav     | Impulse response file                                                                                                                                |
| IIR_ENABLE                | Bool  | 0..1             | Enable IIR filter                                                                                                                                    |
| IIR_FILTER                | Int   | 0.11             | 0:`LPF`,1:`HPF`,2:`BPCSGF`,3:`BPZPGF`,4:`APF`,5:`NOTCH`,6:`RIAA_phono`,7:`PEQ`,8:`BBOOST`<br />9:`LSH`,10:`RIAA_CD`,11:`HSH` |
| IIR_FREQ                  | Int   | 0..40000         | Filter frequency (db)                                                                                                                                |
| IIR_GAIN                  | Int   | -100..100        | Filter gain                                                                                                                                          |
| IIR_QFACT                 | Float | 0.0...4.0        | Filter Qfactor                                                                                                                                       |
