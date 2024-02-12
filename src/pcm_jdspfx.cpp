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
#define MAX_VAL 129

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
            if((strlen(ctl_buf)) >= 6) {
                if(!strcmp(ctl_buf, "COMMIT") && jdsp->pCtlQidx > 0) {
#ifdef DEBUG
                    printf("%d commands commited\n", jdsp->pCtlQidx);
#endif
                    jdsp->pCtl_commit = true;
                    jdsp->pCtlQidx = 0;
                    continue;
                }
                char *i = strchr(ctl_buf,'=');
                if(i) {
                    memset(param, 0, MAX_ARG);
                    memset(val, 0, MAX_VAL);
                    strncpy(param, ctl_buf, (i-ctl_buf));
                    strcpy(val, ctl_buf+(i-ctl_buf)+1);
                    val[strcspn(val,"\r\n=")] = 0;
#ifdef DEBUG
                    printf("param: %s=%s, QCmdIdx=%d\n", param, val, jdsp->pCtlQidx);
#endif
                    if(!strcmp(param, "FX_ENABLE")) {
                        int8_t v = atoi(val);
                        if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_FX_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("FX_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "TUBE_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_TUBE_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("TUBE_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "TUBE_DRIVE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 12000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_TUBE_DRIVE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("TUBE_DRIVE value out of range. Accepted values are: [0-12000]");
                        }
                    }
                    else if(!strcmp(param, "BASS_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_BASS_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("BASS_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "BASS_MODE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 3000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_BASS_MODE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("BASS_MODE value out of range. Accepted values are: [0-3000]");
                        }
                    }
                    else if(!strcmp(param, "BASS_FILTERTYPE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 || v <= 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_BASS_FILTERTYPE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("BASS_FILTERTYPE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "BASS_FREQ")) {
                       int16_t v = atoi(val);
                       if(v >= 30 && v <= 300) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_BASS_FREQ;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("BASS_FREQ value out of range. Accepted values are: [30-300]");
                        }
                    }
                    else if(!strcmp(param, "STEREOWIDE_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_STEREOWIDE_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("STEREOWIDE_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "STEREOWIDE_MCOEFF")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 10000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_STEREOWIDE_MCOEFF;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("STEREOWIDE_MCOEFF value out of range. Accepted values are: [0-10000]");
                        }
                    }
                    else if(!strcmp(param, "STEREOWIDE_SCOEFF")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 10000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_STEREOWIDE_SCOEFF;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("STEREOWIDE_SCOEFF value out of range. Accepted values are: [0-10000]");
                        }
                    }
                    else if(!strcmp(param, "BS2B_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_BS2B_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("BS2B_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "BS2B_FCUT")) {
                       int16_t v = atoi(val);
                       if(v >= 300 && v <= 2000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_BS2B_FCUT;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("BS2B_FCUT value out of range. Accepted values are: [300-2000]");
                        }
                    }
                    else if(!strcmp(param, "BS2B_FEED")) {
                       int16_t v = atoi(val);
                       if(v >= 10 && v <= 150) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_BS2B_FEED;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("BS2B_FEED value out of range. Accepted values are: [10-150]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_COMPRESSOR_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("COMPRESSOR_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_PREGAIN")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 24) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_COMPRESSOR_PREGAIN;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("COMPRESSOR_PREGAIN value out of range. Accepted values are: [0-24]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_THRESHOLD")) {
                       int16_t v = atoi(val);
                       if(v >= -80 && v <= 0) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_COMPRESSOR_THRESHOLD;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("COMPRESSOR_THRESHOLD value out of range. Accepted values are: [-80 - 0]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_KNEE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 40) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_COMPRESSOR_KNEE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("COMPRESSOR_KNEE value out of range. Accepted values are: [0-40]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_RATIO")) {
                       int16_t v = atoi(val);
                       if(v >= -20 && v <= 20) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_COMPRESSOR_RATIO;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("COMPRESSOR_RATIO value out of range. Accepted values are: [-20 - 20]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_ATTACK")) {
                       int16_t v = atoi(val);
                       if(v >= 1 && v <= 1000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_COMPRESSOR_ATTACK;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("COMPRESSOR_ATTACK value out of range. Accepted values are: [1-1000]");
                        }
                    }
                    else if(!strcmp(param, "COMPRESSOR_RELEASE")) {
                       int16_t v = atoi(val);
                       if(v >= 1 && v <= 1000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_COMPRESSOR_RELEASE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("COMPRESSOR_RELEASE value out of range. Accepted values are: [1-1000]");
                        }
                    }
                    else if(!strcmp(param, "TONE_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_TONE_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("TONE_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "TONE_FILTERTYPE")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_TONE_FILTERTYPE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("TONE_FILTERTYPE value out of range. Accepted values are: [0-1]");
                        }
                    }
                    else if(!strcmp(param, "TONE_EQ")) {
                        if (strlen(val) < 100 && strlen(val) > 0) {
                            memset(jdsp->pCtl[jdsp->pCtlQidx].str, 0, sizeof(jdsp->pCtl[jdsp->pCtlQidx].str));
                            strcpy(jdsp->pCtl[jdsp->pCtlQidx].str, val);
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_TONE_EQ;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("TONE_EQ value out of range. Accepted values are: [0;0;0;0;0;0;0;0;0;0;0;0;0;0;0]");
                        }
                    }
                    else if(!strcmp(param, "MASTER_LIMTHRESHOLD")) {
                       float_t v = atof(val);
                       if(v >= -60 && v <= 0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_MASTER_LIMTHRESHOLD;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("MASTER_LIMTHRESHOLD value out of range. Accepted values are: [-60-0] (float)");
                        }
                    }
                    else if(!strcmp(param, "MASTER_LIMRELEASE")) {
                       float_t v = atof(val);
                       if(v >= 1.5 && v <= 2000) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_MASTER_LIMRELEASE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("MASTER_LIMRELEASE value out of range. Accepted values are: [1.5-200] (float)");
                        }
                    }
                    else if(!strcmp(param, "DDC_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DDC_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DDC_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "DDC_COEFFS")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl[jdsp->pCtlQidx].str, 0, sizeof(jdsp->pCtl[jdsp->pCtlQidx].str));
                            strcpy(jdsp->pCtl[jdsp->pCtlQidx].str, val);
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DDC_COEFFS;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DDC_COEFFS value out of range. Accepted values are: [/path/file.vdc]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_CONVOLVER_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("CONVOLVER_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_FILE")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl[jdsp->pCtlQidx].str, 0, sizeof(jdsp->pCtl[jdsp->pCtlQidx].str));
                            strcpy(jdsp->pCtl[jdsp->pCtlQidx].str, val);
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_CONVOLVER_FILE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("CONVOLVER_FILE value out of range. Accepted values are: [/path/file.irs]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_GAIN")) {
                       float_t v = atof(val);
                       if(v >= -80 && v <= 30) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_CONVOLVER_GAIN;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("CONVOLVER_GAIN value out of range. Accepted values are: [-80 - 30] (float)");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_BENCH_C0")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl[jdsp->pCtlQidx].str, 0, sizeof(jdsp->pCtl[jdsp->pCtlQidx].str));
                            strcpy(jdsp->pCtl[jdsp->pCtlQidx].str, val);
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_CONVOLVER_BENCH_C0;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("CONVOLVER_BENCH_C0 value out of range. Accepted values are: [0.000000]");
                        }
                    }
                    else if(!strcmp(param, "CONVOLVER_BENCH_C1")) {
                        if (strlen(val) > 0 && strlen(val) < 128) {
                            memset(jdsp->pCtl[jdsp->pCtlQidx].str, 0, sizeof(jdsp->pCtl[jdsp->pCtlQidx].str));
                            strcpy(jdsp->pCtl[jdsp->pCtlQidx].str, val);
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_CONVOLVER_BENCH_C1;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("CONVOLVER_BENCH_C1 value out of range. Accepted values are: [0.000000]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_OSF")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 4) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_OSF;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_OSF value out of range. Accepted values are: [0-4]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_REFLECTION_AMOUNT")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_REFLECTION_AMOUNT;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_REFLECTION_AMOUNT value out of range. Accepted values are: [0.0-1.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_FINALWET")) {
                       float_t v = atof(val);
                       if(v >= -70 && v <= 10) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_FINALWET;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_FINALWET value out of range. Accepted values are: [-70 - 10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_FINALDRY")) {
                       float_t v = atof(val);
                       if(v >= -70 && v <= 10) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_FINALDRY;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_FINALDRY value out of range. Accepted values are: [-70 - 10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_REFLECTION_FACTOR")) {
                       float_t v = atof(val);
                       if(v >= 0.5 && v <= 2.5) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_REFLECTION_FACTOR;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_REFLECTION_FACTOR value out of range. Accepted values are: [0.5-2.5] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_REFLECTION_WIDTH")) {
                       float_t v = atof(val);
                       if(v >= -1 && v <= 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_REFLECTION_WIDTH;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_REFLECTION_WIDTH value out of range. Accepted values are: [-1 - 1] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_WIDTH")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_WIDTH;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_WIDTH value out of range. Accepted values are: [0-1] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_WET")) {
                       float_t v = atof(val);
                       if(v >= -70 && v <= 10) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_WET;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_WET value out of range. Accepted values are: [-70 - 10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LFO_WANDER")) {
                       float_t v = atof(val);
                       if(v >= 0.1 && v <= 0.6) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_LFO_WANDER;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_LFO_WANDER value out of range. Accepted values are: [0.1 - 0.6] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_BASSBOOST")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 0.5) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_BASSBOOST;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_BASSBOOST value out of range. Accepted values are: [0-0.5] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LFO_SPIN")) {
                       float_t v = atof(val);
                       if(v >= 0 && v <= 10) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_LFO_SPIN;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_LFO_SPIN value out of range. Accepted values are: [0-10] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_DECAY")) {
                       float_t v = atof(val);
                       if(v >= 0.1 && v <= 30) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_DECAY;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_DECAY value out of range. Accepted values are: [0.1-30] (float)");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_DELAY")) {
                       int16_t v = atoi(val);
                       if(v >= -500 && v <= 500) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_DELAY;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_DELAY value out of range. Accepted values are: [-500 - 500]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_INPUT")) {
                       int16_t v = atoi(val);
                       if(v >= 200 && v <= 18000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_LPF_INPUT;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_LPF_INPUT value out of range. Accepted values are: [200-18000]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_BASS")) {
                       int16_t v = atoi(val);
                       if(v >= 50 && v <= 1050) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_LPF_BASS;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_LPF_BASS value out of range. Accepted values are: [50-1050]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_DAMP")) {
                       int16_t v = atoi(val);
                       if(v >=200 && v <= 18000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_LPF_DAMP;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_LPF_DAMP value out of range. Accepted values are: [200-18000]");
                        }
                    }
                    else if(!strcmp(param, "HEADSET_LPF_OUTPUT")) {
                       int16_t v = atoi(val);
                       if(v >=200 && v <= 18000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_HEADSET_LPF_OUTPUT;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("HEADSET_LPF_OUTPUT value out of range. Accepted values are: [200-18000]");
                        }
                    }
                    else if(!strcmp(param, "IIR_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_IIR_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("IIR_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "IIR_FILTER")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 11) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_IIR_FILTER;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("IIR_FILTER value out of range. Accepted values are: [0-11]");
                        }
                    }
                    else if(!strcmp(param, "IIR_QFACT")) {
                       float_t v = atof(val);
                       if(v >= 0.0 && v <= 4.0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_IIR_QFACT;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("IIR_QFACT value out of range. Accepted values are: [0.0-4.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "IIR_FREQ")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 40000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_IIR_FREQ;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("IIR_FREQ value out of range. Accepted values are: [0-40000]");
                        }
                    }
                    else if(!strcmp(param, "IIR_GAIN")) {
                       int16_t v = atoi(val);
                       if(v >= -100 && v <= 100) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_IIR_GAIN;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("IIR_GAIN value out of range. Accepted values are: [-100-100]");
                        }
                    }
                    else if(!strcmp(param, "SE_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_SE_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("SE_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "SE_EXCITER")) {
                       float_t v = atof(val);
                       if(v >= 0.0 && v <= 50.0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_SE_EXCITER;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("SE_EXCITER value out of range. Accepted values are: [0.0-50.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "SE_REFREQ")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 24000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_SE_REFREQ;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("SE_REFREQ value out of range. Accepted values are: [0-24000]");
                        }
                    }
                    else if(!strcmp(param, "FSURROUND_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_FSURROUND_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("FSURROUND_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "FSURROUND_WIDE")) {
                       float_t v = atof(val);
                       if(v >= 0.0 && v <= 10.0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_FSURROUND_WIDE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("FSURROUND_WIDE value out of range. Accepted values are: [0.0-10.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "FSURROUND_MID")) {
                       float_t v = atof(val);
                       if(v >= 0.0 && v <= 10.0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_FSURROUND_MID;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("FSURROUND_MID value out of range. Accepted values are: [0.0-10.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "FSURROUND_DEPTH")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 1000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_FSURROUND_DEPTH;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("FSURROUND_DEPTH value out of range. Accepted values are: [0-1000]");
                        }
                    }
                    else if(!strcmp(param, "ANALOGX_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_ANALOGX_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("ANALOGX_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "ANALOGX_MODEL")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 3) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_ANALOGX_MODEL;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("ANALOGX_MODEL value out of range. Accepted values are: [0-3]");
                        }
                    }
                    else if(!strcmp(param, "DS_ENABLE")) {
                       int8_t v = atoi(val);
                       if(v == 0 || v == 1) {
                            jdsp->pCtl[jdsp->pCtlQidx].i8 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_ENABLE;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_ENABLE value out of range. Accepted values are: [0|1]");
                        }
                    }
                    else if(!strcmp(param, "DS_BASSGAIN")) {
                       float_t v = atof(val);
                       if(v >= 0.0 && v <= 100.0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_BASSGAIN;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_BASSGAIN value out of range. Accepted values are: [0.0-100.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "DS_SIDEGAINX")) {
                       float_t v = atof(val);
                       if(v >= 0.0 && v <= 100.0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_SIDEGAINX;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_SIDEGAINX value out of range. Accepted values are: [0.0-100.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "DS_SIDEGAINY")) {
                       float_t v = atof(val);
                       if(v >= 0.0 && v <= 100.0) {
                            jdsp->pCtl[jdsp->pCtlQidx].f32 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_SIDEGAINY;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_SIDEGAINY value out of range. Accepted values are: [0.0-100.0] (float)");
                        }
                    }
                    else if(!strcmp(param, "DS_COEFFS_X_HIGH")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 12000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_COEFFS_X_HIGH;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_COEFFS_X_HIGH value out of range. Accepted values are: [0-12000]");
                        }
                    }
                    else if(!strcmp(param, "DS_COEFFS_X_LOW")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 12000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_COEFFS_X_LOW;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_COEFFS_X_LOW value out of range. Accepted values are: [0-12000]");
                        }
                    }
                    else if(!strcmp(param, "DS_COEFFS_Y_HIGH")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 12000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_COEFFS_Y_HIGH;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_COEFFS_Y_HIGH value out of range. Accepted values are: [0-12000]");
                        }
                    }
                    else if(!strcmp(param, "DS_COEFFS_Y_LOW")) {
                       int16_t v = atoi(val);
                       if(v >= 0 && v <= 12000) {
                            jdsp->pCtl[jdsp->pCtlQidx].i16 = v;
                            jdsp->pCtl[jdsp->pCtlQidx].param = PROP_DS_COEFFS_Y_LOW;
                            jdsp->pCtl[jdsp->pCtlQidx].pUpdate = true;
                            jdsp->pCtlQidx++;
                        } else {
                            SNDERR("DS_COEFFS_Y_LOW value out of range. Accepted values are: [0-12000]");
                        }
                    }
                memset(ctl_buf, 0, CTL_BUFFSIZE);
                if(jdsp->pCtlQidx >= CMD_QUEUE_LEN)
                    jdsp->pCtlQidx = 0;
                }
            }
        }
    } while ( jdsp->ctl_thread_running );
    close(fd);

