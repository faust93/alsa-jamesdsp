#include "global.h"
#ifdef DEBUG
#include "MemoryUsage.h"
#endif
#include <unistd.h>
#include "EffectDSPMain.h"
typedef struct
{
	int32_t status;
	uint32_t psize;
	uint32_t vsize;
	int32_t cmd;
	int32_t data;
} reply1x4_1x4_t;
const double interpFreq[NUM_BANDS] = { 25.0, 40.0, 63.0, 100.0, 160.0, 250.0, 400.0, 630.0, 1000.0, 1600.0, 2500.0, 4000.0, 6300.0, 10000.0, 16000.0 };

EffectDSPMain::EffectDSPMain()
	: DSPbufferLength(1024), inOutRWPosition(0), equalizerEnabled(0), ramp(1.0), pregain(12.0), threshold(-60.0), knee(30.0), ratio(12.0), attack(0.001), release(0.24), isBenchData(0), mPreset(0), reverbEnabled(0)
	, mMatrixMCoeff(1.0), mMatrixSCoeff(1.0), bassBoostLp(0), FIREq(0), convolver(0), fullStereoConvolver(0), sosCount(0), resampledSOSCount(0), usedSOSCount(0), df441(0), df48(0), dfResampled(0)
	, tempImpulseIncoming(0), tempImpulsedouble(0), finalImpulse(0), convolverReady(-1), bassLpReady(-1), analogModelEnable(0), tubedrive(2.0), eqFilterType(0), arbEq(0), xaxis(0), yaxis(0), eqFIRReady(0), bs2bfcut(0), bs2bfeed(0)
{
	double c0[12] = { 2.138018534150542e-5, 4.0608501987194246e-5, 7.950414700590711e-5, 1.4049065318523225e-4, 2.988065284903209e-4, 0.0013061668170781858, 0.0036204239724680425, 0.008959629624060151, 0.027083658741258742, 0.08156916666666666, 0.1978822177777778, 0.4410733777777778 };
	double c1[12] = { 5.88199398839289e-6, 1.1786813951189911e-5, 2.5600214528512222e-5, 8.53041086120132e-5, 2.656291374239004e-4, 5.047717001008378e-4, 8.214255850540808e-4, 0.0016754651127819551, 0.0033478867132867136, 0.006705333333333334, 0.013496382222222221, 0.02673028888888889 };
	benchmarkValue[0] = (double*)malloc(12 * sizeof(double));
	benchmarkValue[1] = (double*)malloc(12 * sizeof(double));
	memcpy(benchmarkValue[0], c0, sizeof(c0));
	memcpy(benchmarkValue[1], c1, sizeof(c1));
	memSize = DSPbufferLength * sizeof(double);
	inputBuffer[0] = (double*)malloc(memSize);
	inputBuffer[1] = (double*)malloc(memSize);
	outputBuffer[0] = (double*)malloc(memSize);
	outputBuffer[1] = (double*)malloc(memSize);
	memset(outputBuffer[0], 0, memSize);
	memset(outputBuffer[1], 0, memSize);
	tempBuf[0] = (double*)malloc(memSize);
	tempBuf[1] = (double*)malloc(memSize);

	r = NULL;
#ifdef DEBUG
	printf("[I] %d space allocated\n", DSPbufferLength);
#endif
	JLimiterInit(&kLimiter);
	JLimiterSetCoefficients(&kLimiter, -0.1, 60.0, mSamplingRate);
}
EffectDSPMain::~EffectDSPMain()
{
	if (inputBuffer[0])
	{
		free(inputBuffer[0]);
		inputBuffer[0] = 0;
		free(inputBuffer[1]);
		free(outputBuffer[0]);
		free(outputBuffer[1]);
		free(tempBuf[0]);
		free(tempBuf[1]);
	}
	FreeBassBoost();
	FreeEq();
	FreeConvolver();
	if (finalImpulse)
	{
		free(finalImpulse[0]);
		free(finalImpulse[1]);
		free(finalImpulse);
	}
	if (tempImpulseIncoming)
		free(tempImpulseIncoming);
	if (tempImpulsedouble)
		free(tempImpulsedouble);
	if (finalIR)
		free(finalIR);
	if (benchmarkValue[0])
		free(benchmarkValue[0]);
	if (benchmarkValue[1])
		free(benchmarkValue[1]);
	if (sosCount)
	{
		for (int i = 0; i < sosCount; i++)
		{
			free(df441[i]);
			free(df48[i]);
		}
		free(df441);
		free(df48);
		sosCount = 0;
		if (resampledSOSCount)
		{
			for (int i = 0; i < resampledSOSCount; i++)
				free(dfResampled[i]);
			free(dfResampled);
			dfResampled = 0;
			resampledSOSCount = 0;
		}
	}
#ifdef DEBUG
	printf("[I] Buffer freed\n");
#endif
}
void EffectDSPMain::FreeBassBoost()
{
	if (bassBoostLp)
	{
		for (unsigned int i = 0; i < NUMCHANNEL; i++)
		{
			AutoConvolver1x1Free(bassBoostLp[i]);
			free(bassBoostLp[i]);
		}
		free(bassBoostLp);
		bassBoostLp = 0;
		ramp = 0.4;
	}
}
void EffectDSPMain::FreeEq()
{
	if (xaxis)
	{
		free(xaxis);
		xaxis = 0;
	}
	if (yaxis)
	{
		free(yaxis);
		yaxis = 0;
	}
	if (arbEq)
	{
		ArbitraryEqFree(arbEq);
		free(arbEq);
		arbEq = 0;
	}
	if (FIREq)
	{
		for (unsigned int i = 0; i < NUMCHANNEL; i++)
		{
			AutoConvolver1x1Free(FIREq[i]);
			free(FIREq[i]);
		}
		free(FIREq);
		FIREq = 0;
	}
}
void EffectDSPMain::FreeConvolver()
{
	convolverReady = -1;
	int i;
	if (convolver)
	{
		for (i = 0; i < 2; i++)
		{
			if (convolver[i])
			{
				AutoConvolver1x1Free(convolver[i]);
				free(convolver[i]);
				convolver[i] = 0;
			}
		}
		free(convolver);
		convolver = 0;
	}
	if (fullStereoConvolver)
	{
		for (i = 0; i < 4; i++)
		{
			if (fullStereoConvolver[i])
			{
				AutoConvolver1x1Free(fullStereoConvolver[i]);
				free(fullStereoConvolver[i]);
				fullStereoConvolver[i] = 0;
			}
		}
		free(fullStereoConvolver);
		fullStereoConvolver = 0;
	}
}
void EffectDSPMain::channel_splitFloat(const float *buffer, unsigned int num_frames, float **chan_buffers, unsigned int num_channels)
{
	unsigned int i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		chan_buffers[i % num_channels][i / num_channels] = buffer[i];
}
int32_t EffectDSPMain::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
#ifdef DEBUG
	printf("[I] Memory used: %lf Mb\n", (double)getCurrentRSS() / 1024.0 / 1024.0);
