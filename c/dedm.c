#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
#include "dmcommon.h"

FILE* fp;
wavinfo wi;
int delta=0;
void makededmwav(FILE* fpi, wavinfo* wav);

main(int argc, char** argv)
{
	int i, custom_delta=1;
	unsigned char* wavhead;
	
	if(argc<2)
		custom_delta=0;
	else
	{
		for(i=1;i<argc;i++)
		{
			int j;
			for(j=0;j<strlen(argv[i]);j++)
			{
				if(!isdigit(argv[i][j]))
				{
					custom_delta=0;
					break;
				}
			}
			if(custom_delta==1)
			{
				delta=atoi(argv[i]);
			}
		}
	}
	
	wavhead=malloc(36);
	fp=fopen("dm.wav","rb");
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
	
	if(!delta)
	{
		delta=DELTA;
		printf("Use default DELTA = %d\n",delta);
	}
	else
	{
		printf("Use custom DELTA = %d\n",delta);
	}
	
	makededmwav(fp, &wi);
	fclose(fp);
	free(wavhead);
}

void makededmwav(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1l=0,samp2l=0,samp1r=0,samp2r=0;
	int sampo=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dedm.wav","wb");
	if(!fpo)
	{
		printf("Open destination file failed!\n");
		return;
	}
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 4,1, fpi);
	samp1l=(samp[0]+((short)(samp[1])<<8));
	samp1r=(samp[2]+((short)(samp[3])<<8));
	
	samp1l=dmp1bit(samp1l,0,delta);
	samp1r=dmp1bit(samp1r,0,delta);
	
	samp[0]=samp1l&0xff;
	samp[1]=(samp1l&0xff00)>>8;
	samp[2]=samp1r&0xff;
	samp[3]=(samp1r&0xff00)>>8;	
	fwrite(samp, 4,1,fpo);
	
	while(fread(samp, 4,1,fpi))
	{
		samp2l=(samp[0]+((short)(samp[1])<<8));
		samp2r=(samp[2]+((short)(samp[3])<<8));
		//samp2l+=samp1l;
		//samp2r+=samp1r;
		samp2l=dmp1bit(samp2l,samp1l,delta);
		samp2r=dmp1bit(samp2r,samp1r,delta);		
		//sampo=2*samp2;

		
		samp[0]=samp2l&0xff;
		samp[1]=(samp2l&0xff00)>>8;
		samp[2]=samp2r&0xff;
		samp[3]=(samp2r&0xff00)>>8;

		fwrite(samp,4,1,fpo);
		samp1l=samp2l;
		samp1r=samp2r;
	}
	fclose(fpo);
}
