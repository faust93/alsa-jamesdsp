#ifndef __JDSPFX_H__
#define __JDSPFX_H__

#include "EffectDSPMain.h"

#define TRUE  1
#define FALSE 0

#define CMD_QUEUE_LEN 20

enum {
    PROP_0,

    /* global enable */
    PROP_FX_ENABLE,
    /* analog modelling */
    PROP_TUBE_ENABLE,
    PROP_TUBE_DRIVE,
    /* bassboost */
    PROP_BASS_ENABLE,
    PROP_BASS_MODE,
    PROP_BASS_FILTERTYPE,
    PROP_BASS_FREQ,
    /* reverb */
    PROP_HEADSET_ENABLE,
    PROP_HEADSET_OSF,
    PROP_HEADSET_REFLECTION_AMOUNT,
    PROP_HEADSET_FINALWET,
    PROP_HEADSET_FINALDRY,
    PROP_HEADSET_REFLECTION_FACTOR,
    PROP_HEADSET_REFLECTION_WIDTH,
    PROP_HEADSET_WIDTH,
    PROP_HEADSET_WET,
    PROP_HEADSET_LFO_WANDER,
    PROP_HEADSET_BASSBOOST,
    PROP_HEADSET_LFO_SPIN,
    PROP_HEADSET_LPF_INPUT,
    PROP_HEADSET_LPF_BASS,
    PROP_HEADSET_LPF_DAMP,
    PROP_HEADSET_LPF_OUTPUT,
    PROP_HEADSET_DECAY,
    PROP_HEADSET_DELAY,
   /* stereo wide */
    PROP_STEREOWIDE_MCOEFF,
    PROP_STEREOWIDE_SCOEFF,
    PROP_STEREOWIDE_ENABLE,
    /* bs2b */
    PROP_BS2B_FCUT,
    PROP_BS2B_FEED,
    PROP_BS2B_ENABLE,
    /* compressor */
    PROP_COMPRESSOR_ENABLE,
    PROP_COMPRESSOR_PREGAIN,
    PROP_COMPRESSOR_THRESHOLD,
    PROP_COMPRESSOR_KNEE,
    PROP_COMPRESSOR_RATIO,
    PROP_COMPRESSOR_ATTACK,
    PROP_COMPRESSOR_RELEASE,
    /* mixed equalizer */
    PROP_TONE_ENABLE,
    PROP_TONE_FILTERTYPE,
    PROP_TONE_EQ,
    /* limiter */
    PROP_MASTER_LIMTHRESHOLD,
    PROP_MASTER_LIMRELEASE,
    /* ddc */
    PROP_DDC_ENABLE,
    PROP_DDC_COEFFS,
    /* convolver */
    PROP_CONVOLVER_ENABLE,
    PROP_CONVOLVER_GAIN,
    PROP_CONVOLVER_BENCH_C0,
    PROP_CONVOLVER_BENCH_C1,
    PROP_CONVOLVER_FILE,
    /* IIR */
    PROP_IIR_ENABLE,
    PROP_IIR_FILTER,
    PROP_IIR_FREQ,
    PROP_IIR_GAIN,
    PROP_IIR_QFACT,
    /* spectrumextend */
    PROP_SE_ENABLE,
    PROP_SE_EXCITER,
    PROP_SE_REFREQ
};

typedef struct jdsp_param_s {
    bool pUpdate = false;
    int  param;
    union {
        float      f32;
        int32_t    i32;
        int16_t    i16;
        int8_t     i8;
        char       str[128];
    };
} jdsp_param_t;

enum formats{
    notready = 0x0,
    s16le,
    f32le,
    s32le,
    other
};