ct_terminate:
#ifdef DEBUG
    printf("%s\n exit", __func__);
#endif
    pthread_exit(0);
    return NULL;
}

static void jdspfx_set_property(snd_pcm_jdspfx_t *self) {
    for(int i = 0; i < CMD_QUEUE_LEN; i++) {
        if(self->pCtl[i].pUpdate) {
            self->pCtl[i].pUpdate = false;
            switch (self->pCtl[i].param) {
                case PROP_FX_ENABLE: {
                    self->fx_enabled = self->pCtl[i].i8;
                }
                    break;
                case PROP_TUBE_ENABLE: {
                    self->tube_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1206, self->tube_enabled);
                }
                    break;
                case PROP_TUBE_DRIVE: {
                    self->tube_drive = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 150, (int16_t) self->tube_drive);
                }
                    break;
                case PROP_BASS_ENABLE: {
                    self->bass_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1201, self->bass_enabled);
                }
                    break;
                case PROP_BASS_MODE: {
                    self->bass_mode = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 112, (int16_t) self->bass_mode);
                }
                    break;
                case PROP_BASS_FILTERTYPE: {
                    self->bass_filtertype = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 113, (int16_t) self->bass_filtertype);
                }
                    break;
                case PROP_BASS_FREQ: {
                    self->bass_freq = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 114, (int16_t) self->bass_freq);
                }
                    break;
                case PROP_STEREOWIDE_ENABLE: {
                    self->stereowide_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1204, self->stereowide_enabled);
                }
                    break;
                case PROP_STEREOWIDE_MCOEFF: {
                    self->stereowide_mcoeff = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 137, (int16_t) self->stereowide_mcoeff,self->stereowide_scoeff);
                }
                    break;
                case PROP_STEREOWIDE_SCOEFF: {
                    self->stereowide_scoeff = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 137, (int16_t) self->stereowide_mcoeff,self->stereowide_scoeff);
                }
                    break;
                case PROP_BS2B_ENABLE: {
                    self->bs2b_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1208, self->bs2b_enabled);
                }
                    break;
                case PROP_BS2B_FCUT: {
                    self->bs2b_fcut = self->pCtl[i].i16;
                    if(self->bs2b_feed != 0)
                        command_set_px4_vx2x2(self->effectDspMain, 188, (int16_t)self->bs2b_fcut,(int16_t)self->bs2b_feed);
                }
                    break;
                case PROP_BS2B_FEED: {
                    self->bs2b_feed = self->pCtl[i].i16;
                    if(self->bs2b_feed != 0)
                        command_set_px4_vx2x2(self->effectDspMain, 188, (int16_t)self->bs2b_fcut,(int16_t)self->bs2b_feed);
                }
                    break;
                case PROP_COMPRESSOR_ENABLE: {
                    self->compression_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1200, self->compression_enabled);
                }
                    break;
                case PROP_COMPRESSOR_PREGAIN: {
                    self->compression_pregain = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 100, (int16_t) self->compression_pregain);
                }
                    break;
                case PROP_COMPRESSOR_THRESHOLD: {
                    self->compression_threshold  = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 101, (int16_t) self->compression_threshold);
                }
                    break;
                case PROP_COMPRESSOR_KNEE: {
                    self->compression_knee  = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 102, (int16_t) self->compression_knee);
                }
                    break;
                case PROP_COMPRESSOR_RATIO: {
                    self->compression_ratio  = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 103, (int16_t) self->compression_ratio);
                }
                    break;
                case PROP_COMPRESSOR_ATTACK: {
                    self->compression_attack  = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 104, (int16_t) self->compression_attack);
                }
                    break;
                case PROP_COMPRESSOR_RELEASE: {
                    self->compression_release  = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 105, (int16_t) self->compression_release);
                }
                    break;
                case PROP_TONE_ENABLE: {
                    self->tone_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1202, self->tone_enabled);
                }
                    break;
                case PROP_TONE_FILTERTYPE: {
                    self->tone_filtertype = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 151, (int16_t) self->tone_filtertype);
                }
                    break;
                case PROP_TONE_EQ:
                {
                    if (strlen(self->pCtl[i].str) < 100) {
                        memset (self->tone_eq, 0, sizeof(self->tone_eq));
                        strcpy(self->tone_eq, self->pCtl[i].str);
                        command_set_eq (self->effectDspMain, self->tone_eq);
                    }else{
                        printf("[E] EQ string too long (>100 bytes)");
                    }
                }
                    break;
                case PROP_MASTER_LIMTHRESHOLD:
                {
                    self->lim_threshold = self->pCtl[i].f32;
                    command_set_limiter(self->effectDspMain,self->lim_threshold,self->lim_release);
                }
                    break;
                case PROP_MASTER_LIMRELEASE:
                {
                    self->lim_release = self->pCtl[i].f32;
                    command_set_limiter(self->effectDspMain,self->lim_threshold,self->lim_release);
                }
                    break;
                case PROP_DDC_ENABLE: {
                    self->ddc_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1212, self->ddc_enabled);
                }
                    break;
                case PROP_DDC_COEFFS: {
                    memset (self->ddc_coeffs, 0, sizeof(self->ddc_coeffs));
                    strcpy(self->ddc_coeffs, self->pCtl[i].str);
                    command_set_ddc(self->effectDspMain, self->ddc_coeffs,self->ddc_enabled);
                }
                    break;
                case PROP_CONVOLVER_ENABLE: {
                    self->convolver_enabled = self->pCtl[i].i8;
                    command_set_convolver(self->effectDspMain, self->convolver_file,self->convolver_gain,self->convolver_quality,
                                  self->convolver_bench_c0,self->convolver_bench_c1,self->samplerate);
                    command_set_px4_vx2x1(self->effectDspMain, 1205, self->convolver_enabled);
                }
                    break;
                case PROP_CONVOLVER_FILE: {
                    memset (self->convolver_file, 0, sizeof(self->convolver_file));
                    strcpy(self->convolver_file, self->pCtl[i].str);
                }
                    break;
                case PROP_CONVOLVER_BENCH_C0: {
                    memset (self->convolver_bench_c0, 0,  sizeof(self->convolver_bench_c0));
                    strcpy(self->convolver_bench_c0, self->pCtl[i].str);
                }
                    break;
                case PROP_CONVOLVER_BENCH_C1: {
                    memset (self->convolver_bench_c1, 0, sizeof(self->convolver_bench_c1));
                    strcpy(self->convolver_bench_c1, self->pCtl[i].str);
                }
                    break;
                case PROP_CONVOLVER_GAIN:
                {
                    self->convolver_gain = self->pCtl[i].f32;
                }
                    break;
                case PROP_HEADSET_ENABLE: {
                    self->headset_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1203, self->headset_enabled);
                }
                    break;
                case PROP_HEADSET_OSF: {
                    self->headset_osf = self->pCtl[i].i16;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_DELAY: {
                    self->headset_delay =self->pCtl[i].i16;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_LPF_INPUT: {
                    self->headset_inputlpf = self->pCtl[i].i16;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_LPF_BASS: {
                    self->headset_basslpf = self->pCtl[i].i16;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_LPF_DAMP: {
                    self->headset_damplpf = self->pCtl[i].i16;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_LPF_OUTPUT: {
                    self->headset_outputlpf = self->pCtl[i].i16;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_REFLECTION_WIDTH: {
                    self->headset_reflection_width = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_REFLECTION_FACTOR: {
                    self->headset_reflection_factor = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_REFLECTION_AMOUNT: {
                    self->headset_reflection_amount = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_FINALDRY: {
                    self->headset_finaldry = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_FINALWET: {
                    self->headset_finalwet = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_WIDTH: {
                    self->headset_width = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_WET: {
                    self->headset_wet = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_LFO_WANDER: {
                    self->headset_lfo_wander = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_BASSBOOST: {
                    self->headset_bassboost = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_LFO_SPIN: {
                    self->headset_lfo_spin = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_HEADSET_DECAY: {
                    self->headset_decay = self->pCtl[i].f32;
                    command_set_reverb(self->effectDspMain,self);
                }
                    break;
                case PROP_IIR_ENABLE: {
                    self->iir_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1209, self->iir_enabled);
                }
                    break;
                case PROP_IIR_FILTER: {
                    self->iir_filter = self->pCtl[i].i16;
                    command_set_px4_vx8x2p(self->effectDspMain, 1501, self->iir_filter, self->iir_qfact);
                }
                    break;
                case PROP_IIR_QFACT: {
                    self->iir_qfact = self->pCtl[i].f32;
                    command_set_px4_vx8x2p(self->effectDspMain, 1501, self->iir_filter, self->iir_qfact);
                }
                    break;
                case PROP_IIR_FREQ: {
                    self->iir_freq = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 189, (int16_t)self->iir_freq,(int16_t)self->iir_gain);
                }
                    break;
                case PROP_IIR_GAIN: {
                    self->iir_gain = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 189, (int16_t)self->iir_freq,(int16_t)self->iir_gain);
                }
                    break;
                case PROP_SE_ENABLE: {
                    self->se_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1210, self->se_enabled);
                }
                    break;
                case PROP_SE_EXCITER: {
                    self->se_exciter = self->pCtl[i].f32;
                    command_set_px4_vx8x2p(self->effectDspMain, 1502, self->se_exciter, 0);
                }
                    break;
                case PROP_SE_REFREQ: {
                    self->se_refreq = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 1211, (int16_t)self->se_refreq);
                }
                    break;
                case PROP_FSURROUND_ENABLE: {
                    self->fs_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1207, self->fs_enabled);
                }
                    break;
                case PROP_FSURROUND_WIDE: {
                    self->fs_wide = self->pCtl[i].f32;
                    command_set_px4_vx8x2p(self->effectDspMain, 1503, self->fs_wide, self->fs_mid);
                }
                    break;
                case PROP_FSURROUND_MID: {
                    self->fs_mid = self->pCtl[i].f32;
                    command_set_px4_vx8x2p(self->effectDspMain, 1503, self->fs_wide, self->fs_mid);
                }
                    break;
                case PROP_FSURROUND_DEPTH: {
                    self->fs_depth = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 1213, (int16_t)self->fs_depth);
                }
                    break;
                case PROP_ANALOGX_ENABLE: {
                    self->ax_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1214, self->ax_enabled);
                }
                    break;
                case PROP_ANALOGX_MODEL: {
                    self->ax_model = self->pCtl[i].i16;
                    command_set_px4_vx2x1(self->effectDspMain, 1215, (int16_t)self->ax_model);
                }
                    break;
                case PROP_DS_ENABLE: {
                    self->ds_enabled = self->pCtl[i].i8;
                    command_set_px4_vx2x1(self->effectDspMain, 1216, self->ds_enabled);
                }
                    break;
                case PROP_DS_BASSGAIN: {
                    self->ds_bassgain = self->pCtl[i].f32;
                    command_set_px4_vx8x2p(self->effectDspMain, 1504, self->ds_bassgain, 0);
                }
                    break;
                case PROP_DS_SIDEGAINX: {
                    self->ds_sidegain_x = self->pCtl[i].f32;
                    command_set_px4_vx8x2p(self->effectDspMain, 1505, self->ds_sidegain_x, self->ds_sidegain_y);
                }
                    break;
                case PROP_DS_SIDEGAINY: {
                    self->ds_sidegain_y = self->pCtl[i].f32;
                    command_set_px4_vx8x2p(self->effectDspMain, 1505, self->ds_sidegain_x, self->ds_sidegain_y);
                }
                    break;
                case PROP_DS_COEFFS_X_HIGH: {
                    self->ds_coeffsx_high = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 187, (int16_t)self->ds_coeffsx_high,(int16_t)self->ds_coeffsx_low);
                }
                    break;
                case PROP_DS_COEFFS_X_LOW: {
                    self->ds_coeffsx_low = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 187, (int16_t)self->ds_coeffsx_high,(int16_t)self->ds_coeffsx_low);
                }
                    break;
                case PROP_DS_COEFFS_Y_HIGH: {
                    self->ds_coeffsy_high = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 186, (int16_t)self->ds_coeffsy_high,(int16_t)self->ds_coeffsy_low);
                }
                    break;
                case PROP_DS_COEFFS_Y_LOW: {
                    self->ds_coeffsy_low = self->pCtl[i].i16;
                    command_set_px4_vx2x2(self->effectDspMain, 186, (int16_t)self->ds_coeffsy_high,(int16_t)self->ds_coeffsy_low);
                }
                    break;
                default:
                    SNDERR("Invalid property: %d", self->pCtl[i].param);
                    break;
            }
        }
    }
    jdsp_cfg_write(self);
}

/* sync all parameters to fx core */
static void sync_all_parameters(snd_pcm_jdspfx_t *self) {

    command_set_buffercfg(self->effectDspMain,self->samplerate,self->format);

    config_set_px0_vx0x0(self->effectDspMain, EFFECT_CMD_ENABLE);

    // analog modelling
    command_set_px4_vx2x1(self->effectDspMain,
                          1206, (int16_t) self->tube_enabled);

    command_set_px4_vx2x1(self->effectDspMain,
                          150, self->tube_drive);

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

    // iir
    command_set_px4_vx8x2p(self->effectDspMain,
                          1501, self->iir_filter, self->iir_qfact);
    command_set_px4_vx2x2(self->effectDspMain,
                          189, (int16_t)self->iir_freq,(int16_t)self->iir_gain);
    command_set_px4_vx2x1(self->effectDspMain,
                          1209, self->iir_enabled);
    
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
    command_set_px4_vx2x1(self->effectDspMain,
                          151, (int16_t)self->tone_filtertype);
    command_set_px4_vx2x1(self->effectDspMain,
                          1202, self->tone_enabled);
    command_set_eq (self->effectDspMain, self->tone_eq);

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

    // spectrumextend
    command_set_px4_vx2x1(self->effectDspMain,
                          1211, self->se_refreq);
    command_set_px4_vx8x2p(self->effectDspMain,
                          1502, self->se_exciter, 0);
    command_set_px4_vx2x1(self->effectDspMain,
                          1210, self->se_enabled);

    // colorful music (field surround)
    command_set_px4_vx2x1(self->effectDspMain,
                          1213, self->fs_depth);
    command_set_px4_vx8x2p(self->effectDspMain,
                          1503, self->fs_wide, self->fs_mid);
    command_set_px4_vx2x1(self->effectDspMain,
                          1207, self->fs_enabled);

    // analogx
    command_set_px4_vx2x1(self->effectDspMain,
                          1214, self->ax_enabled);
    command_set_px4_vx2x1(self->effectDspMain,
                          1215, self->ax_model);

    // dynamic system
    command_set_px4_vx2x1(self->effectDspMain,
                          1216, self->ds_enabled);
    command_set_px4_vx8x2p(self->effectDspMain,
                          1504, self->ds_bassgain, 0);
    command_set_px4_vx8x2p(self->effectDspMain,
                          1505, self->ds_sidegain_x, self->ds_sidegain_y);
    command_set_px4_vx2x2(self->effectDspMain,
                          187, (int16_t)self->ds_coeffsx_high,(int16_t)self->ds_coeffsx_low);
    command_set_px4_vx2x2(self->effectDspMain,
                          186, (int16_t)self->ds_coeffsy_high,(int16_t)self->ds_coeffsy_low);
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

    if(jdsp->pCtl_commit) {
        pthread_mutex_lock(&jdsp->lock);
        jdspfx_set_property(jdsp);
        pthread_mutex_unlock(&jdsp->lock);
        jdsp->pCtl_commit = false;
    }

    /* Calculate buffer locations */
    src = (float*)(src_areas->addr + (src_areas->first + src_areas->step * src_offset)/8);
    dst = (float*)(dst_areas->addr + (dst_areas->first + dst_areas->step * dst_offset)/8);

    if (jdsp->fx_enabled) {
        switch(jdsp->format){
            case s16le:
                jdsp->in->frameCount = size;
                jdsp->in->s16 = (int16_t *)(src);
                jdsp->out->frameCount = size;
                jdsp->out->s16 = (int16_t *)(dst);
                jdsp->effectDspMain->process(jdsp->in, jdsp->out);
                break;
            case s32le:
                jdsp->in->frameCount = size;
                jdsp->in->s32 = (int32_t *)(src);
                jdsp->out->frameCount = size;
                jdsp->out->s32 = (int32_t *)(dst);
                jdsp->effectDspMain->process(jdsp->in, jdsp->out);
                break;
            case f32le:
                jdsp->in->frameCount = size;
                jdsp->in->f32 = (float *)(src);
                jdsp->out->frameCount = size;
                jdsp->out->f32 = (float*)(dst);
                jdsp->effectDspMain->process(jdsp->in, jdsp->out);
                break;
        }
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
    if(jdsp->ctl_thread_running) {
        jdsp->ctl_thread_running = false;
        pthread_cancel(jdsp->ctl_thread);
        pthread_join(jdsp->ctl_thread, NULL);
    }
    if(jdsp->init_done) {
        free(jdsp->pCtl);
        free(jdsp->in);
        free(jdsp->out);

        jdsp_cfg_write(jdsp);

        EffectDSPMain *intf = jdsp->effectDspMain;
        intf->command(EFFECT_CMD_RESET, 0, NULL, NULL, NULL);
        if (jdsp->effectDspMain != NULL) {
            delete jdsp->effectDspMain;
        }
    }
    free(jdsp);
    return 0;
}

static int jdsp_init(snd_pcm_extplug_t *ext)
{
    snd_pcm_jdspfx_t *jdsp = (snd_pcm_jdspfx_t *)ext;

    if(jdsp->init_done) {
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
    jdsp->iir_enabled = FALSE;
    jdsp->iir_filter = 0;
    jdsp->iir_freq = 400;
    jdsp->iir_gain = 10;
    jdsp->iir_qfact = 0.707;
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
    strncpy(jdsp->tone_eq, "0;0;0;0;0;0;0;0;0;0;0;0;0;0;0", 30);
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

    jdsp->se_enabled = FALSE;
    jdsp->se_exciter = 10;
    jdsp->se_refreq = 7600;
    jdsp->fs_enabled = FALSE;
    jdsp->fs_depth = 1;
    jdsp->fs_mid = 1.0;
    jdsp->fs_wide = 0.0;
    jdsp->ax_enabled = FALSE;
    jdsp->ax_model = 0;
    jdsp->ds_enabled = FALSE;
    jdsp->ds_bassgain = 1.0;
    jdsp->ds_sidegain_x = 1.0;
    jdsp->ds_sidegain_y = 1.0;
    jdsp->ds_coeffsx_high = 80;
    jdsp->ds_coeffsx_low = 120;
    jdsp->ds_coeffsy_high = 11020;
    jdsp->ds_coeffsy_low = 40;

    jdsp->pCtl = (jdsp_param_t *) malloc(CMD_QUEUE_LEN * sizeof(jdsp_param_t));
    for(int i = 0; i < CMD_QUEUE_LEN; i++) {
        jdsp->pCtl[i].pUpdate = false;
        jdsp->pCtl[i].param = 0;
    }

    jdsp_cfg_read(jdsp);

    /* initialize private resources */
    jdsp->effectDspMain = NULL;
    jdsp->effectDspMain = new EffectDSPMain();
    jdsp->in = (audio_buffer_t *) malloc(sizeof(audio_buffer_t));
    jdsp->out = (audio_buffer_t *) malloc(sizeof(audio_buffer_t));

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
        snd_pcm_extplug_delete(&jdsp->ext);
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