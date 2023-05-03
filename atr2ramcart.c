/*--------------------------------------------------------------------*/
/* ATR2RAMCART                                                        */
/* by GienekP                                                         */
/* (c) 2023                                                           */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
typedef unsigned char U8;
/*--------------------------------------------------------------------*/
#define RAMPROC 0x0100
/*--------------------------------------------------------------------*/
#define SEC128 1
#define SEC256 2
#define OLDADDR 0
#define NEWADDR 1
/*--------------------------------------------------------------------*/
#define BANKSIZE (2*8192)
#define CARMAX (8*1024*1024)
#define CLONESIZE (1024)
#define ATRMAX (4*1024*1024)
/*--------------------------------------------------------------------*/
#include "starter.h"
#include "SectorMap.h"
/*--------------------------------------------------------------------*/
void checkATR(const U8 *data, U8 *sector, unsigned int *type)
{
	if ((data[0]==0x96) && (data[1]==02))
	{
		if ((data[4]==0x80) && (data[5]==0x00))
		{
			*sector=SEC128;
		}
		else if ((data[4]==0x00) && (data[5]==0x01))
		{
			*sector=SEC256;
			if (data[2]&0x0F)
			{
				*type=3*128;
			}
			else
			{
				*type=6*128;
			};
		};
	};
}
/*--------------------------------------------------------------------*/
unsigned int loadATR(const char *filename, U8 *data, U8 *sector, unsigned int *type)
{
	U8 header[16];
	unsigned int ret=0;
	int i;
	FILE *pf;
	for (i=0; i<ATRMAX; i++)
	{
		data[i]=0xFF;
	};
	pf=fopen(filename,"rb");
	if (pf)
	{
		i=fread(header,sizeof(U8),16,pf);
		if (i==16)
		{
			checkATR(header,sector,type);
			if (*sector==SEC256)
			{
				ret=fread(data,sizeof(U8),ATRMAX,pf);
			}
			else
			{
				printf("Wrong sector header.\n");
			};
		}
		else
		{
			printf("Wrong ATR header size.\n");
		}
		fclose(pf);
	}
	else
	{
		printf("\"%s\" does not exist.\n",filename);
	};
	return ret;
}
/*--------------------------------------------------------------------*/
void replace(U8 mode, U8 *atrdata, unsigned int i)
{
	static U8 f=0;
	if (mode==NEWADDR)
	{
		atrdata[i+1]=((RAMPROC)&0xFF);
		atrdata[i+2]=(((RAMPROC)>>8)&0xFF);
	};
	if (f==0)
	{
		f=1;
		if (mode==NEWADDR)
		{
			printf("Replace calls:\n");
		}
		else
		{
			printf("Possible calls:\n");
		};
	};
}
/*--------------------------------------------------------------------*/
void checkXINT(U8 *atrdata, U8 mode)
{
	unsigned int i;
	for (i=0; i<ATRMAX-3; i++)
	{
		if ((atrdata[i+1]==0x53) && (atrdata[i+2]==0xE4))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i);
				printf(" JSR JDSKINT ; 0x%06X 20 53 E4 -> 20 00 01\n",i+16);
				
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i);
				printf(" JMP JDSKINT ; 0x%06X 4C 53 E4 -> 4C 00 01\n",i+16);
			};			
		};
		if ((atrdata[i+1]==0xB3) && (atrdata[i+2]==0xC6))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i);
				printf(" JSR DSKINT ; 0x%06X 20 B3 C6 -> 20 00 01\n",i+16);
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i);
				printf(" JMP DSKINT ; 0x%06X 4C B3 C6 -> 4C 00 01\n",i+16);
			};			
		};
		if ((atrdata[i+1]==0x59) && (atrdata[i+2]==0xE4))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i);
				printf(" JSR JSIOINT ; 0x%06X 20 59 E4 -> 20 00 01\n",i+16);
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i);
				printf(" JMP JSIOINT ; 0x%06X 4C 59 E4 -> 4C 00 01\n",i+16);
			};			
		};
		if ((atrdata[i+1]==0x33) && (atrdata[i+2]==0xC9))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i);
				printf(" JSR SIOINT ; 0x%06X 20 33 C9 -> 20 00 01\n",i+16);
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i);
				printf(" JMP SIOINT ; 0x%06X 4C 3 C9 -> 4C 00 01\n",i+16);
			};			
		};	
	};
}
/*--------------------------------------------------------------------*/
void buildCar(const U8 *loader, unsigned int stsize, 
                         const U8 *atrdata, unsigned int atrsize, 
                         U8 *cardata, unsigned int carsize, unsigned int type)
{
	unsigned int i,j,k=0;
	for (i=0; i<CARMAX; i++)
	{
		cardata[i]=0xFF;
	};
	for (j=0; j<(CARMAX/BANKSIZE); j++)
	{
		for (i=0; i<stsize; i++)
		{
			cardata[(j+1)*BANKSIZE-stsize+i]=loader[i];
		};
	};
	for (i=0; i<(SectorMap_dta_len/2); i++)
	{
		cardata[2*BANKSIZE+i]=SectorMap_dta[i*2];
		cardata[3*BANKSIZE+i]=SectorMap_dta[i*2+1];
	};
	k=4*BANKSIZE;
	for (i=0; i<3; i++)
	{
		for (j=0; j<128; j++)
		{
			cardata[k]=atrdata[128*i+j];
			k++;
		};
		k+=128;
	};
	for (i=(3*128); i<atrsize; i++)
	{
		cardata[k]=atrdata[i];
		k++;
		if ((k%16384)>=(16384-1024)) {k+=1024;};	
	};
}
/*--------------------------------------------------------------------*/
U8 saveCAR(const char *filename, const U8 *data, unsigned int carsize)
{
	U8 header[16]={0x43, 0x41, 0x52, 0x54, 0x00, 0x00, 0x00, 0x52,
		           0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
	U8 ret=0;
	int i;
	FILE *pf;
	unsigned int sum=0;
	for (i=0; i<carsize; i++) {sum+=data[i];};
	header[8]=((sum>>24)&0xFF);
	header[9]=((sum>>16)&0xFF);
	header[10]=((sum>>8)&0xFF);
	header[11]=(sum&0xFF);
	pf=fopen(filename,"wb");
	if (pf)
	{
		i=16;
		i=fwrite(header,sizeof(U8),16,pf);
		if (i==16)
		{
			i=fwrite(data,sizeof(U8),carsize,pf);
			if (i==carsize)
			{
				ret=1;
			};			
		};
		fclose(pf);
	};
	return ret;
}
/*--------------------------------------------------------------------*/
U8 atrdata[ATRMAX];
U8 cardata[CARMAX];
void atr2ramcart(const char *atrfn, const char *carfn)
{
	U8 sector;
	unsigned int atrsize=0;
	unsigned int type;
	atrsize=loadATR(atrfn,atrdata,&sector,&type);
	if (atrsize)
	{
		printf("Load \"%s\"\n",atrfn);
		printf("ATR size: %i\n",atrsize+16);
		if (sector==SEC256)
		{
			//checkXINT(atrdata,0);
			buildCar(starter_bin,starter_bin_len,atrdata,atrsize,cardata,CARMAX,type);
			if (saveCAR(carfn,cardata,CARMAX))
			{
				printf("Save \"%s\"\n",carfn);
			}
			else
			{
				printf("Save \"%s\" ERROR!\n",carfn);
			};
		}
		else
		{
			printf("Sector: unsuported\n");
		};
	}
	else
	{
		printf("Load \"%s\" ERROR!\n",atrfn);
	};
}
/*--------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{	
	printf("ATR2RAMCART - ver: %s\n",__DATE__);
	if (argc==3)
	{
		atr2ramcart(argv[1],argv[2]);
	}
	else
	{
		printf("(c) GienekP\n");
		printf("use:\natr2ramcart file.atr file.car\n");
	};
	printf("\n");
	return 0;
}
/*--------------------------------------------------------------------*/
