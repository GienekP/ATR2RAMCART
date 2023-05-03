/*--------------------------------------------------------------------*/
/* CAR2ROM                                                            */
/* by GienekP                                                         */
/* (c) 2023                                                           */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
typedef unsigned char U8;
/*--------------------------------------------------------------------*/
#define CARMAX (32*1024*1024+16)
U8 data[CARMAX];
/*--------------------------------------------------------------------*/
void car2rom(const char *filecar, const char *filebin)
{	
	unsigned int i;
	FILE *pf;
	for (i=0; i<CARMAX; i++) {data[i]=0;};
	pf=fopen(filecar,"rb");
	if (pf)
	{
		i=fread(data,sizeof(U8),16,pf);
		i=fread(data,sizeof(U8),CARMAX-16,pf);
		fclose(pf);
		pf=fopen(filebin,"wb");
		if (pf)
		{		
			i=fwrite(data,sizeof(U8),i,pf);
			printf("Save \"%s\"\n",filebin);
			fclose(pf);
		};		
	};	
}
/*--------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{	
	printf("CAR2ROM - ver: %s\n",__DATE__);
	if (argc==3)
	{
		car2rom(argv[1],argv[2]);
	}
	else
	{
		printf("(c) GienekP\n");
		printf("use:\ncar2rom file.car file.rom\n");
	};
	printf("\n");
	return 0;
}
/*--------------------------------------------------------------------*/
