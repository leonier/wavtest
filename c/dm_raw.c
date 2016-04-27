#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
#include "dmcommon.h"

FILE* fp;
wavinfo wi;
int delta=0;
void makedm(FILE* fpi, wavinfo* wav);

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
	if(!delta)
	{
		delta=DELTA;
		printf("Use default DELTA = %d\n",delta);
	}
	else
	{
		printf("Use custom DELTA = %d\n",delta);
	}
	makedm(fp, &wi);
	fclose(fp);
	free(wavhead);
}

void makedm(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	dmarg *dma;
	int i;
	int wcount=0;
	unsigned int wbuf=0;
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1l=0,samp1r=0,samp2l=0,samp2r=0;
	short sampoutl=0, sampoutr=0;
	
	if(wav->channel!=2 || wav->bits!=16 )
		return;
		
	/*head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);*/
	
	fpo=fopen("test.dm","wb");
	if(!fpo)
	{
		printf("Open destination file failed!\n");
		return;
	}
	printf("Writing header %d bytes...\n",sizeof(wavinfo)+sizeof(dmarg));
	dma=malloc(sizeof(dmarg));
	dma->magicnum=MAGICNUM;
	dma->delta=DELTA;
	dma->mode=MODE_TYPE0;
	fwrite(dma,sizeof(dmarg),1,fpo);
	free(dma);
	fwrite(wav, sizeof(wavinfo), 1, fpo);

	
	fread(samp, 4,1, fpi);
	samp1l=(samp[0]+((short)(samp[1])<<8));
	samp1r=(samp[2]+((short)(samp[3])<<8));
	
	/*for(i=0;i<4;i++)
		samp[i]=0;
		
	fwrite(samp,4,1,fpo);*/
	
	while(fread(samp, 4,1,fpi))
	{
		samp2l=(samp[0]+((short)(samp[1])<<8));
		samp2r=(samp[2]+((short)(samp[3])<<8));
		

		//sampout=samp2/2-samp1/2;
		//sampoutl=samp2l-samp1l;		
		//sampoutr=samp2r-samp1r;
		
		//1-bit quantize
		sampoutl=quant1bit(samp1l,samp2l);
		sampoutr=quant1bit(samp1r,samp2r);
		
		wbuf = wbuf << 1;
		if(sampoutl)
			wbuf++;
		wbuf = wbuf << 1;	
		if(sampoutr)
			wbuf++;
		wcount+=2;
		
		if(wcount>=32)
		{
			fwrite(&wbuf,4,1,fpo);
			wbuf=0;
			wcount=0;
		}
/*		samp[0]=sampoutl&0xff;
		samp[1]=(sampoutl&0xff00)>>8;
		samp[2]=sampoutr&0xff;
		samp[3]=(sampoutr&0xff00)>>8;
		
		fwrite(samp,4,1,fpo);
		//samp1l=samp2l;
		//samp1r=samp2r;*/
		
		samp1l=dmp1bit(sampoutl,samp1l,delta);
		samp1r=dmp1bit(sampoutr,samp1r,delta);
	}
	if(wcount)
		fwrite(&wbuf,4,1,fpo);
	fclose(fpo);
}
