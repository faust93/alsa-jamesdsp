/*
 * ALSA-JDSPFX
 * v0.1 faust93 at <monumentum@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */


#include <stdio.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <alsa/pcm_external.h>
#include <alsa/control.h>
#include <linux/soundcard.h>
#include <pthread.h>
#include <sys/stat.h>

#include "global.h"
#include "dspinterface.h"
#include "jdspfx.h"
#include "Effect.h"
#include "EffectDSPMain.h"

#define CTL_BUFFSIZE 256
#define MAX_ARG 129
#define MAX_VAL 65

extern "C" {

/* JDSP control thread */
void *ctl_thread_loop(void *self)
{
    snd_pcm_jdspfx_t *jdsp = (snd_pcm_jdspfx_t *)self;
#ifdef DEBUG
    printf("%s start\n", __func__);
#endif
    int fd, n;
    char ctl_buf[CTL_BUFFSIZE];
    char val[MAX_VAL];
    char param[MAX_ARG];

    mkfifo(jdsp->ctl_fifo_path, 0666);
    if ( (fd = open(jdsp->ctl_fifo_path, O_RDWR)) < 0) {
        SNDERR("%s: Unable to create JDSP control pipe: %s\n", __func__, jdsp->ctl_fifo_path);
        goto ct_terminate;
    }
    do {
        while( (n = read(fd, ctl_buf, CTL_BUFFSIZE) ) > 0) {
            if((strlen(ctl_buf)) > 6) {
                char *i = strchr(ctl_buf,'=');
                if(i) {
                    memset(param, 0, MAX_ARG);
                    memset(val, 0, MAX_VAL);
                    strncpy(param, ctl_buf, (i-ctl_buf));
                    strcpy(val, ctl_buf+(i-ctl_buf)+1);
                    val[strcspn(val,"\r\n=")] = 0;
#ifdef DEBUG
                    printf("param: %s val: %s\n", param, val);
#endif
                    if(!strcmp(param, "FX_ENABLE")) {
                        int8_t v = atoi(val);
                        if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_FX_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "TUBE_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_TUBE_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "TUBE_DRIVE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 12000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_TUBE_DRIVE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-12000]");
                        }
                    }
                    else if(!strcmp(param, "BASS_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_BASS_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "BASS_MODE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 3000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_BASS_MODE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-3000]");
                        }
                    }
                    else if(!strcmp(param, "BASS_FILTERTYPE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 || v <= 1) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_BASS_FILTERTYPE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "BASS_FREQ")) {
                       int16_t v = atoi(val);
                       if(v >= 30 && v <= 300) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_BASS_FREQ;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [30-300]");
                        }
                    }
                    else if(!strcmp(param, "STEREOWIDE_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_STEREOWIDE_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "STEREOWIDE_MCOEFF")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 10000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_STEREOWIDE_MCOEFF;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-10000]");
                        }
                    }
                    else if(!strcmp(param, "STEREOWIDE_SCOEFF")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 10000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_STEREOWIDE_SCOEFF;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-10000]");
                        }
                    }
                    else if(!strcmp(param, "BS2B_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_BS2B_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "BS2B_FCUT")) {
                       int16_t v = atoi(val);
                       if(v >= 300 && v <= 2000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_BS2B_FCUT;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [300-2000]");
                        }
                    }
                    else if(!strcmp(param, "BS2B_FEED")) {
                       int16_t v = atoi(val);
                       if(v >= 10 && v <= 150) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_BS2B_FEED;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [10-150]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_COMPRESSOR_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_PREGAIN")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 24) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_COMPRESSOR_PREGAIN;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-24]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_THRESHOLD")) {
                       int16_t v = atoi(val);
                       if(v >= -80 && v <= 0) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_COMPRESSOR_THRESHOLD;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-80 - 0]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_KNEE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 40) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_COMPRESSOR_KNEE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-40]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_RATIO")) {
                       int16_t v = atoi(val);
                       if(v >= -20 && v <= 20) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_COMPRESSOR_RATIO;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-20 - 20]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_ATTACK")) {
                       int16_t v = atoi(val);
                       if(v >= 1 && v <= 1000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_COMPRESSOR_ATTACK;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [1-1000]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_RELEASE")) {
                       int16_t v = atoi(val);
                       if(v >= 1 && v <= 1000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_COMPRESSOR_RELEASE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [1-1000]");
                        }
                    }
                    else if(!strcmp(param, "TONE_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_TONE_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "TONE_FILTERTYPE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 1) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_TONE_FILTERTYPE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-1]");
                        }
                    }
                    else if(!strcmp(param, "TONE_EQ")) {
                        if (strlen(val) < 64 && strlen(val) > 0) {
                            memset(jdsp->pCtl->str, 0, sizeof(jdsp->pCtl->str));
                            strcpy(jdsp->pCtl->str, val);
                            jdsp->pCtl->param = PROP_TONE_EQ;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0;0;0;0;0;0;0;0;0;0;0;0;0;0;0]");
                        }
                    }
                    else if(!strcmp(param, "MASTER_LIMTHRESHOLD")) {
                       float_t v = atof(val);
                       if(v >= -60 && v <= 0) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_MASTER_LIMTHRESHOLD;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-60-0] (float)");
                        }
                    }
                    else if(!strcmp(param, "MASTER_LIMRELEASE")) {
                       float_t v = atof(val);
                       if(v >= 1.5 && v <= 200) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_MASTER_LIMRELEASE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [1.5-200] (float)");
                        }
                    }
                    else if(!strcmp(param, "DDC_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_DDC_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "DDC_COEFFS")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl->str, 0, sizeof(jdsp->pCtl->str));
                            strcpy(jdsp->pCtl->str, val);
                            jdsp->pCtl->param = PROP_DDC_COEFFS;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [/path/file.vdc]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_CONVOLVER_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_FILE")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl->str, 0, sizeof(jdsp->pCtl->str));
                            strcpy(jdsp->pCtl->str, val);
                            jdsp->pCtl->param = PROP_CONVOLVER_FILE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [/path/file.irs]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_GAIN")) {
                       float_t v = atof(val);
                       if(v >= -80 && v <= 30) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_CONVOLVER_GAIN;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-80 - 30] (float)");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_BENCH_C0")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl->str, 0, sizeof(jdsp->pCtl->str));
                            strcpy(jdsp->pCtl->str, val);
                            jdsp->pCtl->param = PROP_CONVOLVER_BENCH_C0;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0.000000]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_BENCH_C1")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl->str, 0, sizeof(jdsp->pCtl->str));
                            strcpy(jdsp->pCtl->str, val);
                            jdsp->pCtl->param = PROP_CONVOLVER_BENCH_C1;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0.000000]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl->i8 = v;
                            jdsp->pCtl->param = PROP_HEADSET_ENABLE;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_OSF")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 4) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_HEADSET_OSF;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-4]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_REFLECTION_AMOUNT")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 1) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_REFLECTION_AMOUNT;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0.0-1.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_FINALWET")) {
                       float_t v = atof(val);
                       if(v >= -70 && v <= 10) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_FINALWET;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-70 - 10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_FINALDRY")) {
                       float_t v = atof(val);
                       if(v >= -70 && v <= 10) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_FINALDRY;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-70 - 10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_REFLECTION_FACTOR")) {
                       float_t v = atof(val);
                       if(v >= 0.5 && v <= 2.5) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_REFLECTION_FACTOR;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0.5-2.5] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_REFLECTION_WIDTH")) {
                       float_t v = atof(val);
                       if(v >= -1 && v <= 1) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_REFLECTION_WIDTH;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-1 - 1] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_WIDTH")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 1) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_WIDTH;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-1] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_WET")) {
                       float_t v = atof(val);
                       if(v >= -70 && v <= 10) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_WET;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-70 - 10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LFO_WANDER")) {
                       float_t v = atof(val);
                       if(v >= 0.1 && v <= 0.6) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_LFO_WANDER;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0.1 - 0.6] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_BASSBOOST")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 0.5) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_BASSBOOST;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-0.5] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LFO_SPIN")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 10) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_LFO_SPIN;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0-10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_DECAY")) {
                       float_t v = atof(val);
                       if(v >= 0.1 && v <= 30) {
                            jdsp->pCtl->f32 = v;
                            jdsp->pCtl->param = PROP_HEADSET_DECAY;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [0.1-30] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_DELAY")) {
                       int16_t v = atoi(val);
                       if(v >= -500 && v <= 500) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_HEADSET_DELAY;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [-500 - 500]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_INPUT")) {
                       int16_t v = atoi(val);
                       if(v >= 200 && v <= 18000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_HEADSET_LPF_INPUT;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [200-18000]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_BASS")) {
                       int16_t v = atoi(val);
                       if(v >= 50 && v <= 1050) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_HEADSET_LPF_BASS;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [50-1050]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_DAMP")) {
                       int16_t v = atoi(val);
                       if(v >=200 && v <= 18000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_HEADSET_LPF_DAMP;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [200-18000]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_OUTPUT")) {
                       int16_t v = atoi(val);
                       if(v >=200 && v <= 18000) {
                            jdsp->pCtl->i16 = v;
                            jdsp->pCtl->param = PROP_HEADSET_LPF_OUTPUT;
                            jdsp->pCtl->pUpdate = true;
                        } else {
                            SNDERR("Value out of range. Accepted values are: [200-18000]");
                        }
                    }
                memset(ctl_buf, 0, CTL_BUFFSIZE);
                }
            }
        }
    } while ( jdsp->ctl_thread_running );
    close(fd);

