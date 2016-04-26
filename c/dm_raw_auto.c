#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
#include "dmcommon.h"
#define SLICES_PER_SECOND 30
#define DYN_ERROR 0.1

FILE* fp;
wavinfo wi;
int delta=0;
void makedm(FILE* fpi, wavinfo* wav);
void calcavgdyn(FILE* fpi, wavinfo* wav, double* adyn, double* mdyn);

main(int argc, char** argv)
{
	int i, custom_delta=1;
	double avgdyn;
	double maxdyn;
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
			if(custom_delta=1)
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
	calcavgdyn(fp, &wi, &avgdyn, &maxdyn);
	fclose(fp);

	printf("Average dynamic %f\n",avgdyn);
	printf("Max dynamic %f\n",maxdyn);

	//TODO!!!!
	if(!delta)
	{
		if(abs(maxdyn-avgdyn*3)<maxdyn*DYN_ERROR)
			delta=maxdyn;
		else
		{
			delta=(maxdyn-avgdyn*3)/2+avgdyn;
			while(delta<maxdyn*2/3)
			{
				delta+=avgdyn/3;
			}
		}
		printf("Use DELTA = %d\n",delta);
	}
	else
		printf("Use custom DELTA = %d\n",delta);
	
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
	dma->delta=delta;
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


void calcavgdyn(FILE* fpi, wavinfo* wav, double* adyn, double* mdyn)
{
	int i;
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1l=0,samp1r=0;
	short samp2l=0,samp2r=0;
	double suml=0.0, sumr=0.0;
	double asuml=0.0, asumr=0.0;
	double msuml=0.0, msumr=0.0;
	double adynl=0.0, adynr=0.0;
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

		if(samples%(wav->srate/SLICES_PER_SECOND )==0)
		{
		//	printf("%lf %lf\n", asuml, asumr);
			suml+=(asuml/(wav->srate/SLICES_PER_SECOND ));
			sumr+=(asumr/(wav->srate/SLICES_PER_SECOND ));
			if(msuml<(asuml/(wav->srate/SLICES_PER_SECOND )))
				msuml=(asumr/(wav->srate/SLICES_PER_SECOND ));
			if(msumr<(asumr/(wav->srate/SLICES_PER_SECOND )))
				msumr=(asumr/(wav->srate/SLICES_PER_SECOND ));
			asuml=0.0;
			asumr=0.0;
		}
	}
	
	adynl = suml/((samples/wav->srate)*SLICES_PER_SECOND );
	adynr = sumr/((samples/wav->srate)*SLICES_PER_SECOND );
	*adyn = (adynl+adynr)/2.0;
	*mdyn = (msuml+msumr)/2.0;
//	*mdynr = msumr;
}