#endif
	if (cmdCode == EFFECT_CMD_SET_CONFIG)
	{
		effect_buffer_access_e mAccessMode;
		int32_t *replyData = (int32_t *)pReplyData;

        int32_t ret = Effect::configure(pCmdData, &mAccessMode); //cmdData -> buffer_config_t // mAccessMode appears to be unused
		if (ret != 0)
		{
			*replyData = ret;
			return 0;
		}

		JLimiterSetCoefficients(&kLimiter, -0.1, 60.0, mSamplingRate);
		fullStconvparams.in = inputBuffer;
		fullStconvparams.frameCount = DSPbufferLength;
		fullStconvparams1.in = inputBuffer;
		fullStconvparams1.frameCount = DSPbufferLength;
		rightparams2.in = inputBuffer;
		rightparams2.frameCount = DSPbufferLength;
		if(replyData!=NULL)*replyData = 0;
		return 0;
	}
	if (cmdCode == EFFECT_CMD_GET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData;
		if (cep->psize == 4 && cep->vsize == 4)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 19999)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 19999;
				replyData->data = (int32_t)DSPbufferLength;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20000)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20000;
				replyData->data = (int32_t)mSamplingRate;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20001)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20001;
				if (!convolver)
					replyData->data = (int32_t)1;
				else
					replyData->data = (int32_t)0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20002)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20002;
				replyData->data = (int32_t)getpid();
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20003)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20003;
				if (isBenchData == 2)
					replyData->data = isBenchData;
				else
					replyData->data = 0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20004)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20003;
				if (convolver)
					replyData->data = convolver[0]->methods;
				else if (fullStereoConvolver)
					replyData->data = fullStereoConvolver[0]->methods;
				else
					replyData->data = 0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
		}
	}
	if (cmdCode == EFFECT_CMD_SET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData; //cmdData -> effect_param_t
		int32_t *replyData = (int32_t *)pReplyData;

#ifdef DEBUG
        int32_t _cmd = ((int32_t *)cep)[3];
        int16_t _dat = ((int16_t *)cep)[8];
        printf("[CMD] psize: %lu, vsize: %lu, cmd: %lu, data: %d\n",(unsigned long)cep->psize,(unsigned long)cep->vsize,(unsigned long)_cmd,(int)_dat);
#endif


		if (cep->psize == 4 && cep->vsize == 2)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 100)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = pregain;
				pregain = (double)value;
				if (oldVal != pregain && compressionEnabled)
					refreshCompressor();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 101)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = threshold;
				threshold = (double)-value;
				if (oldVal != threshold && compressionEnabled)
					refreshCompressor();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 102)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = knee;
				knee = (double)value;
				if (oldVal != knee && compressionEnabled)
					refreshCompressor();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 103)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = ratio;
				ratio = (double)value;
				if (oldVal != ratio && compressionEnabled)
					refreshCompressor();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 104)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = attack;
				attack = value / 1000.0f;
				if (oldVal != attack && compressionEnabled)
					refreshCompressor();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 105)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = release;
				release = value / 1000.0f;
				if (oldVal != release && compressionEnabled)
					refreshCompressor();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 112)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = bassBoostStrength;
				bassBoostStrength = value;
				if (oldVal != bassBoostStrength)
					bassLpReady = 0;
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 113)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = bassBoostFilterType;
				bassBoostFilterType = value;
				if (oldVal != bassBoostFilterType)
				{
					FreeBassBoost();
					bassLpReady = 0;
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 114)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = bassBoostCentreFreq;
				if (bassBoostCentreFreq < 55.0)
					bassBoostCentreFreq = 55.0;
				bassBoostCentreFreq = (double)value;
				if ((oldVal != bassBoostCentreFreq) || !bassLpReady)
				{
					bassLpReady = 0;
					if (!bassBoostFilterType)
						refreshBassLinearPhase(DSPbufferLength, 2048, bassBoostCentreFreq);
					else
						refreshBassLinearPhase(DSPbufferLength, 4096, bassBoostCentreFreq);
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 128)
			{
			    //REPLACED! This will have no effect
				int16_t oldVal = mPreset;
				mPreset = ((int16_t *)cep)[8];
				if (oldVal != mPreset)
					refreshReverb();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}

			else if (cmd == 150)
			{
				double oldVal = tubedrive;
				tubedrive = ((int16_t *)cep)[8] / 1000.0;
				if (analogModelEnable && oldVal != tubedrive)
					refreshTubeAmp();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 151)
			{
				int16_t oldVal = eqFilterType;
				int16_t value = ((int16_t *)cep)[8];
				if (oldVal != value)
				{
					eqFilterType = value;
					eqFIRReady = 2;
#ifdef DEBUG
					printf("[I] EQ filter type: %d\n", eqFilterType);
#endif
					FreeEq();
#ifdef DEBUG
					printf("[I] FIR EQ reseted caused by filter type change\n");
#endif
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			/*			else if (cmd == 808)
			{
			double oldVal = tubedrive;
			tubedrive = ((int16_t *)cep)[8] / 1000.0f;
			if(wavechild670Enabled == -1)
			{
			Real inputLevelA = tubedrive;
			Real ACThresholdA = 0.35; // This require 0 < ACThresholdA < 1.0
			uint timeConstantSelectA = 1; // Integer from 1-6
			Real DCThresholdA = 0.35; // This require 0 < DCThresholdA < 1.0
			Real outputGain = 1.0 / inputLevelA;
			int sidechainLink = 0;
			int isMidSide = 0;
			int useFeedbackTopology = 1;
			Real inputLevelB = inputLevelA;
			Real ACThresholdB = ACThresholdA;
			uint timeConstantSelectB = timeConstantSelectA;
			Real DCThresholdB = DCThresholdA;
			Wavechild670Parameters params = Wavechild670ParametersInit(inputLevelA, ACThresholdA, timeConstantSelectA, DCThresholdA,
			inputLevelB, ACThresholdB, timeConstantSelectB, DCThresholdB,
			sidechainLink, isMidSide, useFeedbackTopology, outputGain);
			if (compressor670)
			free(compressor670);
			compressor670 = Wavechild670Init((Real)mSamplingRate, &params);
			Wavechild670WarmUp(compressor670, 0.5);
			wavechild670Enabled = 1;
			#ifdef DEBUG
			printf("[I] Compressor670 Initialised\n");
			#endif
			}
			if(replyData!=NULL)*replyData = 0;
			return 0;
			}*/
			else if (cmd == 1200)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = compressionEnabled;
				compressionEnabled = value;
				if (oldVal != compressionEnabled)
					refreshCompressor();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1201)
			{
				int16_t oldVal = bassBoostEnabled;
				bassBoostEnabled = ((int16_t *)cep)[8];
				if (!bassBoostEnabled && (oldVal != bassBoostEnabled))
				{
					FreeBassBoost();
					bassLpReady = 0;
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1202)
			{
				int16_t oldVal = equalizerEnabled;
				equalizerEnabled = ((int16_t *)cep)[8];
				if ((equalizerEnabled == 1 && (oldVal != equalizerEnabled)) || eqFIRReady == 2)
				{
					xaxis = (double*)malloc(1024 * sizeof(double));
					yaxis = (double*)malloc(1024 * sizeof(double));
					linspace(xaxis, 1024, interpFreq[0], interpFreq[NUM_BANDSM1]);
					arbEq = (ArbitraryEq*)malloc(sizeof(ArbitraryEq));
					eqfilterLength = 8192;
					InitArbitraryEq(arbEq, &eqfilterLength, eqFilterType);
					for (int i = 0; i < 1024; i++)
						ArbitraryEqInsertNode(arbEq, xaxis[i], 0.0, 0);
#ifdef DEBUG
					printf("[I] FIR EQ Initialised\n");
#endif
				}
				else if (!equalizerEnabled && (oldVal != equalizerEnabled))
				{
					eqFIRReady = 0;
					FreeEq();
#ifdef DEBUG
					printf("[I] FIR EQ destroyed\n");
#endif
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1203)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = reverbEnabled;
				reverbEnabled = value;
				if (oldVal != reverbEnabled)
					refreshReverb();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1204)
			{
				stereoWidenEnabled = ((int16_t *)cep)[8];
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1208)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val)
					bs2bEnabled = 2;
				else
					bs2bEnabled = 0;
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1209)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val) {
					refreshIIR();
					IIRenabled = 1;
				}
				else
					IIRenabled = 0;
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1210)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val) {
					spectrumExtend.SetSamplingRate(mSamplingRate);
					spectrumExtend.SetReferenceFrequency((uint32_t)spectrumFreq);
					spectrumExtend.SetExciter(spectrumExciter);
					spectrumExtend.SetEnable(true);
					spectrumExtend.Reset();
					spectrumEnabled = 1;
				}
				else {
					spectrumExtend.SetEnable(false);
					spectrumEnabled = 0;
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
 			else if (cmd == 1211)
			{
				uint32_t oldFreq = spectrumFreq; 
				uint32_t newfreq = ((int16_t *)cep)[8];
				spectrumFreq = newfreq;
                if (spectrumEnabled && spectrumFreq != oldFreq)
                {
					spectrumExtend.SetReferenceFrequency(spectrumFreq);
#ifdef DEBUG
					printf("[I] SpectrumExtend - Freq: %d\n",newfreq);
#endif
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1205)
			{
				convolverEnabled = ((int16_t *)cep)[8];
				if (!convolverEnabled)
					FreeConvolver();
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1206)
			{
				int16_t oldVal = analogModelEnable;
				analogModelEnable = ((int16_t *)cep)[8];
				if (analogModelEnable && oldVal != analogModelEnable)
					refreshTubeAmp();
                if(replyData!=NULL)*replyData = 0;
                return 0;
			}
			else if (cmd == 1207)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val) {
                    colorfulMusic.SetEnable(true);
                    colorfulMusic.SetSamplingRate(mSamplingRate);
					colorfulMusic.SetDepthValue((int16_t)fSurroundDepth);
					colorfulMusic.SetMidImageValue(fSurroundMid);
					colorfulMusic.SetWidenValue(fSurroundWide);
                    colorfulMusic.Reset();
					fSurroundEnabled = 1;
				}
				else {
					colorfulMusic.SetEnable(false);
					fSurroundEnabled = 0;
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1216)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val) {
					dynamicSystem.SetEnable(true);
					dynamicSystem.SetSamplingRate(mSamplingRate);
					dynamicSystem.SetBassGain(DSbassGain);
					dynamicSystem.SetYCoeffs(DSYcoeffsLow, DSYcoeffsHigh);
					dynamicSystem.SetXCoeffs(DSXcoeffsLow, DSXcoeffsHigh);
					dynamicSystem.SetSideGain(DSsideGainX, DSsideGainY);
					dynamicSystem.Reset();
					DSenabled = 1;
				}
				else {
					dynamicSystem.SetEnable(false);
					DSenabled = 0;
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1215)
			{
				int16_t oldM = AXModel;
				AXModel = ((int16_t *)cep)[8];
				if (AXEnabled && oldM != AXModel) {
					analogX.SetProcessingModel(AXModel);
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1214)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val) {
					analogX.SetEnable(true);
					analogX.SetSamplingRate(mSamplingRate);
					analogX.SetProcessingModel(AXModel);
					analogX.Reset();
					AXEnabled = 1;
				}
				else {
					analogX.SetEnable(false);
					AXEnabled = 0;
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1213)
			{
				int16_t oldD = fSurroundDepth;
				fSurroundDepth = ((int16_t *)cep)[8];
				if (fSurroundEnabled && oldD != fSurroundDepth) {
					colorfulMusic.SetDepthValue((int16_t)fSurroundDepth);
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1212)
			{
				int16_t oldVal = viperddcEnabled;
				if (mSamplingRate == 44100.0 && df441)
				{
					sosPointer = df441;
					usedSOSCount = sosCount;
					viperddcEnabled = ((int16_t *)cep)[8];
				}
				else if (mSamplingRate == 48000.0 && df48)
				{
					sosPointer = df48;
					usedSOSCount = sosCount;
					viperddcEnabled = ((int16_t *)cep)[8];
				}
				else
				{
					resampledSOSCount = PeakingFilterResampler(df48, 48000.0, &dfResampled, mSamplingRate, sosCount);
					usedSOSCount = resampledSOSCount;
					sosPointer = dfResampled;
					viperddcEnabled = ((int16_t *)cep)[8];
				}
#ifdef DEBUG
				printf("[I] viperddcEnabled: %d\n", viperddcEnabled);
#endif
				if (!viperddcEnabled && oldVal)
				{
					if (stringEq)
					{
						free(stringEq);
						stringEq = 0;
					}
					if (sosCount)
					{
						for (int i = 0; i < sosCount; i++)
						{
							free(df441[i]);
							free(df48[i]);
						}
						free(df441);
						df441 = 0;
						free(df48);
						df48 = 0;
						sosCount = 0;
						if (resampledSOSCount)
						{
							for (int i = 0; i < resampledSOSCount; i++)
								free(dfResampled[i]);
							free(dfResampled);
							dfResampled = 0;
							resampledSOSCount = 0;
						}
						sosPointer = 0;
					}
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 10003)
			{
				samplesInc = ((int16_t *)cep)[8];
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 10004)
			{
				int i, j, tempbufValue;
				if (finalImpulse)
				{
					for (i = 0; i < previousimpChannels; i++)
						free(finalImpulse[i]);
					free(finalImpulse);
					finalImpulse = 0;
				}
				if(tempImpulsedouble)
					free(tempImpulsedouble);
				if (!finalImpulse)
				{
					tempbufValue = impulseLengthActual * impChannels;
					tempImpulsedouble = (double*)malloc(tempbufValue * sizeof(double));
					if (!tempImpulsedouble)
					{
						convolverReady = -1;
						convolverEnabled = !convolverEnabled;
					}
					for (i = 0; i < tempbufValue; i++)
						tempImpulsedouble[i] = (double)tempImpulseIncoming[i];
					free(tempImpulseIncoming);
					tempImpulseIncoming = 0;
					finalImpulse = (double**)malloc(impChannels * sizeof(double*));
					for (i = 0; i < impChannels; i++)
					{
						double* channelbuf = (double*)malloc(impulseLengthActual * sizeof(double));
						if (!channelbuf)
						{
							convolverReady = -1;
							convolverEnabled = !convolverEnabled;
							free(finalImpulse);
							finalImpulse = 0;
						}
						double* p = tempImpulsedouble + i;
						for (j = 0; j < impulseLengthActual; j++)
							channelbuf[j] = p[j * impChannels];
						finalImpulse[i] = channelbuf;
					}
					if (!refreshConvolver(DSPbufferLength))
					{
						convolverReady = -1;
						convolverEnabled = !convolverEnabled;
						if (finalImpulse)
						{
							for (i = 0; i < impChannels; i++)
								free(finalImpulse[i]);
							free(finalImpulse);
							finalImpulse = 0;
						}
						if (tempImpulsedouble)
						{
							free(tempImpulsedouble);
							tempImpulsedouble = 0;
						}
					}
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 10005)
			{
				stringIndex = ((int16_t *)cep)[8];
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 10009)
			{
				if (!viperddcEnabled)
				{
					if (sosCount)
					{
						for (int i = 0; i < sosCount; i++)
						{
							free(df441[i]);
							free(df48[i]);
						}
						free(df441);
						df441 = 0;
						free(df48);
						df48 = 0;
						sosCount = 0;
						if (resampledSOSCount)
						{
							for (int i = 0; i < resampledSOSCount; i++)
								free(dfResampled[i]);
							free(dfResampled);
							dfResampled = 0;
							resampledSOSCount = 0;
						}
						sosPointer = 0;
					}
				}
#ifdef DEBUG
				printf("[I] %s\n", stringEq);
#endif
				sosCount = DDCParser(stringEq, &df441, &df48);
#ifdef DEBUG
			printf("[I] VDC num of SOS: %d\n", sosCount);
			printf("[I] VDC df48[0].b0: %1.14lf\n", df48[0]->b0);
#endif
				free(stringEq);
				stringEq = 0;
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
        if (cep->psize == 4 && cep->vsize == 4) {
            int32_t cmd = ((int32_t *)cep)[3];
            if (cmd == 137) {
                int16_t matrixM = ((int16_t *) cep)[8];
                int16_t matrixS = ((int16_t *) cep)[9];
                refreshStereoWiden((uint32_t)matrixM,(uint32_t)matrixS);
                if (replyData != NULL)*replyData = 0;
                return 0;
            }
            else if (cmd == 186)
            {
				int16_t oldh = DSYcoeffsHigh;
				int16_t oldl = DSYcoeffsLow;
                DSYcoeffsHigh = ((int16_t *) cep)[8];
                DSYcoeffsLow = ((int16_t *) cep)[9];
                if (DSenabled && ((int16_t)DSYcoeffsHigh != oldh || (int16_t)DSYcoeffsLow != oldl))
                {
					dynamicSystem.SetYCoeffs(DSYcoeffsLow, DSYcoeffsHigh);
                }
                if(replyData!=NULL)*replyData = 0;
                return 0;
            }
            else if (cmd == 187)
            {
				int16_t oldh = DSXcoeffsHigh;
				int16_t oldl = DSXcoeffsLow;
                DSXcoeffsHigh = ((int16_t *) cep)[8];
                DSXcoeffsLow = ((int16_t *) cep)[9];
                if (DSenabled && ((int16_t)DSXcoeffsHigh != oldh || (int16_t)DSXcoeffsLow != oldl))
                {
					dynamicSystem.SetXCoeffs(DSXcoeffsLow, DSXcoeffsHigh);
                }
                if(replyData!=NULL)*replyData = 0;
                return 0;
            }
            else if (cmd == 188)
            {
				int16_t oldFcut = bs2bfcut;
				int16_t oldFeed = bs2bfeed;
                bs2bfcut = ((int16_t *) cep)[8];
                bs2bfeed = ((int16_t *) cep)[9];
                //fcut {300-2000}
                //feed {10-150} 10=1dB
                int res = ((unsigned int)bs2bfcut | ((unsigned int)bs2bfeed << 16));
                if (bs2bEnabled == 2 || (bs2bfcut != oldFcut || bs2bfeed != oldFeed))
                {
					bs2bEnabled = 0;
                    BS2BInit(&bs2b, (unsigned int)mSamplingRate, res);
#ifdef DEBUG
                    printf("[I] BS2B - Crossfeeding level: %d (%fdB), Cutoff frequency: %dHz\n",bs2bfeed,bs2bfeed/10.0f,bs2bfcut);
#endif
                    /*if (value == 0)
                       BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_JMEIER_CLEVEL);
                   else if (value == 1)
                       BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_CMOY_CLEVEL);
                   else if (value == 2)
                       BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_DEFAULT_CLEVEL);*/
                    bs2bEnabled = 1;
                }
                if(replyData!=NULL)*replyData = 0;
                return 0;
            }
 			else if (cmd == 189)
			{
				double oldFreq = IIRfreq; 
				double oldGgain = IIRgain;
				int16_t newfreq = ((int16_t *)cep)[8];
				int16_t newgain = ((int16_t *)cep)[9];
				IIRfreq = newfreq/10.0f;
				IIRgain = newgain/10.0f;
                if (IIRenabled && (IIRfreq != oldFreq || IIRgain != oldGgain))
                {
					refreshIIR();
#ifdef DEBUG
					printf("[I] IIR - Freq: %d (%fdB), Gain: %f, Qfact: %f, Filter: %d\n",newfreq,IIRfreq,IIRgain,IIRqfact,IIRfilter);
#endif
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
        }
		if (cep->psize == 4 && cep->vsize == 8)
		{
			int32_t cmd = (int32_t)((float*)cep)[3];
			if (cmd == 1500)
			{
				double limThreshold = (double)((float*)cep)[4];
				double limRelease = (double)((float*)cep)[5];
				if (limThreshold > -0.09)
					limThreshold = -0.09;
				if (limRelease < 0.15)
					limRelease = 0.15;
				JLimiterSetCoefficients(&kLimiter, limThreshold, limRelease, (double)mSamplingRate);
#ifdef DEBUG
				printf("[I] limThreshold: %f, limRelease: %f\n", (float)limThreshold, (float)limRelease);
#endif
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1501)
			{
				int16_t oldF = IIRfilter;
				float_t oldQ = IIRqfact;
				IIRfilter = (double)((float*)cep)[4];
				IIRqfact = (double)((float*)cep)[5];
				if (IIRenabled && (oldF != IIRfilter || oldQ != IIRqfact)) {
					refreshIIR();
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1502)
			{
				float_t oldE = spectrumExciter;
				spectrumExciter = ((float*)cep)[4];
				if (spectrumEnabled && oldE != spectrumExciter) {
					spectrumExtend.SetExciter((float) spectrumExciter);
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1503)
			{
				float_t oldW = fSurroundWide;
				float_t oldM = fSurroundMid;
				fSurroundWide = ((float*)cep)[4];
				fSurroundMid = ((float*)cep)[5];
				if (fSurroundEnabled && (oldW != fSurroundWide || oldM != fSurroundMid)) {
					colorfulMusic.SetWidenValue(fSurroundWide);
					colorfulMusic.SetMidImageValue(fSurroundMid);
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1504)
			{
				float_t oldv = DSbassGain;
				DSbassGain = ((float*)cep)[4];
				if (DSenabled && oldv != DSbassGain) {
					dynamicSystem.SetBassGain(DSbassGain);
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
			else if (cmd == 1505)
			{
				int16_t oldx = DSsideGainX;
				float_t oldy = DSsideGainY;
				DSsideGainX = (double)((float*)cep)[4];
				DSsideGainY = (double)((float*)cep)[5];
				if (DSenabled && (oldx != DSsideGainX || oldy != DSsideGainY)) {
					dynamicSystem.SetSideGain(DSsideGainX, DSsideGainY);
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 60)
		{
			int32_t cmd = (int32_t)((float*)cep)[3];
			if (cmd == 115)
			{
				double mBand[NUM_BANDS];
				for (int i = 0; i < NUM_BANDS; i++)
					mBand[i] = (double)((float*)cep)[4 + i];
				refreshEqBands(DSPbufferLength, mBand);
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 40)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 1997)
			{
				if (isBenchData < 3)
				{
					for (uint32_t i = 0; i < 10; i++)
					{
						benchmarkValue[0][i] = (double)((float*)cep)[4 + i];
#ifdef DEBUG
						printf("[I] bench_c0: %lf\n", benchmarkValue[0][i]);
#endif
					}
					isBenchData++;
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 40)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 1998)
			{
				if (isBenchData < 3)
				{
					for (uint32_t i = 0; i < 10; i++)
					{
						benchmarkValue[1][i] = (double)((float*)cep)[4 + i];
#ifdef DEBUG
						printf("[I] bench_c1: %lf\n", benchmarkValue[1][i]);
#endif
					}
					isBenchData++;
					if (convolverReady > 0)
					{
						if (!refreshConvolver(DSPbufferLength))
						{
							if (finalImpulse)
							{
								for (int i = 0; i < impChannels; i++)
									free(finalImpulse[i]);
								free(finalImpulse);
								finalImpulse = 0;
								free(tempImpulsedouble);
								tempImpulsedouble = 0;
							}
							convolverReady = -1;
							convolverEnabled = !convolverEnabled;
						}
					}
				}
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}

        if (cep->psize == 4 && cep->vsize == 8)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 8888)
			{
				int32_t times = ((int32_t *)cep)[4];
				int32_t sizePerBuffer = ((int32_t *)cep)[5];
				stringLength = times * sizePerBuffer;
#ifdef DEBUG
				printf("[I] Allocate %d stringLength\n", stringLength);
#endif
				if(stringEq) {
					free(stringEq);
					stringEq = NULL;
				}
				stringEq = (char*)calloc(stringLength, sizeof(char));
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 16)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 9999)
			{
				previousimpChannels = impChannels;
				impChannels = ((int32_t *)cep)[5];
				impulseLengthActual = ((int32_t *)cep)[4] / impChannels;
				convGaindB = (double)((int32_t *)cep)[6] / 262144.0;
				if (convGaindB > 50.0)
					convGaindB = 50.0;
				int numTime2Send = ((int32_t *)cep)[7];
				if(tempImpulseIncoming) {
					free(tempImpulseIncoming);
					tempImpulseIncoming = NULL;
				}
				tempImpulseIncoming = (float*)calloc(4096 * impChannels * numTime2Send, sizeof(float));
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 16384)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 12000)
			{
				memcpy(tempImpulseIncoming + (samplesInc * 4096), ((float*)cep) + 4, 4096 * sizeof(float));
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 256)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 12001)
			{
#ifdef DEBUG
				printf("[I] Copying string\n");
#endif
				memcpy(stringEq + (stringIndex * 256), ((char*)cep) + 16, 256 * sizeof(char));
                if(replyData!=NULL)*replyData = 0;
				return 0;
			}
		}
		return -1;
	}

	return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}
void EffectDSPMain::refreshTubeAmp()
{
	if (!InitTube(&tubeP[0], 0, mSamplingRate, tubedrive, 8192, 0)) {
        analogModelEnable = 0;
    }
	tubeP[1] = tubeP[0];
	rightparams2.tube = tubeP;
}
void EffectDSPMain::refreshBassLinearPhase(uint32_t DSPbufferLength, uint32_t tapsLPFIR, double bassBoostCentreFreq)
{
	double strength = (double)bassBoostStrength / 100.0;
	if (strength < 1.0)
		strength = 1.0;
	int filterLength = (int)tapsLPFIR;
	double transition = 80.0;
	if (filterLength > 4096)
		transition = 40.0;
	double freq[4] = { 0, (bassBoostCentreFreq * 2.0) / mSamplingRate, (bassBoostCentreFreq * 2.0 + transition) / mSamplingRate, 1.0 };
	double amplitude[4] = { strength, strength, 0, 0 };
	double *freqSamplImp = fir2(&filterLength, freq, amplitude, 4);
#ifdef DEBUG
	printf("[I] filterLength: %d\n", filterLength);
	if (!freqSamplImp)
		printf("[I] Pointer freqSamplImp is invalid\n");
#endif
	unsigned int i;
	if (!bassBoostLp)
	{
		bassBoostLp = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			bassBoostLp[i] = AllocateAutoConvolver1x1ZeroLatency(freqSamplImp, filterLength, DSPbufferLength);
		ramp = 0.4;
	}
	else
	{
		for (i = 0; i < NUMCHANNEL; i++)
			UpdateAutoConvolver1x1ZeroLatency(bassBoostLp[i], freqSamplImp, filterLength);
	}
	free(freqSamplImp);
#ifdef DEBUG
	printf("[I] Linear phase bass boost allocate all done: total taps %d\n", filterLength);
#endif
	bassLpReady = 1;
}
int EffectDSPMain::refreshConvolver(uint32_t DSPbufferLength)
{
	if (!finalImpulse)
		return 0;
#ifdef DEBUG
	printf("[I] refreshConvolver::IR channel count:%d, IR frame count:%d, Audio buffer size:%d\n", impChannels, impulseLengthActual, DSPbufferLength);
#endif
	int i;
	FreeConvolver();
	if (!convolver)
	{
		if (impChannels < 3)
		{
			convolver = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * 2);
			if (!convolver)
				return 0;
			for (i = 0; i < 2; i++)
			{
				if (impChannels == 1)
					convolver[i] = InitAutoConvolver1x1(finalImpulse[0], impulseLengthActual, DSPbufferLength, convGaindB, benchmarkValue, 12);
				else
					convolver[i] = InitAutoConvolver1x1(finalImpulse[i], impulseLengthActual, DSPbufferLength, convGaindB, benchmarkValue, 12);
			}
			fullStconvparams.conv = convolver;
			fullStconvparams.out = outputBuffer;
			if (finalImpulse)
			{
				for (i = 0; i < impChannels; i++)
					free(finalImpulse[i]);
				free(finalImpulse);
				finalImpulse = 0;
				free(tempImpulsedouble);
				tempImpulsedouble = 0;
			}
			if (impulseLengthActual < 20000)
				convolverReady = 1;
			else
				convolverReady = 2;
#ifdef DEBUG
			printf("[I] Convolver strategy used: %d\n", convolver[0]->methods);
#endif
		}
		else if (impChannels == 4)
		{
			if(fullStereoConvolver)
				free(fullStereoConvolver);
			fullStereoConvolver = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * 4);
			if (!fullStereoConvolver)
				return 0;
			for (i = 0; i < 4; i++)
				fullStereoConvolver[i] = InitAutoConvolver1x1(finalImpulse[i], impulseLengthActual, DSPbufferLength, convGaindB, benchmarkValue, 12);
			fullStconvparams.conv = fullStereoConvolver;
			fullStconvparams1.conv = fullStereoConvolver;
			fullStconvparams.out = tempBuf;
			fullStconvparams1.out = tempBuf;
			if (finalImpulse)
			{
				for (i = 0; i < 4; i++)
					free(finalImpulse[i]);
				free(finalImpulse);
				finalImpulse = 0;
				free(tempImpulsedouble);
				tempImpulsedouble = 0;
			}
			if (impulseLengthActual < 8192)
				convolverReady = 3;
			else
				convolverReady = 4;
#ifdef DEBUG
			printf("[I] Convolver strategy used: %d\n", fullStereoConvolver[0]->methods);
#endif
		}
		ramp = 0.4;
#ifdef DEBUG
		printf("[I] Convolver IR allocate complete\n");
#endif
	}
	return 1;
}
void EffectDSPMain::refreshStereoWiden(uint32_t m,uint32_t s)
{
    mMatrixMCoeff = m/1000.0f; //Min-Max: 0-10000 -> x/1000 -> 0.0-10.0
    mMatrixSCoeff = s/1000.0f;
#ifdef DEBUG
    printf("[I] Stereo widener - MCoeff: %f, SCoeff: %f\n",mMatrixMCoeff,mMatrixSCoeff);
#endif
	/*switch (parameter)
	{
	case 0: // A Bit
		mMatrixMCoeff = 1.0 * 0.5;
		mMatrixSCoeff = 1.2 * 0.5;
		break;
	case 1: // Slight
		mMatrixMCoeff = 0.95 * 0.5;
		mMatrixSCoeff = 1.4 * 0.5;
		break;
	case 2: // Moderate
		mMatrixMCoeff = 0.9 * 0.5;
		mMatrixSCoeff = 1.6 * 0.5;
		break;
	case 3: // High
		mMatrixMCoeff = 0.85 * 0.5;
		mMatrixSCoeff = 1.8 * 0.5;
		break;
	case 4: // Super
		mMatrixMCoeff = 0.8 * 0.5;
		mMatrixSCoeff = 2.0 * 0.5;
		break;
	}*/
}
void EffectDSPMain::refreshCompressor()
{
	sf_advancecomp(&compressor, mSamplingRate, pregain, threshold, knee, ratio, attack, release, 0.003, 0.09, 0.16, 0.42, 0.98, -(pregain / 1.4));
	ramp = 0.3;
}
void EffectDSPMain::refreshIIR()
{
	iir.pf_freq = IIRfreq;
	iir.pf_gain = IIRgain;
	iir.pf_qfact = IIRqfact; //0.707
	IIRFilterInit(&iir, mSamplingRate, IIRfilter);
}
void EffectDSPMain::refreshEqBands(uint32_t DSPbufferLength, double *bands)
{
	if (!arbEq || !xaxis || !yaxis)
		return;
#ifdef DEBUG
	printf("[I] Allocating FIR Equalizer\n");
#endif
	double y2[NUM_BANDS];
	double workingBuf[NUM_BANDSM1]; // interpFreq or bands data length minus 1
	spline(&interpFreq[0], bands, NUM_BANDS, &y2[0], &workingBuf[0]);
	splint(&interpFreq[0], bands, &y2[0], NUM_BANDS, xaxis, yaxis, 1024, 1);
	int i;
	for (i = 0; i < 1024; i++)
		arbEq->nodes[i]->gain = yaxis[i];
	double *eqImpulseResponse = arbEq->GetFilter(arbEq, mSamplingRate);
	if (!FIREq)
	{
		FIREq = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			FIREq[i] = AllocateAutoConvolver1x1ZeroLatency(eqImpulseResponse, eqfilterLength, DSPbufferLength);
	}
	else
	{
		for (i = 0; i < NUMCHANNEL; i++)
			UpdateAutoConvolver1x1ZeroLatency(FIREq[i], eqImpulseResponse, eqfilterLength);
	}
#ifdef DEBUG
	printf("[I] FIR Equalizer allocate all done: total taps %d\n", eqfilterLength);
#endif
	eqFIRReady = 1;
}
void EffectDSPMain::refreshReverb()
{
	sf_advancereverb(&myreverb,mSamplingRate,r->oversamplefactor,r->ertolate,r->erefwet
	        ,r->dry,r->ereffactor,r->erefwidth,r->width,r->wet,r->wander,r->bassb,
    	    r->spin,r->inputlpf,r->basslpf,r->damplpf,r->outputlpf,r->rt60,r->delay);
	//sf_presetreverb(&myreverb, mSamplingRate, (sf_reverb_preset)mPreset);
}
void *EffectDSPMain::threadingConvF(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[0]->process(arguments->conv[0], arguments->in[0], arguments->out[0], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingConvF1(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[1]->process(arguments->conv[1], arguments->in[0], arguments->out[1], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingConvF2(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[3]->process(arguments->conv[3], arguments->in[1], arguments->out[1], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingTube(void *args)
{
	ptrThreadParamsTube *arguments = (ptrThreadParamsTube*)args;
	processTube(&arguments->tube[1], arguments->in[1], arguments->in[1], arguments->frameCount);
	return 0;
}
int32_t EffectDSPMain::process(audio_buffer_t *in, audio_buffer_t *out)
{

	int i, framePos, framePos2x, actualFrameCount = in->frameCount;
	int pos = inOutRWPosition;
	switch (formatFloatModeInt32Mode)
	{
	case 0:
		for (framePos = 0; framePos < actualFrameCount; framePos++)
		{
			framePos2x = framePos << 1;
			outputBuffer[0][pos] *= ramp;
			outputBuffer[1][pos] *= ramp;
			JLimiterProcess(&kLimiter, &outputBuffer[0][pos], &outputBuffer[1][pos]);
			if (outputBuffer[0][pos] > 1.0)
				outputBuffer[0][pos] = 1.0;
			if (outputBuffer[0][pos] < -1.0)
				outputBuffer[0][pos] = -1.0;
			if (outputBuffer[1][pos] > 1.0)
				outputBuffer[1][pos] = 1.0;
			if (outputBuffer[1][pos] < -1.0)
				outputBuffer[1][pos] = -1.0;
			inputBuffer[0][pos] = (double)in->s16[framePos2x] * 3.051757812500000e-05;
			out->s16[framePos2x] = (int16_t)(outputBuffer[0][pos] * 32768.0);
			inputBuffer[1][pos] = (double)in->s16[++framePos2x] * 3.051757812500000e-05;
			out->s16[framePos2x] = (int16_t)(outputBuffer[1][pos] * 32768.0);
			pos++;
			if (pos == DSPbufferLength)
			{
				if (spectrumEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
                        spectrumExtend.Process(&inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if(DSenabled)
					dynamicSystem.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				if (bassBoostEnabled)
				{
					if (bassLpReady > 0)
					{
						bassBoostLp[0]->process(bassBoostLp[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						bassBoostLp[1]->process(bassBoostLp[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (equalizerEnabled)
				{
					if (eqFIRReady == 1)
					{
						FIREq[0]->process(FIREq[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						FIREq[1]->process(FIREq[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (stereoWidenEnabled)
				{
					double outLR, outRL;
					for (i = 0; i < DSPbufferLength; i++)
					{
						outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
						outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
						inputBuffer[0][i] = outLR + outRL;
						inputBuffer[1][i] = outLR - outRL;
					}
				}
				if (fSurroundEnabled)
				{
					colorfulMusic.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				}
				if (reverbEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (convolverEnabled)
				{
					if (convolverReady == 1)
					{
						convolver[0]->process(convolver[0], inputBuffer[0], outputBuffer[0], DSPbufferLength);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 2)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 3)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						fullStereoConvolver[1]->process(fullStereoConvolver[1], inputBuffer[0], tempBuf[1], DSPbufferLength);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
					else if (convolverReady == 4)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						pthread_create(&rightconv1, 0, EffectDSPMain::threadingConvF1, (void*)&fullStconvparams1);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						pthread_join(rightconv1, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
				}
				if (analogModelEnable)
				{
					pthread_create(&righttube, 0, EffectDSPMain::threadingTube, (void*)&rightparams2);
					processTube(&tubeP[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
					pthread_join(righttube, 0);
				}
				if (bs2bEnabled == 1)
				{
					for (i = 0; i < DSPbufferLength; i++)
						BS2BProcess(&bs2b, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (IIRenabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						IIRFilterProcess(&iir, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (ramp < 1.0)
					ramp += 0.05;
				if (compressionEnabled)
					sf_compressor_process(&compressor, DSPbufferLength, inputBuffer[0], inputBuffer[1], inputBuffer[0], inputBuffer[1]);
				if (AXEnabled)
					analogX.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				if (viperddcEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
					{
						double sampleOutL = inputBuffer[0][i], sampleOutR = inputBuffer[1][i];
						for (int j = 0; j < usedSOSCount; j++)
							SOS_DF2_StereoProcess(sosPointer[j], sampleOutL, sampleOutR, &sampleOutL, &sampleOutR);
						outputBuffer[0][i] = sampleOutL;
						outputBuffer[1][i] = sampleOutR;
					}
				}
				else
				{
					memcpy(outputBuffer[0], inputBuffer[0], memSize);
					memcpy(outputBuffer[1], inputBuffer[1], memSize);
				}
				pos = 0;
			}
		}
		break;
	case 1:
		for (framePos = 0; framePos < actualFrameCount; framePos++)
		{
			framePos2x = framePos << 1;
			outputBuffer[0][pos] *= ramp;
			outputBuffer[1][pos] *= ramp;
			JLimiterProcess(&kLimiter, &outputBuffer[0][pos], &outputBuffer[1][pos]);
			if (outputBuffer[0][pos] > 1.0)
				outputBuffer[0][pos] = 1.0;
			if (outputBuffer[0][pos] < -1.0)
				outputBuffer[0][pos] = -1.0;
			if (outputBuffer[1][pos] > 1.0)
				outputBuffer[1][pos] = 1.0;
			if (outputBuffer[1][pos] < -1.0)
				outputBuffer[1][pos] = -1.0;
			inputBuffer[0][pos] = (double)in->f32[framePos2x];
			out->f32[framePos2x] = (float)outputBuffer[0][pos];
			inputBuffer[1][pos] = (double)in->f32[++framePos2x];
			out->f32[framePos2x] = (float)outputBuffer[1][pos];
			pos++;
			if (pos == DSPbufferLength)
			{
				if (spectrumEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
                        spectrumExtend.Process(&inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if(DSenabled)
					dynamicSystem.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				if (bassBoostEnabled)
				{
					if (bassLpReady > 0)
					{
						bassBoostLp[0]->process(bassBoostLp[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						bassBoostLp[1]->process(bassBoostLp[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (equalizerEnabled)
				{
					if (eqFIRReady == 1)
					{
						FIREq[0]->process(FIREq[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						FIREq[1]->process(FIREq[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (stereoWidenEnabled)
				{
					double outLR, outRL;
					for (i = 0; i < DSPbufferLength; i++)
					{
						outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
						outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
						inputBuffer[0][i] = outLR + outRL;
						inputBuffer[1][i] = outLR - outRL;
					}
				}
				if (fSurroundEnabled)
				{
					colorfulMusic.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				}
				if (reverbEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (convolverEnabled)
				{
					if (convolverReady == 1)
					{
						convolver[0]->process(convolver[0], inputBuffer[0], outputBuffer[0], DSPbufferLength);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 2)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 3)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						fullStereoConvolver[1]->process(fullStereoConvolver[1], inputBuffer[0], tempBuf[1], DSPbufferLength);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
					else if (convolverReady == 4)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						pthread_create(&rightconv1, 0, EffectDSPMain::threadingConvF1, (void*)&fullStconvparams1);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						pthread_join(rightconv1, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
				}
				if (analogModelEnable)
				{
					pthread_create(&righttube, 0, EffectDSPMain::threadingTube, (void*)&rightparams2);
					processTube(&tubeP[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
					pthread_join(righttube, 0);
				}
				if (bs2bEnabled == 1)
				{
					for (i = 0; i < DSPbufferLength; i++)
						BS2BProcess(&bs2b, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (IIRenabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						IIRFilterProcess(&iir, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (ramp < 1.0)
					ramp += 0.05;
				if (compressionEnabled)
					sf_compressor_process(&compressor, DSPbufferLength, inputBuffer[0], inputBuffer[1], inputBuffer[0], inputBuffer[1]);
				if (AXEnabled)
					analogX.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				if (viperddcEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
					{
						double sampleOutL = inputBuffer[0][i], sampleOutR = inputBuffer[1][i];
						for (int j = 0; j < usedSOSCount; j++)
							SOS_DF2_StereoProcess(sosPointer[j], sampleOutL, sampleOutR, &sampleOutL, &sampleOutR);
						outputBuffer[0][i] = sampleOutL;
						outputBuffer[1][i] = sampleOutR;
					}
				}
				else
				{
					memcpy(outputBuffer[0], inputBuffer[0], memSize);
					memcpy(outputBuffer[1], inputBuffer[1], memSize);
				}
				pos = 0;
			}
		}
		break;
	case 2:
		for (framePos = 0; framePos < actualFrameCount; framePos++)
		{
			framePos2x = framePos << 1;
			outputBuffer[0][pos] *= ramp;
			outputBuffer[1][pos] *= ramp;
			JLimiterProcess(&kLimiter, &outputBuffer[0][pos], &outputBuffer[1][pos]);
			if (outputBuffer[0][pos] > 1.0)
				outputBuffer[0][pos] = 1.0;
			if (outputBuffer[0][pos] < -1.0)
				outputBuffer[0][pos] = -1.0;
			if (outputBuffer[1][pos] > 1.0)
				outputBuffer[1][pos] = 1.0;
			if (outputBuffer[1][pos] < -1.0)
				outputBuffer[1][pos] = -1.0;
			inputBuffer[0][pos] = (double)in->s32[framePos2x] * 4.656612875245797e-10;
			out->s32[framePos2x] = (int32_t)(outputBuffer[0][pos] * 2147483647.0);
			inputBuffer[1][pos] = (double)in->s32[++framePos2x] * 4.656612875245797e-10;
			out->s32[framePos2x] = (int32_t)(outputBuffer[1][pos] * 2147483647.0);
			pos++;
			if (pos == DSPbufferLength)
			{
				if (spectrumEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
                        spectrumExtend.Process(&inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if(DSenabled)
					dynamicSystem.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				if (bassBoostEnabled)
				{
					if (bassLpReady > 0)
					{
						bassBoostLp[0]->process(bassBoostLp[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						bassBoostLp[1]->process(bassBoostLp[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (equalizerEnabled)
				{
					if (eqFIRReady == 1)
					{
						FIREq[0]->process(FIREq[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						FIREq[1]->process(FIREq[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (stereoWidenEnabled)
				{
					double outLR, outRL;
					for (i = 0; i < DSPbufferLength; i++)
					{
						outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
						outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
						inputBuffer[0][i] = outLR + outRL;
						inputBuffer[1][i] = outLR - outRL;
					}
				}
				if (fSurroundEnabled)
				{
					colorfulMusic.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				}
				if (reverbEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (convolverEnabled)
				{
					if (convolverReady == 1)
					{
						convolver[0]->process(convolver[0], inputBuffer[0], outputBuffer[0], DSPbufferLength);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 2)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 3)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						fullStereoConvolver[1]->process(fullStereoConvolver[1], inputBuffer[0], tempBuf[1], DSPbufferLength);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
					else if (convolverReady == 4)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						pthread_create(&rightconv1, 0, EffectDSPMain::threadingConvF1, (void*)&fullStconvparams1);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						pthread_join(rightconv1, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
				}
				if (analogModelEnable)
				{
					pthread_create(&righttube, 0, EffectDSPMain::threadingTube, (void*)&rightparams2);
					processTube(&tubeP[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
					pthread_join(righttube, 0);
				}
				if (bs2bEnabled == 1)
				{
					for (i = 0; i < DSPbufferLength; i++)
						BS2BProcess(&bs2b, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (IIRenabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						IIRFilterProcess(&iir, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (ramp < 1.0)
					ramp += 0.05;
				if (compressionEnabled)
					sf_compressor_process(&compressor, DSPbufferLength, inputBuffer[0], inputBuffer[1], inputBuffer[0], inputBuffer[1]);
				if (AXEnabled)
					analogX.Process(inputBuffer[0], inputBuffer[1], DSPbufferLength);
				if (viperddcEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
					{
						double sampleOutL = inputBuffer[0][i], sampleOutR = inputBuffer[1][i];
						for (int j = 0; j < usedSOSCount; j++)
							SOS_DF2_StereoProcess(sosPointer[j], sampleOutL, sampleOutR, &sampleOutL, &sampleOutR);
						outputBuffer[0][i] = sampleOutL;
						outputBuffer[1][i] = sampleOutR;
					}
				}
				else
				{
					memcpy(outputBuffer[0], inputBuffer[0], memSize);
					memcpy(outputBuffer[1], inputBuffer[1], memSize);
				}
				pos = 0;
			}
		}
		break;
	}
	inOutRWPosition = pos;
	return mEnable ? 0 : -ENODATA;
}
void EffectDSPMain::_loadDDC(char* ddc_str){

    stringEq = (char*)calloc(strlen(ddc_str), sizeof(char));
    strcpy(stringEq,ddc_str);
    if (!viperddcEnabled)
    {
        if (sosCount)
        {
            for (int i = 0; i < sosCount; i++)
            {
                free(df441[i]);
                free(df48[i]);
            }
            free(df441);
            df441 = 0;
            free(df48);
            df48 = 0;
            sosCount = 0;
            if (resampledSOSCount)
            {
                for (int i = 0; i < resampledSOSCount; i++)
                    free(dfResampled[i]);
                free(dfResampled);
                dfResampled = 0;
                resampledSOSCount = 0;
            }
            sosPointer = 0;
        }
    }

    sosCount = DDCParser(stringEq, &df441, &df48);

#ifdef DEBUG
    printf("[I] VDC num of SOS: %d\n", sosCount);
	printf("VDC df48[0].b0: %1.14f\n", (float)df48[0]->b0);
#endif
    free(stringEq);
	free(ddc_str);
    stringEq = 0;
    return;
}
void EffectDSPMain::_loadReverb(reverbdata_t *r2){
    r = r2;
    refreshReverb();
    return;
}
void EffectDSPMain::_loadConv(int impulseCutted,int channels,float convGaindB,float* ir){

    impChannels = channels;

    //9999: ALLOCATE
    previousimpChannels = impChannels;
    impulseLengthActual = impulseCutted / impChannels;
    if (convGaindB > 50.0)
        convGaindB = 50.0;

    //10004: COMPLETE
    int i, j, tempbufValue;
    tempbufValue = impulseLengthActual * impChannels;

    if (finalImpulse)
    {
        for (i = 0; i < previousimpChannels; i++)
            free(finalImpulse[i]);
        free(finalImpulse);
        finalImpulse = 0;
    }

	if(finalIR) {
		free(finalIR);
		finalIR = NULL;
	}
    finalIR = (double*)malloc(tempbufValue * sizeof(double));
    for(int i=0;i<tempbufValue;i++){
        finalIR[i] = (double)ir[i];
    }

    if (!finalImpulse)
    {

        finalImpulse = (double**)malloc(impChannels * sizeof(double*));
        for (i = 0; i < impChannels; i++)
        {
            double* channelbuf = (double*)malloc(impulseLengthActual * sizeof(double));
            if (!channelbuf)
            {
                convolverReady = -1;
                convolverEnabled = !convolverEnabled;
                free(finalImpulse);
                finalImpulse = 0;
            }
            double* p = finalIR + i;
            for (j = 0; j < impulseLengthActual; j++)
                channelbuf[j] = p[j * impChannels];
            finalImpulse[i] = channelbuf;
        }
        if (!refreshConvolver(DSPbufferLength))
        {
            convolverReady = -1;
            convolverEnabled = !convolverEnabled;
            if (finalImpulse)
            {
                for (i = 0; i < impChannels; i++)
                    free(finalImpulse[i]);
                free(finalImpulse);
                finalImpulse = 0;
            }
            if (finalIR)
            {
                free(finalIR);
                finalIR = 0;
            }
        }
    }
}