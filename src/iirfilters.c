/* Foobar2000 IIR DSP Filters port */
/* Taken from https://github.com/ptytb/foo_dsp_effect/blob/master/iirfilters.cpp */
#include <math.h>
#include "iirfilters.h"
#include <stddef.h>
#include <string.h>

#ifndef M_PI
#define M_PI  3.1415926535897932384626433832795
#endif
#ifndef sqr 
#define sqr(a) ((a) * (a))
#endif

//lynched from SoX >w>
void make_poly_from_roots(double const * roots, size_t num_roots, double * poly)
{
	size_t i, j;
	poly[0] = 1;
	poly[1] = -roots[0];
	memset(poly + 2, 0, (num_roots + 1 - 2) * sizeof(*poly));
	for (i = 1; i < num_roots; ++i)
		for (j = num_roots; j > 0; --j)
			poly[j] -= poly[j - 1] * roots[i];
}

void IIRFilterInit(iirfilter_t *iir, int samplerate, int filter_type)
{
	iir->lfs.xn1[0]=0;
	iir->lfs.xn2[0]=0;
	iir->lfs.yn1[0]=0;
	iir->lfs.yn2[0]=0;
	iir->lfs.xn1[1]=0;
	iir->lfs.xn2[1]=0;
	iir->lfs.yn1[1]=0;
	iir->lfs.yn2[1]=0;

	//limit to nyquist
	if (iir->pf_freq > ((int)samplerate / 2.0)) iir->pf_freq = (int)(samplerate / 2.0);

	if (filter_type == RIAA_CD)
	{
		//thanks to lvqcl for coeffs
		switch (samplerate)
		{
		case 44100:
			iir->pf_gain = -9.465;
			iir->pf_qfact = 0.4850;
			iir->pf_freq = 5277;
			break;
		case 48000:
			iir->pf_gain = -9.605;
			iir->pf_qfact = 0.4858;
			iir->pf_freq = 5356;
			break;
		default: // we cannot be here but ...
			iir->pf_gain = -9.465;
			iir->pf_qfact = 0.4850;
			iir->pf_freq = 5277;
			break;
		}
	}
	
	iir->omega = 2 * M_PI * iir->pf_freq/samplerate;
	iir->cs = cos(iir->omega);
	iir->sn = sin(iir->omega);
	iir->A = exp(log(10.0) * iir->pf_gain  / 40);
	iir->beta = 2* sqrt(iir->A);

	iir->a1pha = iir->sn / (2.0 * iir->pf_qfact);

	//Set up filter coefficients according to type
	switch (filter_type)
	{
	case LPF:
		iir->b0 =  (1.0 - iir->cs) / 2.0 ;
		iir->b1 =   1.0 - iir->cs ;
		iir->b2 =  (1.0 - iir->cs) / 2.0 ;
		iir->a0 =   1.0 + iir->a1pha ;
		iir->a1 =  -2.0 * iir->cs ;
		iir->a2 =   1.0 - iir->a1pha ;
		break;
	case HPF:
		iir->b0 =  (1.0 + iir->cs) / 2.0 ;
		iir->b1 = -(1.0 + iir->cs) ;
		iir->b2 =  (1.0 + iir->cs) / 2.0 ;
		iir->a0 =   1.0 + iir->a1pha ;
		iir->a1 =  -2.0 * iir->cs ;
		iir->a2 =   1.0 - iir->a1pha ;
		break;
	case APF:
		iir->b0=1.0-iir->a1pha;
		iir->b1=-2.0*iir->cs;
		iir->b2=1.0+iir->a1pha;
		iir->a0=1.0+iir->a1pha;
		iir->a1=-2.0*iir->cs;
		iir->a2=1.0-iir->a1pha;
		break;
	case BPZPGF:
		iir->b0 =   iir->a1pha ;
		iir->b1 =   0.0 ;
		iir->b2 = -iir->a1pha ;
		iir->a0 =   1.0 + iir->a1pha ;
		iir->a1 =  -2.0 * iir->cs ;
		iir->a2 =   1.0 - iir->a1pha ;
		break;
	case BPCSGF:
		iir->b0=iir->sn/2.0;
		iir->b1=0.0;
		iir->b2=-iir->sn/2;
		iir->a0=1.0+iir->a1pha;
		iir->a1=-2.0*iir->cs;
		iir->a2=1.0-iir->a1pha;
	break;
	case NOTCH: 
		iir->b0 = 1;
		iir->b1 = -2 * iir->cs;
		iir->b2 = 1;
		iir->a0 = 1 + iir->a1pha;
		iir->a1 = -2 * iir->cs;
		iir->a2 = 1 - iir->a1pha;
		break;
	case RIAA_phono: /* http://www.dsprelated.com/showmessage/73300/3.php */
		if (samplerate == 44100) {
			static const double zeros[] = {-0.2014898, 0.9233820};
			static const double poles[] = {0.7083149, 0.9924091};
			make_poly_from_roots(zeros, (size_t)2, &iir->b0);
			make_poly_from_roots(poles, (size_t)2, &iir->a0);
		}
		else if (samplerate == 48000) {
			static const double zeros[] = {-0.1766069, 0.9321590};
			static const double poles[] = {0.7396325, 0.9931330};
			make_poly_from_roots(zeros, (size_t)2, &iir->b0);
			make_poly_from_roots(poles, (size_t)2, &iir->a0);
		}
		{ /* Normalise to 0dB at 1kHz (Thanks to Glenn Davis) */
			double y = 2 * M_PI * 1000 / samplerate ;
			double b_re = iir->b0 + iir->b1 * cos(-y) +iir->b2 * cos(-2 * y);
			double a_re = iir->a0 + iir->a1 * cos(-y) + iir->a2 * cos(-2 * y);
			double b_im = iir->b1 * sin(-y) + iir->b2 * sin(-2 * y);
			double a_im = iir->a1 * sin(-y) + iir->a2 * sin(-2 * y);
			double g = 1 / sqrt((sqr(b_re) + sqr(b_im)) / (sqr(a_re) + sqr(a_im)));
			iir->b0 *= g; iir->b1 *= g; iir->b2 *= g;
		}
		break;
	case PEQ: 
		iir->b0 =   1 + iir->a1pha * iir->A ;
		iir->b1 =  -2 * iir->cs ;
		iir->b2 =   1 - iir->a1pha * iir->A ;
		iir->a0 =   1 + iir->a1pha / iir->A ;
		iir->a1 =  -2 * iir->cs ;
		iir->a2 =   1 - iir->a1pha / iir->A ;
		break; 
	case BBOOST:       
		iir->beta = sqrt((iir->A * iir->A + 1) / 1.0 - (pow((iir->A - 1), 2)));
		iir->b0 = iir->A * ((iir->A + 1) - (iir->A - 1) * iir->cs + iir->beta * iir->sn);
		iir->b1 = 2 * iir->A * ((iir->A - 1) - (iir->A + 1) * iir->cs);
		iir->b2 = iir->A * ((iir->A + 1) - (iir->A - 1) * iir->cs - iir->beta * iir->sn);
		iir->a0 = ((iir->A + 1) + (iir->A - 1) * iir->cs + iir->beta * iir->sn);
		iir->a1 = -2 * ((iir->A - 1) + (iir->A + 1) * iir->cs);
		iir->a2 = (iir->A + 1) + (iir->A - 1) * iir->cs - iir->beta * iir->sn;
		break;
	case LSH:
		iir->b0 = iir->A * ((iir->A + 1) - (iir->A - 1) * iir->cs + iir->beta * iir->sn);
		iir->b1 = 2 * iir->A * ((iir->A - 1) - (iir->A + 1) * iir->cs);
		iir->b2 = iir->A * ((iir->A + 1) - (iir->A - 1) * iir->cs - iir->beta * iir->sn);
		iir->a0 = (iir->A + 1) + (iir->A - 1) * iir->cs + iir->beta * iir->sn;
		iir->a1 = -2 * ((iir->A - 1) + (iir->A + 1) * iir->cs);
		iir->a2 = (iir->A + 1) + (iir->A - 1) * iir->cs - iir->beta * iir->sn;
		break;
	case RIAA_CD:
	case HSH:
		iir->b0 = iir->A * ((iir->A + 1) + (iir->A - 1) * iir->cs + iir->beta * iir->sn);
		iir->b1 = -2 * iir->A * ((iir->A - 1) + (iir->A + 1) * iir->cs);
		iir->b2 = iir->A * ((iir->A + 1) + (iir->A - 1) * iir->cs - iir->beta * iir->sn);
		iir->a0 = (iir->A + 1) - (iir->A - 1) * iir->cs + iir->beta * iir->sn;
		iir->a1 = 2 * ((iir->A - 1) - (iir->A + 1) * iir->cs);
		iir->a2 = (iir->A + 1) - (iir->A - 1) * iir->cs - iir->beta * iir->sn;
		break;
	default:
		break;
	}
}

void IIRFilterProcess(iirfilter_t *iir,  double *sampleL, double *sampleR)
{
	double outL, outR, inL, inR = 0;
	inL = *sampleL;
	inR = *sampleR;
	outL = (iir->b0 * inL + iir->b1 * iir->lfs.xn1[0] + iir->b2 * iir->lfs.xn2[0] - iir->a1 * iir->lfs.yn1[0] - iir->a2 * iir->lfs.yn2[0]) / iir->a0;
    outR = (iir->b0 * inR + iir->b1 * iir->lfs.xn1[1] + iir->b2 * iir->lfs.xn2[1] - iir->a1 * iir->lfs.yn1[1] - iir->a2 * iir->lfs.yn2[1]) / iir->a0;

	iir->lfs.xn2[0] = iir->lfs.xn1[0];
	iir->lfs.xn1[0] = inL;
	iir->lfs.yn2[0] = iir->lfs.yn1[0];
	iir->lfs.yn1[0] = outL;

	iir->lfs.xn2[1] = iir->lfs.xn1[1];
	iir->lfs.xn1[1] = inR;
	iir->lfs.yn2[1] = iir->lfs.yn1[1];
	iir->lfs.yn1[1] = outR;

	*sampleL = outL;
	*sampleR = outR;
}