ct_terminate:
#ifdef DEBUG
    printf("%s\n exit", __func__);
#endif
    pthread_detach(jdsp->ctl_thread);
    return NULL;
}

static void jdspfx_set_property(snd_pcm_jdspfx_t *self) {
    switch (self->pCtl->param) {
        case PROP_FX_ENABLE: {
            self->fx_enabled = self->pCtl->i8;
        }
            break;
        case PROP_TUBE_ENABLE: {
            self->tube_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1206, self->tube_enabled);
        }
            break;
        case PROP_TUBE_DRIVE: {
            self->tube_drive = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 150, (int16_t) self->tube_drive);
        }
            break;
        case PROP_BASS_ENABLE: {
            self->bass_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1201, self->bass_enabled);
        }
            break;
        case PROP_BASS_MODE: {
            self->bass_mode = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 112, (int16_t) self->bass_mode);
        }
            break;
        case PROP_BASS_FILTERTYPE: {
            self->bass_filtertype = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 113, (int16_t) self->bass_filtertype);
        }
            break;
        case PROP_BASS_FREQ: {
            self->bass_freq = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 114, (int16_t) self->bass_freq);
        }
            break;
        case PROP_STEREOWIDE_ENABLE: {
            self->stereowide_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1204, self->stereowide_enabled);
        }
            break;
        case PROP_STEREOWIDE_MCOEFF: {
            self->stereowide_mcoeff = self->pCtl->i16;
            command_set_px4_vx2x2(self->effectDspMain, 137, (int16_t) self->stereowide_mcoeff,self->stereowide_scoeff);
        }
            break;
        case PROP_STEREOWIDE_SCOEFF: {
            self->stereowide_scoeff = self->pCtl->i16;
            command_set_px4_vx2x2(self->effectDspMain, 137, (int16_t) self->stereowide_mcoeff,self->stereowide_scoeff);
        }
            break;
        case PROP_BS2B_ENABLE: {
            self->bs2b_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1208, self->bs2b_enabled);
        }
            break;
        case PROP_BS2B_FCUT: {
            self->bs2b_fcut = self->pCtl->i16;
            if(self->bs2b_feed != 0)
                command_set_px4_vx2x2(self->effectDspMain, 188, (int16_t)self->bs2b_fcut,(int16_t)self->bs2b_feed);
        }
            break;
        case PROP_BS2B_FEED: {
            self->bs2b_feed = self->pCtl->i16;
            if(self->bs2b_feed != 0)
                command_set_px4_vx2x2(self->effectDspMain, 188, (int16_t)self->bs2b_fcut,(int16_t)self->bs2b_feed);
        }
            break;
        case PROP_COMPRESSOR_ENABLE: {
            self->compression_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1200, self->compression_enabled);
        }
            break;
        case PROP_COMPRESSOR_PREGAIN: {
            self->compression_pregain = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 100, (int16_t) self->compression_pregain);
        }
            break;
        case PROP_COMPRESSOR_THRESHOLD: {
            self->compression_threshold  = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 101, (int16_t) self->compression_threshold);
        }
            break;
        case PROP_COMPRESSOR_KNEE: {
            self->compression_knee  = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 102, (int16_t) self->compression_knee);
        }
            break;
        case PROP_COMPRESSOR_RATIO: {
            self->compression_ratio  = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 103, (int16_t) self->compression_ratio);
        }
            break;
        case PROP_COMPRESSOR_ATTACK: {
            self->compression_attack  = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 104, (int16_t) self->compression_attack);
        }
            break;
        case PROP_COMPRESSOR_RELEASE: {
            self->compression_release  = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 105, (int16_t) self->compression_release);
        }
            break;
        case PROP_TONE_ENABLE: {
            self->tone_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1202, self->tone_enabled);
        }
            break;
        case PROP_TONE_FILTERTYPE: {
            self->tone_filtertype = self->pCtl->i16;
            command_set_px4_vx2x1(self->effectDspMain, 151, (int16_t) self->tone_filtertype);
        }
            break;
        case PROP_TONE_EQ:
        {
            if (strlen(self->pCtl->str) < 64) {
                memset (self->tone_eq, 0, sizeof(self->tone_eq));
                strcpy(self->tone_eq, self->pCtl->str);
                command_set_eq (self->effectDspMain, self->tone_eq);
            }else{
                printf("[E] EQ string too long (>64 bytes)");
            }
        }
            break;
        case PROP_MASTER_LIMTHRESHOLD:
        {
            self->lim_threshold = self->pCtl->f32;
            command_set_limiter(self->effectDspMain,self->lim_threshold,self->lim_release);
        }
            break;
        case PROP_MASTER_LIMRELEASE:
        {
            self->lim_release = self->pCtl->f32;
            command_set_limiter(self->effectDspMain,self->lim_threshold,self->lim_release);
        }
            break;
        case PROP_DDC_ENABLE: {
            self->ddc_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1212, self->ddc_enabled);
        }
            break;
        case PROP_DDC_COEFFS: {
            memset (self->ddc_coeffs, 0, sizeof(self->ddc_coeffs));
            strcpy(self->ddc_coeffs, self->pCtl->str);
            command_set_ddc(self->effectDspMain, self->ddc_coeffs,self->ddc_enabled);
        }
            break;
        case PROP_CONVOLVER_ENABLE: {
            self->convolver_enabled = self->pCtl->i8;
            command_set_convolver(self->effectDspMain, self->convolver_file,self->convolver_gain,self->convolver_quality,
                          self->convolver_bench_c0,self->convolver_bench_c1,self->samplerate);
            command_set_px4_vx2x1(self->effectDspMain, 1205, self->convolver_enabled);
        }
            break;
        case PROP_CONVOLVER_FILE: {
            memset (self->convolver_file, 0, sizeof(self->convolver_file));
            strcpy(self->convolver_file, self->pCtl->str);
        }
            break;
        case PROP_CONVOLVER_BENCH_C0: {
            memset (self->convolver_bench_c0, 0,  sizeof(self->convolver_bench_c0));
            strcpy(self->convolver_bench_c0, self->pCtl->str);
        }
            break;
        case PROP_CONVOLVER_BENCH_C1: {
            memset (self->convolver_bench_c1, 0, sizeof(self->convolver_bench_c1));
            strcpy(self->convolver_bench_c1, self->pCtl->str);
        }
            break;
        case PROP_CONVOLVER_GAIN:
        {
            self->convolver_gain = self->pCtl->f32;
        }
            break;
        case PROP_HEADSET_ENABLE: {
            self->headset_enabled = self->pCtl->i8;
            command_set_px4_vx2x1(self->effectDspMain, 1203, self->headset_enabled);
        }
            break;
        case PROP_HEADSET_OSF: {
            self->headset_osf = self->pCtl->i16;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_DELAY: {
            self->headset_delay =self->pCtl->i16;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_LPF_INPUT: {
            self->headset_inputlpf = self->pCtl->i16;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_LPF_BASS: {
            self->headset_basslpf = self->pCtl->i16;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_LPF_DAMP: {
            self->headset_damplpf = self->pCtl->i16;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_LPF_OUTPUT: {
            self->headset_outputlpf = self->pCtl->i16;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_REFLECTION_WIDTH: {
            self->headset_reflection_width = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_REFLECTION_FACTOR: {
            self->headset_reflection_factor = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_REFLECTION_AMOUNT: {
            self->headset_reflection_amount = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_FINALDRY: {
            self->headset_finaldry = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_FINALWET: {
            self->headset_finalwet = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_WIDTH: {
            self->headset_width = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_WET: {
            self->headset_wet = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_LFO_WANDER: {
            self->headset_lfo_wander = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_BASSBOOST: {
            self->headset_bassboost = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_LFO_SPIN: {
            self->headset_lfo_spin = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        case PROP_HEADSET_DECAY: {
            self->headset_decay = self->pCtl->f32;
            command_set_reverb(self->effectDspMain,self);
        }
            break;
        default:
            SNDERR("Invalid property: %d", self->pCtl->param);
            break;
    }
    self->pCtl->pUpdate = false;
}

/* sync all parameters to fx core */
static void sync_all_parameters(snd_pcm_jdspfx_t *self) {

    //self->fx_enabled = TRUE;

    config_set_px0_vx0x0(self->effectDspMain, EFFECT_CMD_ENABLE);

    // analog modelling
    command_set_px4_vx2x1(self->effectDspMain,
                          1206, (int16_t) self->tube_drive);

    command_set_px4_vx2x1(self->effectDspMain,
                          150, self->tube_enabled);

    // bassboost
    command_set_px4_vx2x1(self->effectDspMain,
                          112, (int16_t)self->bass_mode);

    command_set_px4_vx2x1(self->effectDspMain,
                          113, (int16_t)self->bass_filtertype);

    command_set_px4_vx2x1(self->effectDspMain,
                          114, (int16_t)self->bass_freq);

    command_set_px4_vx2x1(self->effectDspMain,
                          1201, self->bass_enabled);

    // reverb
    command_set_reverb(self->effectDspMain, self);

    command_set_px4_vx2x1(self->effectDspMain,
                          1203, self->headset_enabled);

    // stereo wide
    command_set_px4_vx2x2(self->effectDspMain,
                          137, (int16_t)self->stereowide_mcoeff,(int16_t)self->stereowide_scoeff);

    command_set_px4_vx2x1(self->effectDspMain,
                          1204, self->stereowide_enabled);

    // bs2b
    if(self->bs2b_feed != 0)
        command_set_px4_vx2x2(self->effectDspMain,
                          188, (int16_t)self->bs2b_fcut,(int16_t)self->bs2b_feed);

    command_set_px4_vx2x1(self->effectDspMain,
                          1208, self->bs2b_enabled);

    // compressor
    command_set_px4_vx2x1(self->effectDspMain,
                          100, (int16_t)self->compression_pregain);
    command_set_px4_vx2x1(self->effectDspMain,
                          101, (int16_t)self->compression_threshold);
    command_set_px4_vx2x1(self->effectDspMain,
                          102, (int16_t)self->compression_knee);
    command_set_px4_vx2x1(self->effectDspMain,
                          103, (int16_t)self->compression_ratio);
    command_set_px4_vx2x1(self->effectDspMain,
                          104, (int16_t)self->compression_attack);
    command_set_px4_vx2x1(self->effectDspMain,
                          105, (int16_t)self->compression_release);
    command_set_px4_vx2x1(self->effectDspMain,
                          1200, self->compression_enabled);

    // mixed equalizer
    command_set_eq (self->effectDspMain, self->tone_eq);
    command_set_px4_vx2x1(self->effectDspMain,
                          151, (int16_t)self->tone_filtertype);
    command_set_px4_vx2x1(self->effectDspMain,
                          1202, self->tone_enabled);

    // limiter
    command_set_limiter(self->effectDspMain,self->lim_threshold,self->lim_release);

    // ddc
    command_set_ddc(self->effectDspMain,self->ddc_coeffs,self->ddc_enabled);
    command_set_px4_vx2x1(self->effectDspMain,
                          1212, self->ddc_enabled);

    // convolver
    command_set_convolver(self->effectDspMain, self->convolver_file,self->convolver_gain,self->convolver_quality,
                          self->convolver_bench_c0,self->convolver_bench_c1,self->samplerate);
    command_set_px4_vx2x1(self->effectDspMain,
                          1205, self->convolver_enabled);

}

static snd_pcm_sframes_t jdsp_transfer(snd_pcm_extplug_t *ext,
    const snd_pcm_channel_area_t *dst_areas,
    snd_pcm_uframes_t dst_offset,
    const snd_pcm_channel_area_t *src_areas,
    snd_pcm_uframes_t src_offset,
    snd_pcm_uframes_t size)
{
    snd_pcm_jdspfx_t *jdsp = (snd_pcm_jdspfx_t *)ext;
    float *src, *dst;

    if(jdsp->pCtl->pUpdate) {
        pthread_mutex_lock(&jdsp->lock);
        jdspfx_set_property(jdsp);
        pthread_mutex_unlock(&jdsp->lock);
    }

    /* Calculate buffer locations */
    src = (float*)(src_areas->addr + (src_areas->first + src_areas->step * src_offset)/8);
    dst = (float*)(dst_areas->addr + (dst_areas->first + dst_areas->step * dst_offset)/8);

    if (jdsp->fx_enabled) {
        audio_buffer_t *in = (audio_buffer_t *) malloc(sizeof(audio_buffer_t));
        audio_buffer_t *out = (audio_buffer_t *) malloc(sizeof(audio_buffer_t));

        switch(jdsp->format){
            case s16le:
                in->frameCount = (size_t) size;
                in->s16 = (int16_t *)(src);
                out->frameCount = size;
                out->s16 = (int16_t *)(dst);
                jdsp->effectDspMain->process(in, out);
                break;
            case s32le:
                in->frameCount = (size_t) size;
                in->s32 = (int32_t *)(src);
                out->frameCount = size;
                out->s32 = (int32_t *)(dst);
                jdsp->effectDspMain->process(in, out);
                break;
            case f32le:
                in->frameCount = (size_t) size;
                in->f32 = (float *)(src);
                out->frameCount = size;
                out->f32 = (float*)(dst);
                jdsp->effectDspMain->process(in, out);
                break;
        }
        delete in;
        delete out;
    } else {
        memcpy(dst, src, snd_pcm_frames_to_bytes(ext->pcm, size));
    }
    return size;
}

static int jdsp_close(snd_pcm_extplug_t *ext) {
    snd_pcm_jdspfx_t *jdsp = (snd_pcm_jdspfx_t *)ext;
#ifdef DEBUG
    printf("%s\n", __func__);
#endif
    jdsp->init_done = false;
    if(jdsp->ctl_thread_running) {
        jdsp->ctl_thread_running = false;
        pthread_cancel(jdsp->ctl_thread);
        pthread_join(jdsp->ctl_thread, NULL);
    }
    free(jdsp->pCtl);

    jdsp_cfg_write(jdsp);

    EffectDSPMain *intf = jdsp->effectDspMain;
    intf->command(EFFECT_CMD_RESET, 0, NULL, NULL, NULL);
    if (jdsp->effectDspMain != NULL) {
        delete jdsp->effectDspMain;
    }
    free(jdsp);
    return 0;
}

static int jdsp_init(snd_pcm_extplug_t *ext)
{
    snd_pcm_jdspfx_t *jdsp = (snd_pcm_jdspfx_t *)ext;

    if(jdsp->fx_enabled || jdsp->init_done) {
        if(jdsp->samplerate != ext->rate) {
                if(ext->format == SND_PCM_FORMAT_S16_LE)
                    jdsp->format = s16le;
                else if(ext->format == SND_PCM_FORMAT_S32_LE)
                    jdsp->format = s32le;
                else if(ext->format == SND_PCM_FORMAT_FLOAT_LE)
                    jdsp->format = f32le;
                else
                    jdsp->format = other;
                command_set_buffercfg(jdsp->effectDspMain,jdsp->samplerate,jdsp->format);
                command_set_convolver(jdsp->effectDspMain, jdsp->convolver_file,jdsp->convolver_gain,jdsp->convolver_quality,
                                      jdsp->convolver_bench_c0,jdsp->convolver_bench_c1,jdsp->samplerate);
        }
        return 0;
    }

    /* initialize properties */
    jdsp->tube_enabled = FALSE;
    jdsp->tube_drive = 0;
    jdsp->bass_mode = 0;
    jdsp->bass_filtertype = 0;
    jdsp->bass_freq = 55;
    jdsp->bass_enabled = FALSE;
    jdsp->headset_osf=1;
    jdsp->headset_delay=0;
    jdsp->headset_inputlpf=200;
    jdsp->headset_basslpf=50;
    jdsp->headset_damplpf=200;
    jdsp->headset_outputlpf=200;
    jdsp->headset_reflection_amount=0;
    jdsp->headset_reflection_factor=0.5;
    jdsp->headset_reflection_width=0;
    jdsp->headset_finaldry=0;
    jdsp->headset_finalwet=0;
    jdsp->headset_width=0;
    jdsp->headset_wet=0;
    jdsp->headset_lfo_wander=0.1;
    jdsp->headset_bassboost=0;
    jdsp->headset_lfo_spin=0;
    jdsp->headset_decay=0.1;
    jdsp->headset_enabled = FALSE;
    jdsp->stereowide_mcoeff = 0;
    jdsp->stereowide_scoeff = 0;
    jdsp->stereowide_enabled = FALSE;
    jdsp->bs2b_enabled = FALSE;
    jdsp->bs2b_fcut = 700;
    jdsp->bs2b_feed = 10;
    jdsp->compression_pregain = 12;
    jdsp->compression_threshold = -60;
    jdsp->compression_knee = 30;
    jdsp->compression_ratio = 12;
    jdsp->compression_attack = 1;
    jdsp->compression_release = 24;
    jdsp->compression_enabled = FALSE;
    jdsp->tone_filtertype = 0;
    jdsp->tone_enabled = FALSE;
    memset(jdsp->tone_eq, 0, sizeof(jdsp->tone_eq));
    jdsp->lim_threshold = 0;
    jdsp->lim_release = 60;
    jdsp->ddc_enabled = FALSE;
    memset(jdsp->ddc_coeffs, 0, sizeof(jdsp->ddc_coeffs));
    jdsp->convolver_enabled = FALSE;
    memset(jdsp->convolver_bench_c0, 0, sizeof(jdsp->convolver_bench_c0));
    strcpy(jdsp->convolver_bench_c0 , "0.000000");
    memset(jdsp->convolver_bench_c1, 0, sizeof(jdsp->convolver_bench_c1));
    strcpy(jdsp->convolver_bench_c1 , "0.000000");
    memset(jdsp->convolver_file, 0, sizeof(jdsp->convolver_file));
    jdsp->convolver_gain = 0;
    jdsp->convolver_quality = 100;

    jdsp->pCtl = (jdsp_param_t *) malloc(sizeof(jdsp_param_t));
    jdsp->pCtl->pUpdate = false;
    jdsp->pCtl->param = 0;

    jdsp_cfg_read(jdsp);

    /* initialize private resources */
    jdsp->effectDspMain = NULL;
    jdsp->effectDspMain = new EffectDSPMain();

    jdsp->samplerate = ext->rate;
    if(ext->format == SND_PCM_FORMAT_S16_LE)
        jdsp->format = s16le;
    else if(ext->format == SND_PCM_FORMAT_S32_LE)
        jdsp->format = s32le;
    else if(ext->format == SND_PCM_FORMAT_FLOAT_LE)
        jdsp->format = f32le;
    else
        jdsp->format = other;

    if(jdsp->format == other)
        SNDERR("SAMPLE FORMAT %d NOT SUPPORTED, attempting to continue anyway...\n", jdsp->format);

    command_set_buffercfg(jdsp->effectDspMain,jdsp->samplerate,jdsp->format);
    command_set_convolver(jdsp->effectDspMain, jdsp->convolver_file,jdsp->convolver_gain,jdsp->convolver_quality,
                          jdsp->convolver_bench_c0,jdsp->convolver_bench_c1,jdsp->samplerate);

    if (jdsp->effectDspMain != NULL)
        sync_all_parameters(jdsp);

#ifdef DEBUG
    printf("\n--------INIT DONE--------\n\n");
    printf("Samplerate: %d format: %d\n", jdsp->samplerate, jdsp->format);
#endif

    jdsp->ctl_thread_running = true;
    if(pthread_create(&jdsp->ctl_thread, NULL, ctl_thread_loop, jdsp)) {
        SNDERR("%s : Error creating thread\n", __func__);
        jdsp->ctl_thread_running = false;
        return -1;
    }
    jdsp->init_done = true;
    return 0;
}


static snd_pcm_extplug_callback_t jdsp_callback = {
    .transfer = jdsp_transfer,
    .close = jdsp_close,
    .init = jdsp_init,
};

SND_PCM_PLUGIN_DEFINE_FUNC(jdspfx)
{
    snd_config_iterator_t i, next;
    snd_pcm_jdspfx_t *jdsp;
    snd_config_t *sconf = NULL;
    long channels = 2;
    const char *sFormat;
    const char *sSettings = "/tmp/jdspfx.txt";
    const char *sFIFO = "/tmp/.jdspfx.ctl";
    int dFormat = 0;
    int err;

    snd_config_for_each(i, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(i);
        const char *id;
        if (snd_config_get_id(n, &id) < 0)
            continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0)
            continue;
        if (strcmp(id, "slave") == 0) {
            sconf = n;
            continue;
        }
        if (strcmp(id, "channels") == 0) {
            snd_config_get_integer(n, &channels);
            if (channels != 2) {
                SNDERR("Only stereo streams supported");
                return -EINVAL;
            }
            continue;
        }
        if (strcmp(id, "pcm_format") == 0) {
            snd_config_get_string(n, &sFormat);
            continue;
        }
        if (strcmp(id, "settings_path") == 0) {
            snd_config_get_string(n, &sSettings);
            continue;
        }
        if (strcmp(id, "ctl_fifo_path") == 0) {
            snd_config_get_string(n, &sFIFO);
            continue;
        }
        SNDERR("Unknown field %s", id);
        return -EINVAL;
    }

    if (strcmp(sFormat, "f32") == 0) {
        dFormat = SND_PCM_FORMAT_FLOAT;
    } else if(strcmp(sFormat, "s32") == 0) {
        dFormat = SND_PCM_FORMAT_S32;
    } else if(strcmp(sFormat, "s16") == 0) {
        dFormat = SND_PCM_FORMAT_S16;
    }
#ifdef DEBUG
    printf("PCM Processing Format: %d\n",dFormat);
#endif
    if (! sconf) {
        SNDERR("No slave configuration for jdspfx pcm");
        return -EINVAL;
    }

    jdsp = (snd_pcm_jdspfx_t*)calloc(1, sizeof(*jdsp));
    if (jdsp == NULL)
        return -ENOMEM;

    jdsp->ext.version = SND_PCM_EXTPLUG_VERSION;
    jdsp->ext.name = "jdspfx";
    jdsp->ext.callback = &jdsp_callback;
    jdsp->ext.private_data = jdsp;
    jdsp->channels = channels;
    strncpy(jdsp->ctl_fifo_path, sFIFO, 128);
    strncpy(jdsp->settings_path, sSettings, 128);
    jdsp->fx_enabled = FALSE;

    err = snd_pcm_extplug_create(&jdsp->ext, name, root, sconf, stream, mode);
    if (err < 0) {
        free(jdsp);
        return err;
    }

    snd_pcm_extplug_set_param_minmax(&jdsp->ext, SND_PCM_EXTPLUG_HW_CHANNELS, channels, channels);
    snd_pcm_extplug_set_slave_param(&jdsp->ext, SND_PCM_EXTPLUG_HW_CHANNELS, channels);

    /* Set sample format explicitly if configured */
    if(dFormat != 0) {
        snd_pcm_extplug_set_param(&jdsp->ext, SND_PCM_EXTPLUG_HW_FORMAT, dFormat);
        snd_pcm_extplug_set_slave_param(&jdsp->ext, SND_PCM_EXTPLUG_HW_FORMAT, dFormat);
    }
    *pcmp = jdsp->ext.pcm;
    return 0;
}

SND_PCM_PLUGIN_SYMBOL(jdspfx);
}