typedef struct snd_pcm_jdspfx {
    snd_pcm_extplug_t ext;

    bool init_done = false;
    
    pthread_t ctl_thread;
    pthread_mutex_t lock;
    bool ctl_thread_running = false;

    char ctl_fifo_path[129];
    char settings_path[129];

    jdsp_param_t *pCtl;
    uint32_t pCtlQidx = 0;
    bool pCtl_commit = false;

    uint32_t samplerate = 0;
    uint32_t format = 0;
    uint32_t channels = 2;

    /* properties */
    // global enable
    int8_t fx_enabled;

    // analog remodelling
    int8_t tube_enabled;
    int32_t tube_drive;

    // bass boost
    int8_t bass_enabled;
    int32_t bass_mode;
    int32_t bass_filtertype;
    int32_t bass_freq;

    // reverb
    int8_t headset_enabled;
    int32_t headset_osf;
    int32_t headset_delay;
    int32_t headset_inputlpf;
    int32_t headset_basslpf;
    int32_t headset_damplpf;
    int32_t headset_outputlpf;

    float_t headset_reflection_amount;
    float_t headset_reflection_factor;
    float_t headset_reflection_width;
    float_t headset_finaldry;
    float_t headset_finalwet;
    float_t headset_width;
    float_t headset_wet;
    float_t headset_lfo_wander;
    float_t headset_bassboost;
    float_t headset_lfo_spin;
    float_t headset_decay;

    // stereo wide
    int8_t stereowide_enabled;
    int32_t stereowide_mcoeff;
    int32_t stereowide_scoeff;

    // bs2b
    int8_t bs2b_enabled;
    int32_t bs2b_fcut;
    int32_t bs2b_feed;

    // compressor
    int8_t compression_enabled;
    int32_t compression_pregain;
    int32_t compression_threshold;
    int32_t compression_knee;
    int32_t compression_ratio;
    int32_t compression_attack;
    int32_t compression_release;

    // mixed equalizer
    int8_t tone_enabled;
    int32_t tone_filtertype;
    char tone_eq[100];

    // master
    float_t lim_threshold;
    float_t lim_release;

    // ddc
    int8_t ddc_enabled;
    char ddc_coeffs[4096];

    // convolver
    int8_t convolver_enabled;
    int32_t convolver_quality;
    float_t convolver_gain;
    char convolver_bench_c0[128];
    char convolver_bench_c1[128];
    char convolver_file[4096];

    // iir
    int8_t iir_enabled;
    int8_t iir_filter;
    int32_t iir_freq;
    int32_t iir_gain;
    float_t iir_qfact;

    // VFX_RE SpectrumExtend
    int8_t se_enabled;
    float_t se_exciter;
    int32_t se_refreq;

    audio_buffer_t *in;
    audio_buffer_t *out;

    /* < private > */
    EffectDSPMain *effectDspMain;
} snd_pcm_jdspfx_t;

#define LINEBUF 256
#define ARGBUF 64
#define VALBUF 128

