/*--------------------------------------------------------------------*/
/* RAM-CART to XEX - (c) GienekP                                      */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
#define BANKSIZE (16*1024)
/*--------------------------------------------------------------------*/
void prepareStart(FILE *file)
{
	const unsigned char start[]={0xFF,0xFF,0x2F,0x02,0x2F,0x02,0x00,0x00,0xD4,0x00,0xD4,0x00};
	fwrite(start,sizeof(unsigned char),sizeof(start),file);
}
/*--------------------------------------------------------------------*/
void prepareEnd(FILE *file)
{
	const unsigned char end[]={0xE0,0x02,0xE0,0x02,0x90,0xC2};
	fwrite(end,sizeof(unsigned char),sizeof(end),file);
}
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
void newBank(FILE *file, unsigned int bank)
{
	unsigned char config[10]={0x00,0xD5,0x01,0xD5,0xFF,0xFF,0x00,0x80,0xFF,0xBF};
	unsigned int D500,D501;
	D500=(((ramcartCrazyBits(bank&0x3F))<<2)|0x03);
	D501=((bank>>6)&0xFF);
	config[4]=D500;
	config[5]=D501;
	fwrite(config,sizeof(unsigned char),sizeof(config),file);
}
/*--------------------------------------------------------------------*/
void clearBuf(unsigned char *buf, unsigned int size)
{
	unsigned int i;
	for (i=0; i<size; i++) {buf[i]=0xFF;};
};
/*--------------------------------------------------------------------*/
void ramcart2xex(const char *fin, const char *fout)
{
	unsigned int i,fsize,bsize;
	unsigned char buf[BANKSIZE];
    FILE *fi,*fo;
    fi=fopen(fin,"rb");
    if (fi)
    {
		printf("Open \"%s\"\n",fin);
		fseek(fi,0,SEEK_END);
		fsize=ftell(fi);
		bsize=(fsize/BANKSIZE);
		printf("Input size: %i bytes\n",fsize);
		printf("No banks[16kB]: %i\n",bsize);
		fseek(fi,0,0);
		fo=fopen(fout,"wb");
		if (fo)
		{
			printf("XEX \"%s\"\n",fout);
			prepareStart(fo);
			for (i=0; i<bsize; i++)
			{
				newBank(fo,i);
				clearBuf(buf,BANKSIZE);
				fread(buf,sizeof(unsigned char),BANKSIZE,fi);
				fwrite(buf,sizeof(unsigned char),BANKSIZE,fo);	
			};
			prepareEnd(fo);
			fclose(fo);
		};
		fclose(fi);
	};
}
/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	printf("RAM-CART image to XEX - (c) GienekP\n");
	switch (argc)
	{
		case 3:
		{
			ramcart2xex(argv[1],argv[2]);
		} break;
		default:
		{
			printf("use:\n");
			printf("   ramcart2xex ramcartimage.bin output.xex\n");	
		} break;
	};
	printf("\n");
	return 0;
}
/*--------------------------------------------------------------------*/
