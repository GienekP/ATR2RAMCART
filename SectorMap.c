/*--------------------------------------------------------------------*/
/* SectorMap                                                          */
/* by GienekP                                                         */
/* (c) 2023                                                           */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
#define BANKSIZE (2*8192)
#define RAMCARTSIZE (4*1024*1024)
#define SECSIZE 256
#define PROCSIZE 1024
/*--------------------------------------------------------------------*/
const unsigned char maxpos=((BANKSIZE-PROCSIZE)/SECSIZE);
const unsigned int maxsec=(maxpos*(RAMCARTSIZE/BANKSIZE)-maxpos*4);
/*--------------------------------------------------------------------*/
unsigned int ramcartCrazyBits(unsigned int x)
{
	/*98 764325xx */
	/*   98764325 */
	/*   76542103 */
	unsigned int ret=0,a,b,c;
	a=((x<<1)&0x0E);
	b=((x&0x08)>>3);
	c=(x&0xF0);
	ret=(a|b|c);
	return ret;
}
/*--------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{	
	int i;
	unsigned int bank=4,hb,lb;
	unsigned char pos=0;
	FILE *pf;
	printf("SectorMap Generator\nMax Disk Size: %ibytes (%i sectors)\n",maxsec*SECSIZE,maxsec);
	pf=fopen("SectorMap.dta","wb");
	if (pf)
	{
		printf("Build SectorMap.dta\n");
		fputc(0xFF,pf);
		fputc(0xFF,pf);
		for (i=1; i<=maxsec; i++)
		{
			lb=(bank&0xFF);
			hb=((((bank>>8)&0x03)<<6)+pos)+0x80;
			lb=ramcartCrazyBits(lb);
			putc(lb,pf);
			putc(hb,pf);
			printf("Sector: %i -> LB: %X  HB: %X  Bank: %03X  Pos: %02X ($%04X 0x%05X)\n",i,lb,hb,bank,pos,SECSIZE*pos+0x8000,bank*BANKSIZE+SECSIZE*pos);
			pos++;
			if (pos==maxpos)
			{
				pos=0;
				bank++;
			};
		};	
		fclose(pf);
	};
	printf("\n");
	return 0;
}
/*--------------------------------------------------------------------*/