int jdsp_cfg_write(snd_pcm_jdspfx_t *self){
    FILE *fn = fopen(self->settings_path,"w");
    if (!fn)
       return -1;
    fprintf(fn, "FX_ENABLE=%d\n", self->fx_enabled);
    fprintf(fn, "TUBE_ENABLE=%d\n", self->tube_enabled);
    fprintf(fn, "TUBE_DRIVE=%d\n", self->tube_drive);
    fprintf(fn, "BASS_ENABLE=%d\n", self->bass_enabled);
    fprintf(fn, "BASS_MODE=%d\n", self->bass_mode);
    fprintf(fn, "BASS_FILTERTYPE=%d\n", self->bass_filtertype);
    fprintf(fn, "BASS_FREQ=%d\n", self->bass_freq);
    fprintf(fn, "STEREOWIDE_ENABLE=%d\n", self->stereowide_enabled);
    fprintf(fn, "STEREOWIDE_MCOEFF=%d\n", self->stereowide_mcoeff);
    fprintf(fn, "STEREOWIDE_SCOEFF=%d\n", self->stereowide_scoeff);
    fprintf(fn, "BS2B_ENABLE=%d\n", self->bs2b_enabled);
    fprintf(fn, "BS2B_FCUT=%d\n", self->bs2b_fcut);
    fprintf(fn, "BS2B_FEED=%d\n", self->bs2b_feed);
    fprintf(fn, "COMPRESSOR_ENABLE=%d\n", self->compression_enabled);
    fprintf(fn, "COMPRESSOR_PREGAIN=%d\n", self->compression_pregain);
    fprintf(fn, "COMPRESSOR_THRESHOLD=%d\n", self->compression_threshold);
    fprintf(fn, "COMPRESSOR_KNEE=%d\n", self->compression_knee);
    fprintf(fn, "COMPRESSOR_RATIO=%d\n", self->compression_ratio);
    fprintf(fn, "COMPRESSOR_ATTACK=%d\n", self->compression_attack);
    fprintf(fn, "COMPRESSOR_RELEASE=%d\n", self->compression_release);
    fprintf(fn, "TONE_ENABLE=%d\n", self->tone_enabled);
    fprintf(fn, "TONE_FILTERTYPE=%d\n", self->tone_filtertype);
    fprintf(fn, "TONE_EQ=%s\n", self->tone_eq);
    fprintf(fn, "MASTER_LIMTHRESHOLD=%f\n", self->lim_threshold);
    fprintf(fn, "MASTER_LIMRELEASE=%f\n", self->lim_release);
    fprintf(fn, "DDC_ENABLE=%d\n", self->ddc_enabled);
    fprintf(fn, "DDC_COEFFS=%s\n", self->ddc_coeffs);
    fprintf(fn, "CONVOLVER_ENABLE=%d\n", self->convolver_enabled);
    fprintf(fn, "CONVOLVER_FILE=%s\n", self->convolver_file);
    fprintf(fn, "CONVOLVER_GAIN=%f\n", self->convolver_gain);
    fprintf(fn, "CONVOLVER_BENCH_C0=%s\n", self->convolver_bench_c0);
    fprintf(fn, "CONVOLVER_BENCH_C1=%s\n", self->convolver_bench_c1);
    fprintf(fn, "HEADSET_ENABLE=%d\n", self->headset_enabled);
    fprintf(fn, "HEADSET_OSF=%d\n", self->headset_osf);
    fprintf(fn, "HEADSET_REFLECTION_AMOUNT=%f\n", self->headset_reflection_amount);
    fprintf(fn, "HEADSET_FINALWET=%f\n", self->headset_finalwet);
    fprintf(fn, "HEADSET_FINALDRY=%f\n", self->headset_finaldry);
    fprintf(fn, "HEADSET_REFLECTION_FACTOR=%f\n", self->headset_reflection_factor);
    fprintf(fn, "HEADSET_REFLECTION_WIDTH=%f\n", self->headset_reflection_width);
    fprintf(fn, "HEADSET_WIDTH=%f\n", self->headset_width);
    fprintf(fn, "HEADSET_WET=%f\n", self->headset_wet);
    fprintf(fn, "HEADSET_LFO_WANDER=%f\n", self->headset_lfo_wander);
    fprintf(fn, "HEADSET_BASSBOOST=%f\n", self->headset_bassboost);
    fprintf(fn, "HEADSET_LFO_SPIN=%f\n", self->headset_lfo_spin);
    fprintf(fn, "HEADSET_DECAY=%f\n", self->headset_decay);
    fprintf(fn, "HEADSET_DELAY=%d\n", self->headset_delay);
    fprintf(fn, "HEADSET_LPF_INPUT=%d\n", self->headset_inputlpf);
    fprintf(fn, "HEADSET_LPF_BASS=%d\n", self->headset_basslpf);
    fprintf(fn, "HEADSET_LPF_DAMP=%d\n", self->headset_damplpf);
    fprintf(fn, "HEADSET_LPF_OUTPUT=%d\n", self->headset_outputlpf);
    fprintf(fn, "IIR_ENABLE=%d\n", self->iir_enabled);
    fprintf(fn, "IIR_FILTER=%d\n", self->iir_filter);
    fprintf(fn, "IIR_FREQ=%d\n", self->iir_freq);
    fprintf(fn, "IIR_GAIN=%d\n", self->iir_gain);
    fprintf(fn, "IIR_QFACT=%f\n", self->iir_qfact);
    fprintf(fn, "SE_ENABLE=%d\n", self->se_enabled);
    fprintf(fn, "SE_EXCITER=%f\n", self->se_exciter);
    fprintf(fn, "SE_REFREQ=%d\n", self->se_refreq);
    fclose(fn);
    return 0;
}

