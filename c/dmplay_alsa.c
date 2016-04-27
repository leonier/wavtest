//player for Linux ALSA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <alsa/asoundlib.h>

#include "tst.h"
#include "dmcommon.h"

FILE* fp;
wavinfo* wi;
dmarg* dminfo;

#define PCM_DEVICE "default"
#define BUFFER_NUM 64
#define SMPRATE 44100
short amplitude=32767;

unsigned char* buf = NULL;
snd_pcm_uframes_t frames;
snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
//snd_async_handler_t *pcm_callback;

void initsound(int samplerate, int channels);
int decodeDM(FILE* fpi,  wavinfo* wav, dmarg* dminfo, unsigned char* buffer, int buffer_size);

int main(int argc, char* argv[])
{
	int df, wf;
	
	if(argc<2)
	{
		printf("Usage: %s DM-file [wav-file]\n",argv[0]);
		exit(0);
	}
	dminfo=malloc(sizeof(dmarg));
	wi=malloc(sizeof(wavinfo));
	if(!dminfo||!wi)
	{
		printf("Memory Allocation Error!\n");
		exit(0);
	}		
	fp=fopen(argv[1],"rb");
	if(!fp)
	{
		printf("Error opening input file %s!\n", argv[1]);
		exit(0);
	}
	fread(dminfo,sizeof(dmarg),1,fp);
	if(dminfo->magicnum!=MAGICNUM)
	{
		printf("not a dm file!\n");
		exit(0);
	}
	if(dminfo->delta<0x10000)
		printf("DM delta value %d\n",dminfo->delta);
	else
	{
		int deltal=dminfo->delta&0xffff;
		int deltar=(dminfo->delta-deltal)/0x10000;
		printf("DM delta value %d/%d\n",deltal,deltar);
	}
	if(dminfo->mode==MODE_TYPE1)
		printf("Channel Delta Mode\n");
	if(dminfo->mode==MODE_TYPE2)
		printf("L+R L-R Mode\n");
	fread(wi,sizeof(wavinfo),1,fp);
	printf("Sample rate %dHz, %d bits, %d channels\n",wi->srate,wi->bits,wi->channel);
	printf("Length %d:%d\n",(wi->samples/wi->srate)/60,(wi->samples/wi->srate)%60);

	
	initsound(wi->srate, wi->channel);
	
	snd_pcm_prepare(pcm_handle);
	decodeDM(fp, wi, dminfo, buf, BUFFER_NUM*frames*2*wi->channel);
	snd_pcm_writei (pcm_handle, buf,  BUFFER_NUM*frames);
	snd_pcm_start(pcm_handle);	
	
	while(1)
	{
		df=decodeDM(fp, wi, dminfo, buf, BUFFER_NUM*frames*2*wi->channel);
		wf = snd_pcm_writei (pcm_handle, buf,  df);
		if(df<BUFFER_NUM*frames)
			break;
	}
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
	free(buf);
}

void initsound(int samplerate, int channels)
{
    
    int dir, pcmrc;


    /* Open the PCM device in playback mode */
    snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    /* Set parameters */
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
    snd_pcm_hw_params_set_rate(pcm_handle, params, samplerate, 0);

    /* Write parameters */
    snd_pcm_hw_params(pcm_handle, params);


	
    /* Allocate buffer to hold single period */
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    buf = malloc(frames * channels * 2 * BUFFER_NUM);
	
}

//read and decode DM stream.
int decodeDM(FILE* fpi,  wavinfo* wav, dmarg* dminfo, unsigned char* buffer, int buffer_size)
{
	int i,rc,pos;
	int rsamples,df=0;
	
	int wcount=0;
	unsigned int wbuf=0;
	unsigned int bmask=0x80000000;
	
	static int samp1l=0,samp1r=0;
	int sampl=0,sampr=0;
	int valuel=0,valuer=0;
	
	rsamples=(8*buffer_size)/((wav->bits)*(wav->channel));
	
	if(feof(fpi))
		return 0;
	
	switch(dminfo->mode)
	{
	int deltal, deltar;
	case MODE_TYPE0:
		if(dminfo->delta<65536)
		{
			deltal=deltar=dminfo->delta;
		}
		else
		{
			deltal=dminfo->delta&0xffff;
			deltar=(dminfo->delta-deltal)/0x10000;
		}
		for(i=0; i<rsamples; i++)
		{
			if(!wcount)
			{
				if(fread(&wbuf,sizeof(unsigned int),1,fpi)<1)
					break;
			}
				
			sampl=(wbuf&bmask)?1:0;
			bmask=bmask>>1;

			sampr=(wbuf&bmask)?1:0;	
			bmask=bmask>>1;	
			wcount+=2;
			if(wcount>31)
			{
				wcount=0;
				bmask=0x80000000;
			}
		

			sampl=dmp1bit(sampl,samp1l,deltal);
			sampr=dmp1bit(sampr,samp1r,deltar);
			
			pos=i*((wav->bits*wav->channel)/8);
			buffer[pos]=sampl&0xff;	
			buffer[pos+1]=(sampl&0xff00)>>8;
			buffer[pos+2]=sampr&0xff;
			buffer[pos+3]=(sampr&0xff00)>>8;
			
			samp1l=sampl;
			samp1r=sampr;
			df++;
		}
		break;
	case MODE_TYPE1:
		for(i=0; i<rsamples*2; i++)
		{
			if(!wcount)
			{
				if(fread(&wbuf,sizeof(unsigned int),1,fpi)<1)
					break;
			}
		
			sampl=(wbuf&bmask)?1:0;
			bmask=bmask>>1;

			wcount++;
			if(wcount>31)
			{
				wcount=0;
				bmask=0x80000000;
			}
		
			sampl=dmp1bit(sampl,samp1l,dminfo->delta);
			
			pos=i*((wav->bits)/8);
			buffer[pos]=sampl&0xff;	
			buffer[pos+1]=(sampl&0xff00)>>8;

			samp1l=sampl;
			
			df+=i%2;
		}
		break;	
	case MODE_TYPE2:
		for(i=0; i<rsamples; i++)
		{
			if(!wcount)
			{
				if(fread(&wbuf,sizeof(unsigned int),1,fpi)<1)
					break;
			}

			sampl=(wbuf&bmask)?1:0;
			bmask=bmask>>1;

			sampr=(wbuf&bmask)?1:0;	
			bmask=bmask>>1;	
			wcount+=2;
			if(wcount>31)
			{
				wcount=0;
				bmask=0x80000000;
			}
		
			sampl=dmp1bit(sampl,samp1l,dminfo->delta);
			sampr=dmp1bit(sampr,samp1r,dminfo->delta);
			
			valuel=(sampl+sampr);
			valuer=(sampr-sampl);
			
			pos=i*((wav->bits*wav->channel)/8);
			*(buffer+pos)=valuel&0xff;	
			*(buffer+pos+1)=(valuel&0xff00)>>8;
			*(buffer+pos+2)=valuer&0xff;
			*(buffer+pos+3)=(valuer&0xff00)>>8;

			samp1l=sampl;
			samp1r=sampr;
			df++;
		}
		break;
	}
	return df;
}