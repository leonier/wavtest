#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
#include "dmcommon.h"

FILE* fp;
wavinfo wi;
int deltal,deltar;
void makedm(FILE* fpi, wavinfo* wav);
void calcavgdyn(FILE* fpi, wavinfo* wav, double* adynl, double* adynr, double* mdynl, double* mdynr);

main()
{
	double avgdynl,avgdynr;
	double maxdynl,maxdynr;
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
	calcavgdyn(fp, &wi, &avgdynl, &avgdynr, &maxdynl, &maxdynr);
	fclose(fp);

	printf("Average dynamic %f/%f\n",avgdynl,avgdynr);
	printf("Max dynamic %f/%f\n",maxdynl,maxdynr);

	//TODO!!!!
	if(maxdynl<avgdynl*4)
		deltal=maxdynl;
	else
	{
		deltal=(maxdynl-avgdynl*4)/2+avgdynl;
		while(deltal<maxdynl/3)
		{
			deltal+=avgdynl;
		}
	}
	if(maxdynr<avgdynr*4)
		deltar=maxdynr;
	else
	{
		deltar=(maxdynr-avgdynr*4)/2+avgdynr;
		while(deltar<maxdynr/3)
		{
			deltar+=avgdynl;
		}
	}
	printf("Use DELTA = %d/%d\n",deltal,deltar);

	
	fp=fopen("d.wav","rb");
	fread(wavhead,36,1,fp);
	wi.datalen=finddatachunk(fp);
	wi.samples=wi.datalen/((wi.bits/8)*wi.channel);	
	makedm(fp, &wi);
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
	dma->delta=deltal+(unsigned int)deltar*65536;
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
		
		samp1l=dmp1bit(sampoutl,samp1l,deltal);
		samp1r=dmp1bit(sampoutr,samp1r,deltar);
	}
	if(wcount)
		fwrite(&wbuf,4,1,fpo);
	fclose(fpo);
}


void calcavgdyn(FILE* fpi, wavinfo* wav, double* adynl, double* adynr, double* mdynl, double* mdynr)
{
	int i;
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1l=0,samp1r=0;
	short samp2l=0,samp2r=0;
	double suml=0.0, sumr=0.0;
	double asuml=0.0, asumr=0.0;
	double msuml=0.0, msumr=0.0;
	double l,r;
	unsigned int samples=0;

	if(wav->channel!=2 || wav->bits!=16 )
		return;
		
	while(fread(samp, 4,1,fpi))
	{
		samp1l=(samp[0]+((short)(samp[1])<<8));
		samp1r=(samp[2]+((short)(samp[3])<<8));
		
		l=abs(samp1l-samp2l);
		r=abs(samp1r-samp2r);

		asuml+=abs(samp1l-samp2l);
		asumr+=abs(samp1r-samp2r);
		samples++;
		samp2l=samp1l;
		samp2r=samp1r;

		if(samples%(wav->srate/20)==0)
		{
		//	printf("%lf %lf\n", asuml, asumr);
			suml+=(asuml/(wav->srate/20));
			sumr+=(asumr/(wav->srate/20));
			if(msuml<(asuml/(wav->srate/20)))
				msuml=(asumr/(wav->srate/20));
			if(msumr<(asumr/(wav->srate/20)))
				msumr=(asumr/(wav->srate/20));
			asuml=0.0;
			asumr=0.0;
		}
	}
	
	*adynl = suml/((samples/wav->srate)*20);
	*adynr = sumr/((samples/wav->srate)*20);
	*mdynl = msuml;
	*mdynr = msumr;
}