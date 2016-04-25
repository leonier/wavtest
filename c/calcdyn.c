#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"

FILE* fp;
wavinfo wi;
int calcdyn(FILE* fpi, wavinfo* wav);

main()
{
	int maxdyn;
	unsigned char* wavhead;
	wavhead=malloc(36);
	fp=fopen("d.wav","rb");
	fread(wavhead,36,1,fp);
	if(!parsewaveheader(wavhead,&wi))
	{
		printf("WAV header error!\n");
		exit(0);
	}
	printf("WAV sample rate %d\nWAV bits per sample %d\nWAV channels %d\n",wi.srate, wi.bits,wi.channel);
	wi.datalen=finddatachunk(fp);
	wi.samples=wi.datalen/((wi.bits/8)*wi.channel);
	printf("WAV data length %d\n",wi.datalen);
	maxdyn=calcdyn(fp, &wi);
	printf("Max Dynamic range %d\n",maxdyn);
	fclose(fp);
	free(wavhead);
}

int calcdyn(FILE* fpi, wavinfo* wav)
{
	int i;
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1l=0,samp1r=0;
	int maxl=0, minl=0;
	int maxr=0, minr=0;
	
	if(wav->channel!=2 || wav->bits!=16 )
		return 0;
		
	while(fread(samp, 4,1,fpi))
	{
		samp1l=(samp[0]+((short)(samp[1])<<8));
		samp1r=(samp[2]+((short)(samp[3])<<8));
		
		if(maxl<samp1l)maxl=samp1l;
		if(maxr<samp1l)maxr=samp1r;
		
		if(minl>samp1l)minl=samp1l;
		if(minr<samp1l)minr=samp1r;
	}
	
	if((maxl-minl)<(maxr-minr))
	{
		return (maxr-minr);
	}	
	else
	{
		return (maxl-minl);
	}
}
