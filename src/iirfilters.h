#ifndef	IIRFILTERS_H
#define	IIRFILTERS_H
#include <stddef.h>
/* filter types */
enum {
	LPF, /* low pass filter */
	HPF, /* High pass filter */
	BPCSGF,/* band pass filter 1 */
	BPZPGF,/* band pass filter 2 */
	APF, /* Allpass filter*/
	NOTCH, /* Notch Filter */
	RIAA_phono, /* RIAA record/tape deemphasis */
	PEQ, /* Peaking band EQ filter */
	BBOOST, /* Bassboost filter */
	LSH, /* Low shelf filter */
	RIAA_CD, /* CD de-emphasis */
	HSH /* High shelf filter */

};

typedef struct str_iirfilter
{
    double pf_freq, pf_qfact, pf_gain;
    int type, pf_q_is_bandwidth; 
    double omega, cs, a1pha, beta, b0, b1, b2, a0, a1,a2, A, sn;
	/* Buffer of last filtered sample: [0] 1-st channel, [1] 2-d channel */
	struct { double xn1[ 2 ], xn2[ 2 ], yn1[ 2 ], yn2[ 2]; } lfs;
} iirfilter_t;

void IIRFilterProcess(iirfilter_t *iir,  double *sampleL, double *sampleR);
void IIRFilterInit(iirfilter_t *iir, int samplerate, int filter_type);

//void IIRsetFrequency(float val);
//void IIRsetQuality(float val);
//void IIRsetGain(float val);

void make_poly_from_roots(double const * roots, size_t num_roots, double * poly);

#endif