int jdsp_cfg_read(snd_pcm_jdspfx_t *self) {
    FILE *fn;
    char line[LINEBUF];
    char val[VALBUF];
    char param[ARGBUF];
    int lines = 0;

    fn = fopen(self->settings_path,"r");
    if (!fn)
       return -1;
    while (fgets(line, LINEBUF, fn) != NULL) {
        if(line[0] == '#' || line[0] == ';') continue;
        if((strlen(line))>10) {
            char *i = strchr(line,'=');
            if(i) {
                memset(param, 0, ARGBUF);
                memset(val, 0, VALBUF);
                strncpy(param, line, (i-line));
                strcpy (val, line+(i-line)+1);
                if(!strcmp(param, "FX_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->fx_enabled = v;
                    } else {
                        printf("FX_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "TUBE_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->tube_enabled = v;
                    } else {
                        printf("TUBE_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "TUBE_DRIVE")) {
                        int16_t v = atoi(val);
                       if(v >= 0 && v <= 12000) {
                        self->tube_drive = v;
                    } else {
                        printf("TUBE_DRIVE value out of range. Accepted values are: [0-12000]\n");
                    }
                }
                else if(!strcmp(param, "BASS_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->bass_enabled = v;
                    } else {
                        printf("BASS_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "BASS_MODE")) {
                        int16_t v = atoi(val);
                       if(v >= 0 && v <= 3000) {
                        self->bass_mode = v;
                    } else {
                        printf("BASS_MODE value out of range. Accepted values are: [0-3000]\n");
                    }
                }
                else if(!strcmp(param, "BASS_FILTERTYPE")) {
                      int16_t v = atoi(val);
                      if(v >= 0 || v <= 1) {
                        self->bass_filtertype = v;
                       } else {
                            printf("BASS_FILTERTYPE value out of range. Accepted values are: [0|1]\n");
                       }
                }
                else if(!strcmp(param, "BASS_FREQ")) {
                     int16_t v = atoi(val);
                     if(v >= 30 && v <= 300) {
                        self->bass_freq = v;
                    } else {
                          printf("BASS_FREQ value out of range. Accepted values are: [30-300]\n");
                    }
                }
                else if(!strcmp(param, "STEREOWIDE_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->stereowide_enabled = v;
                    } else {
                        printf("STEREOWIDE_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "STEREOWIDE_MCOEFF")) {
                    int16_t v = atoi(val);
                    if(v >= 0 && v <= 10000) {
                        self->stereowide_mcoeff = v;
                    } else {
                        printf("STEREOWIDE_MCOEFF value out of range. Accepted values are: [0-10000]\n");
                    }
                }
                else if(!strcmp(param, "STEREOWIDE_SCOEFF")) {
                    int16_t v = atoi(val);
                    if(v >= 0 && v <= 10000) {
                        self->stereowide_scoeff = v;
                    } else {
                        printf("STEREOWIDE_SCOEFF value out of range. Accepted values are: [0-10000]\n");
                    }
                }
                else if(!strcmp(param, "BS2B_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->bs2b_enabled = v;
                    } else {
                        printf("BS2B_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "BS2B_FCUT")) {
                    int16_t v = atoi(val);
                    if(v >= 300 && v <= 2000) {
                        self->bs2b_fcut = v;
                    } else {
                        printf("BS2B_FCUT value out of range. Accepted values are: [300-2000]\n");
                    }
                }
                else if(!strcmp(param, "BS2B_FEED")) {
                    int16_t v = atoi(val);
                    if(v >= 10 && v <= 150) {
                        self->bs2b_feed = v;
                    } else {
                        printf("BS2B_FEED value out of range. Accepted values are: [10-150]\n");
                    }
                }
                else if(!strcmp(param, "COMPRESSOR_ENABLE")) {
                   int8_t v = atoi(val);
                   if(v == 0 || v == 1) {
                        self->compression_enabled = v;
                    } else {
                        printf("COMPRESSOR_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "COMPRESSOR_PREGAIN")) {
                   int16_t v = atoi(val);
                   if(v >= 0 && v <= 24) {
                        self->compression_pregain = v;
                    } else {
                        printf("COMPRESSOR_PREGAIN value out of range. Accepted values are: [0-24]\n");
                    }
                }
                else if(!strcmp(param, "COMPRESSOR_THRESHOLD")) {
                   int16_t v = atoi(val);
                   if(v >= -80 && v <= 0) {
                        self->compression_threshold = v;
                    } else {
                        printf("COMPRESSOR_THRESHOLD value out of range. Accepted values are: [-80 - 0]\n");
                    }
                }
                else if(!strcmp(param, "COMPRESSOR_KNEE")) {
                   int16_t v = atoi(val);
                   if(v >= 0 && v <= 40) {
                        self->compression_knee = v;
                    } else {
                        printf("COMPRESSOR_KNEE value out of range. Accepted values are: [0-40]\n");
                    }
                }
                else if(!strcmp(param, "COMPRESSOR_RATIO")) {
                   int16_t v = atoi(val);
                   if(v >= -20 && v <= 20) {
                        self->compression_ratio = v;
                    } else {
                        printf("COMPRESSOR_RATIO value out of range. Accepted values are: [-20 - 20]\n");
                    }
                }
                else if(!strcmp(param, "COMPRESSOR_ATTACK")) {
                   int16_t v = atoi(val);
                   if(v >= 1 && v <= 1000) {
                        self->compression_attack = v;
                    } else {
                        printf("COMPRESSOR_ATTACK value out of range. Accepted values are: [1-1000]\n");
                    }
                }
                else if(!strcmp(param, "COMPRESSOR_RELEASE")) {
                   int16_t v = atoi(val);
                   if(v >= 1 && v <= 1000) {
                        self->compression_release = v;
                    } else {
                        printf("COMPRESSOR_RELEASE value out of range. Accepted values are: [1-1000]\n");
                    }
                }
                else if(!strcmp(param, "TONE_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->tone_enabled = v;
                     } else {
                         printf("TONE_ENABLE value out of range. Accepted values are: [0|1]\n");
                     }
                 }
                 else if(!strcmp(param, "TONE_FILTERTYPE")) {
                    int16_t v = atoi(val);
                    if(v >= 0 && v <= 1) {
                        self->tone_filtertype = v;
                     } else {
                         printf("TONE_FILTERTYPE value out of range. Accepted values are: [0-1]\n");
                     }
                 }
                 else if(!strcmp(param, "TONE_EQ")) {
                     if (strlen(val) < 100) {
                         memset(self->tone_eq, 0, sizeof(self->tone_eq));
                         strncpy(self->tone_eq, val, strlen(val)-1);
                     } else {
                         printf("TONE_EQ value out of range. Accepted values are: [0;0;0;0;0;0;0;0;0;0;0;0;0;0;0]\n");
                     }
                 }
                 else if(!strcmp(param, "MASTER_LIMTHRESHOLD")) {
                    float_t v = atof(val);
                    if(v >= -60 && v <= 0) {
                        self->lim_threshold = v;
                     } else {
                         printf("MASTER_LIMTHRESHOLD value out of range. Accepted values are: [-60-0] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "MASTER_LIMRELEASE")) {
                    float_t v = atof(val);
                    if(v >= 1.5 && v <= 2000) {
                        self->lim_release = v;
                     } else {
                         printf("MASTER_LIMRELEASE value out of range. Accepted values are: [1.5-2000] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "DDC_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->ddc_enabled = v;
                     } else {
                         printf("DDC_ENABLE vlue out of range. Accepted values are: [0|1]\n");
                     }
                 }
                 else if(!strcmp(param, "DDC_COEFFS")) {
                     if (strlen(val) > 0 && strlen(val) < 4096) {
                         memset(self->ddc_coeffs, 0, sizeof(self->ddc_coeffs));
                         strncpy(self->ddc_coeffs, val, strlen(val)-1);
                     } else {
                         printf("DDC_COEFFS value out of range. Accepted values are: [/path/file.vdc]\n");
                     }
                 }
                 else if(!strcmp(param, "CONVOLVER_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->convolver_enabled = v;
                     } else {
                         printf("CONVOLVER_ENABLE value out of range. Accepted values are: [0|1]\n");
                     }
                 }
                 else if(!strcmp(param, "CONVOLVER_FILE")) {
                     if (strlen(val) > 0 && strlen(val) < 4096) {
                         memset(self->convolver_file, 0, sizeof(self->convolver_file));
                         strncpy(self->convolver_file, val, strlen(val)-1);
                     } else {
                         printf("CONVOLVER_FILE value out of range. Accepted values are: [/path/file.irs]\n");
                     }
                 }
                 else if(!strcmp(param, "CONVOLVER_GAIN")) {
                    float_t v = atof(val);
                    if(v >= -80 && v <= 30) {
                        self->convolver_gain = v;
                     } else {
                         printf("CONVOLVER_GAIN value out of range. Accepted values are: [-80 - 30] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "CONVOLVER_BENCH_C0")) {
                     if (strlen(val) > 0 && strlen(val) < 128) {
                         memset(self->convolver_bench_c0, 0, sizeof(self->convolver_bench_c0));
                         strncpy(self->convolver_bench_c0, val, strlen(val)-1);
                     } else {
                         printf("CONVOLVER_BENCH_C0 value out of range. Accepted values are: [0.000000]\n");
                     }
                 }
                 else if(!strcmp(param, "CONVOLVER_BENCH_C1")) {
                     if (strlen(val) > 0  && strlen(val) < 128) {
                         memset(self->convolver_bench_c1, 0, sizeof(self->convolver_bench_c1));
                         strncpy(self->convolver_bench_c1, val, strlen(val)-1);
                     } else {
                         printf("CONVOLVER_BENCH_C1 value out of range. Accepted values are: [0.000000]\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->headset_enabled = v;
                     } else {
                         printf("HEADSET_ENABLE value out of range. Accepted values are: [0|1]\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_OSF")) {
                    int16_t v = atoi(val);
                    if(v >= 0 && v <= 4) {
                        self->headset_osf = v;
                     } else {
                         printf("HEADSET_OSF value out of range. Accepted values are: [0-4]\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_REFLECTION_AMOUNT")) {
                    float_t v = atof(val);
                    if(v >= 0 && v <= 1) {
                        self->headset_reflection_amount = v;
                     } else {
                         printf("HEADSET_REFLECTION_AMOUNT value out of range. Accepted values are: [0.0-1.0] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_FINALWET")) {
                    float_t v = atof(val);
                    if(v >= -70 && v <= 10) {
                        self->headset_finalwet = v;
                     } else {
                         printf("HEADSET_FINALWET value out of range. Accepted values are: [-70 - 10] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_FINALDRY")) {
                    float_t v = atof(val);
                    if(v >= -70 && v <= 10) {
                        self->headset_finaldry = v;
                     } else {
                         printf("HEADSET_FINALDRY value out of range. Accepted values are: [-70 - 10] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_REFLECTION_FACTOR")) {
                    float_t v = atof(val);
                    if(v >= 0.5 && v <= 2.5) {
                        self->headset_reflection_factor = v;
                     } else {
                         printf("HEADSET_REFLECTION_FACTOR value out of range. Accepted values are: [0.5-2.5] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_REFLECTION_WIDTH")) {
                    float_t v = atof(val);
                    if(v >= -1 && v <= 1) {
                        self->headset_reflection_width = v;
                     } else {
                         printf("HEADSET_REFLECTION_WIDTH value out of range. Accepted values are: [-1 - 1] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_WIDTH")) {
                    float_t v = atof(val);
                    if(v >= 0 && v <= 1) {
                        self->headset_width = v;
                     } else {
                         printf("HEADSET_WIDTH value out of range. Accepted values are: [0-1] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_WET")) {
                    float_t v = atof(val);
                    if(v >= -70 && v <= 10) {
                        self->headset_wet = v;
                     } else {
                         printf("HEADSET_WET value out of range. Accepted values are: [-70 - 10] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_LFO_WANDER")) {
                    float_t v = atof(val);
                    if(v >= 0.1 && v <= 0.6) {
                        self->headset_lfo_wander = v;
                     } else {
                         printf("HEADSET_LFO_WANDER value out of range. Accepted values are: [0.1 - 0.6] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_BASSBOOST")) {
                    float_t v = atof(val);
                    if(v >= 0 && v <= 0.5) {
                        self->headset_bassboost = v;
                     } else {
                         printf("HEADSET_BASSBOOST value out of range. Accepted values are: [0-0.5] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_LFO_SPIN")) {
                    float_t v = atof(val);
                    if(v >= 0 && v <= 10) {
                        self->headset_lfo_spin = v;
                     } else {
                         printf("HEADSET_LFO_SPIN value out of range. Accepted values are: [0-10] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_DECAY")) {
                    float_t v = atof(val);
                    if(v >= 0.1 && v <= 30) {
                        self->headset_decay = v;
                     } else {
                         printf("HEADSET_DECAY value out of range. Accepted values are: [0.1-30] (float)\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_DELAY")) {
                    int16_t v = atoi(val);
                    if(v >= -500 && v <= 500) {
                        self->headset_delay = v;
                     } else {
                         printf("HEADSET_DELAY value out of range. Accepted values are: [-500 - 500]\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_LPF_INPUT")) {
                    int16_t v = atoi(val);
                    if(v >= 200 && v <= 18000) {
                        self->headset_inputlpf = v;
                     } else {
                         printf("HEADSET_LPF_INPUT value out of range. Accepted values are: [200-18000]\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_LPF_BASS")) {
                    int16_t v = atoi(val);
                    if(v >= 50 && v <= 1050) {
                        self->headset_basslpf = v;
                     } else {
                         printf("HEADSET_LPF_BASS value out of range. Accepted values are: [50-1050]\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_LPF_DAMP")) {
                    int16_t v = atoi(val);
                    if(v >=200 && v <= 18000) {
                        self->headset_damplpf = v;
                     } else {
                         printf("HEADSET_LPF_DAMP value out of range. Accepted values are: [200-18000]\n");
                     }
                 }
                 else if(!strcmp(param, "HEADSET_LPF_OUTPUT")) {
                    int16_t v = atoi(val);
                    if(v >=200 && v <= 18000) {
                        self->headset_outputlpf = v;
                     } else {
                         printf("HEADSET_LPF_OUTPUT value out of range. Accepted values are: [200-18000]\n");
                     }
                 }
                else if(!strcmp(param, "IIR_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->iir_enabled = v;
                    } else {
                        printf("IIR_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "IIR_FILTER")) {
                    int16_t v = atoi(val);
                    if(v >= 0 && v <= 11) {
                        self->iir_filter = v;
                    } else {
                        printf("IIR_FILTER value out of range. Accepted values are: [0-11]\n");
                    }
                }
                else if(!strcmp(param, "IIR_FREQ")) {
                    int16_t v = atoi(val);
                    if(v >= 0 && v <= 40000) {
                        self->iir_freq = v;
                    } else {
                        printf("IIR_FREQ value out of range. Accepted values are: [0-40000]\n");
                    }
                }
                else if(!strcmp(param, "IIR_GAIN")) {
                    int16_t v = atoi(val);
                    if(v >= -100 && v <= 100) {
                        self->iir_gain = v;
                    } else {
                        printf("IIR_GAIN value out of range. Accepted values are: [-100-100]\n");
                    }
                }
                else if(!strcmp(param, "IIR_QFACT")) {
                    float_t v = atof(val);
                    if(v >= 0.0 && v <= 4.0) {
                        self->iir_qfact = v;
                    } else {
                        printf("IIR_QFACT value out of range. Accepted values are: [0.0000-4.0000]\n");
                    }
                }
                else if(!strcmp(param, "SE_ENABLE")) {
                    int8_t v = atoi(val);
                    if(v == 0 || v == 1) {
                        self->se_enabled = v;
                    } else {
                        printf("SE_ENABLE value out of range. Accepted values are: [0|1]\n");
                    }
                }
                else if(!strcmp(param, "SE_REFREQ")) {
                    int16_t v = atoi(val);
                    if(v >= 0 && v <= 24000) {
                        self->se_refreq = v;
                    } else {
                        printf("SE_REFREQ value out of range. Accepted values are: [0-24000]\n");
                    }
                }
                else if(!strcmp(param, "SE_EXCITER")) {
                    float_t v = atof(val);
                    if(v >= 0.0 && v <= 50.0) {
                        self->se_exciter = v;
                    } else {
                        printf("SE_EXCITER value out of range. Accepted values are: [0.0000-50.0000]\n");
                    }
                }
                lines++;
            }
        }
    }
    fclose(fn);
    return lines;
}

#endif
