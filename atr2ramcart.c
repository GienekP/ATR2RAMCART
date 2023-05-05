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
#define CARMAX (4*1024*1024)
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
U8 upgrade(U8 *atrdata, unsigned int atrsize, const U8 *old, const U8 *new, unsigned int size)
{
	U8 ret;
	unsigned int i,j;
	for (i=0; i<(atrsize-size); i++)
	{
		ret=1;
		for (j=0; j<size; j++)
		{
			if (atrdata[i+j]!=old[j])
			{
				ret=0;
				j=size;
			};
		};
		if (ret==1)
		{
			printf("Upgrade (%i) ",i);
			for (j=0; j<size; j++)
			{
				atrdata[i+j]=new[j];
				printf("$%02X ",(int)(atrdata[i+j]));
			};
			printf("\n");
			i=ATRMAX;
		};
	};	
	return ret;
}
/*--------------------------------------------------------------------*/
void upgradeMyDOS(U8 *atrdata, unsigned int atrsize)
{
	// OLD DOS $07E0
	// NEW DOS $012D  - calculated
	const U8 old1[]={0x4D,0x03,0x00,0x07,0xE0,0x07,0x4C,0x14,0x07,0x03};
	const U8 new1[]={0x4D,0x03,0x00,0x07,0x2D,0x01,0x4C,0x14,0x07,0x03};
	const U8 old2[]={0xA9,0x07,0x85,0x0d,0xa9,0xE0,0x85,0x0c,0xa9,0x02};
	const U8 new2[]={0xA9,0x01,0x85,0x0d,0xa9,0x2D,0x85,0x0c,0xa9,0x02};
	upgrade(atrdata,atrsize,old1,new1,sizeof(old1)); 
	upgrade(atrdata,atrsize,old2,new2,sizeof(old2));
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
	U8 header[16]={0x43, 0x41, 0x52, 0x54, 0x00, 0x00, 0x00, 0x51,
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
			upgradeMyDOS(atrdata,atrsize);
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
