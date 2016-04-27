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
void makedm(FILE* fpi, wavinfo* wav, unsigned char* outfile);
void calcavgdyn(FILE* fpi, wavinfo* wav, double* adyn, double* mdyn);

main(int argc, char** argv)
{
	int i, custom_delta=1;
	double avgdyn;
	double maxdyn;
	unsigned char* wavhead;
	unsigned char* outfile;
	
	printf("leonier's DM encoder\n");
	
	if(argc<2)
	{
		printf("Usage: %s wav-file [DM-file] [custom DELTA value]\n",argv[0]);
		exit(0);
	}
	else
	{
		outfile=malloc(strlen(argv[1])+5);
		if(argc<=3)
		{
			strcpy(outfile,argv[1]);
			for(i=strlen(outfile); i>=0; i--)
			{
				if(*(outfile+i)=='.')
				{
					*(outfile+i)='\0';
					break;
				}
			}
			strcat(outfile,".dm");
			
			if(argc==3)
			{
				int j;
				for(j=0;j<strlen(argv[2]);j++)
				{
					if(!isdigit(argv[2][j]))
					{
						custom_delta=0;
						break;
					}
				}
				if(custom_delta==1)
				{
					delta=atoi(argv[2]);
				}
				else
					strcpy(outfile, argv[2]);
			}
		}
		else
		{
			int j;
			strcpy(outfile, argv[2]);
			for(j=0;j<strlen(argv[3]);j++)
			{
				if(!isdigit(argv[3][j]))
				{
					custom_delta=0;
					break;
				}
			}
			if(custom_delta==1)
			{
				delta=atoi(argv[3]);
			}				
			else
			{
				printf("DELTA value must be a number!\n");
				exit(0);
			}
		}
	}
	
	wavhead=malloc(36);
	fp=fopen(argv[1],"rb");
	if(!fp)
	{
		printf("Error opening input file %s!", argv[1]);
		exit(0);
	}
	fread(wavhead,36,1,fp);
	if(!parsewaveheader(wavhead,&wi))
	{
		printf("WAV header error!\n");
		exit(0);
	}
	printf("WAV sample rate %d\nWAV bits per sample %d\nWAV channels %d\n",wi.srate, wi.bits,wi.channel);
	wi.datalen=finddatachunk(fp);
	if(!wi.datalen)
	{
		printf("WAV data chunk not found!\n");
		exit(0);
	}
	wi.samples=wi.datalen/((wi.bits/8)*wi.channel);
	printf("WAV length %d:%d\n",(wi.samples/wi.srate)/60,(wi.samples/wi.srate)%60);
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
	
	fp=fopen(argv[1],"rb");
	fread(wavhead,36,1,fp);
	wi.datalen=finddatachunk(fp);
	wi.samples=wi.datalen/((wi.bits/8)*wi.channel);	
	makedm(fp, &wi, outfile);
	free(outfile);
	free(wavhead);
}

void makedm(FILE* fpi, wavinfo* wav, unsigned char* outfile)
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
		
	fpo=fopen(outfile,"wb");
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