#include <string.h>
#include <cstdio>
#include <cmath>
#include "process.h"
#include "siggen.h"
#include "param.h"

static float to_real(float x, float y)
{
    return sqrt(x*x+y*y);
};

typedef struct{float x,y;} float2;
#define MEM_SIZE 2048
static void avg(const float *data, float *chans, size_t discards=8)
{
    //extra work is done to avoid transients and partial sample rows
    memset(chans, 0, (CHANNELS/2+1)*sizeof(float));
    float2 *out = (float2*)data;
    memset(out, 0, (CHANNELS/2+1)*discards*sizeof(float2));
    size_t length = MEM_SIZE/2 - (MEM_SIZE/2%(CHANNELS/2+1));
    for(size_t rowidx=0,i=0;i<length;i++,rowidx++) {
        float smp = to_real(out[i].x, out[i].y);
        rowidx %= (CHANNELS/2+1);
        //printf("%c%f",rowidx?',':'\n',smp);
        chans[rowidx] += smp;
    }

    for(size_t i=0;i<=CHANNELS/2;++i)//Divide by length of avg and fft
        chans[i] /= CHANNELS*MEM_SIZE/(CHANNELS*2+4); //multiplication by 2 to finish norm
    //resulting values for pfb should be 0..1
}

float *genCosResponse(float *data, float frequency);
int main()
{
    float *data = new float[MEM_SIZE];
    //Assuming all parameters should be used for testing
    float stepSize = FS*0.01/CHANNELS;
    float chans[CHANNELS/2];

    for(unsigned i=0; i*stepSize<FS/2.0; ++i) {
        const float freq = i*stepSize;
        //printf("#frequency:=%f",freq);
        avg(genCosResponse(data, freq), chans);
        for(size_t i=0;i<=CHANNELS/2;++i)
            printf("%c%f",i?',':'\n',chans[i]* (!i||i==CHANNELS/2?0.0f:2.0f));
    }
    delete[] data;
}


float *genCosResponse(float *data, float frequency)
{
    //Zero Out memory
    memset(data, 0, sizeof(MEM_SIZE)*sizeof(float));

    //Generate Cosine
    gen_cos(data, MEM_SIZE, frequency);
    //printf("data[132] %f", data[132]);
    //*((char*)0x00);

    //Generate FIR coeffs [could have scope expanded]
    float fir[TAPS];
    gen_fir(fir, TAPS, CHANNELS);
    scale_fir(fir, TAPS);
    window_fir(fir, TAPS);

    //Execute pfb
    class Pfb *pfb = alloc_pfb(fir, TAPS, MEM_SIZE, CHANNELS);
    apply_pfb(data, pfb);
    delete_pfb(pfb);

    return data;
}

