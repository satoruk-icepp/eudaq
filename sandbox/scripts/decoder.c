#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>


#define RAWSIZE 30787

unsigned char raw[RAWSIZE];
unsigned int ev[4][1924];
unsigned int dati[4][128][13];

bool convertTxt = true;

unsigned int gray_to_binary1(unsigned int gray)
{
  // Sandro's implementation
  
  unsigned int bit11, bit10, bit9, bit8, bit7, bit6, bit5, bit4, bit3, bit2, bit1, bit0;
  
  bit11 = (gray >> 11) & 1;
  bit10 = bit11 ^ ((gray >>10) &1);
  bit9 = bit10 ^ ((gray >>9) &1);
  bit8 = bit9 ^ ((gray >>8) &1);
  bit7 = bit8 ^ ((gray >>7) &1);
  bit6 = bit7 ^ ((gray >>6) &1);
  bit5 = bit6 ^ ((gray >>5) &1);
  bit4 = bit5 ^ ((gray >>4) &1);
  bit3 = bit4 ^ ((gray >>3) &1);
  bit2 = bit3 ^ ((gray >>2) &1);
  bit1 = bit2 ^ ((gray >>1) &1);
  bit0 = bit1 ^ ((gray >>0) &1);

  return ((bit11 << 11) + (bit10 << 10) + (bit9 << 9) + (bit8 << 8)
	  + (bit7 << 7) + (bit6 << 6) + (bit5 << 5) + (bit4 << 4)
	  + (bit3  << 3) + (bit2 << 2) + (bit1  << 1) + bit0);
}



unsigned int gray_to_binary2(unsigned int gray)
{
  // Code from:
  //https://github.com/CMS-HGCAL/TestBeam/blob/826083b3bbc0d9d78b7d706198a9aee6b9711210/RawToDigi/plugins/HGCalTBRawToDigi.cc#L154-L170
  unsigned int result = gray & (1 << 11);
  result |= (gray ^ (result >> 1)) & (1 << 10);
  result |= (gray ^ (result >> 1)) & (1 << 9);
  result |= (gray ^ (result >> 1)) & (1 << 8);
  result |= (gray ^ (result >> 1)) & (1 << 7);
  result |= (gray ^ (result >> 1)) & (1 << 6);
  result |= (gray ^ (result >> 1)) & (1 << 5);
  result |= (gray ^ (result >> 1)) & (1 << 4);
  result |= (gray ^ (result >> 1)) & (1 << 3);
  result |= (gray ^ (result >> 1)) & (1 << 2);
  result |= (gray ^ (result >> 1)) & (1 << 1);
  result |= (gray ^ (result >> 1)) & (1 << 0);
  return result;

}

int decode_raw(){
  int i, j, k, m;
  unsigned char x;
  for( i = 0; i < 1924; i = i+1){
    for (k = 0; k < 4; k = k + 1){
      ev[k][i] = 0;
    }
  }
  for( i = 0; i < 1924; i = i+1){
    for (j=0; j < 16; j = j+1){
      x = raw[1 + i*16 + j];
      x = x & 15; // <-- APZ: Why is this needed?
      for (k = 0; k < 4; k = k + 1){
	ev[k][i] = ev[k][i] | (unsigned int) (((x >> (3 - k) ) & 1) << (15 - j));
      }
    }
  }
  
  /*****************************************************/
  /*    Gray to binary conversion                      */
  /*****************************************************/
  unsigned int t, bith;
  for(k = 0; k < 4 ; k = k +1 ){
    for(i = 0; i < 128*13; i = i + 1){
      bith = ev[k][i] & 0x8000;
      
      //t = gray_to_binary1(ev[k][i] & 0x7fff);
      t = gray_to_binary2(ev[k][i] & 0x7fff);
      ev[k][i] =  bith | t;
    }
  }
}

int format_channels(){
  int chip, hit, ch;
  
  for (chip =0; chip < 4; chip = chip +1 ){
    for (ch = 0; ch < 128; ch = ch +1 ){
      for (hit = 0 ; hit <13 ; hit = hit +1){
	dati[chip][ch][hit] = ev[chip][hit*128+ch] & 0x0FFF;
      }
    }
  }
  return(0);
}

void rollMaskPrint(unsigned int r){
  printf("Roll mask = %d \n", r);

  int k1 = -1, k2 = -1;
  for (int p=0; p<13; p++){
    printf("pos = %d, %d \n", p, r & (1<<12-p));
    if (r & (1<<12-p)) {
      if (k1==-1)
	k1 = p; 
      else if (k2==-1)
	k2 = p;
      else
	printf("Error: more than two positions in roll mask! %x \n",r);
    }
  }

  printf("k1 = %d, k2 = %d \n", k1, k2);

  // Check that k1 and k2 are consecutive
  char last = -1;
  if (k1==0 && k2==12) { last = 0;}
  else if (abs(k1-k2)>1)
    printf("The k1 and k2 are not consecutive! abs(k1-k2) = %d\n", abs(k1-k2));
  else
    last = k2;
  
  printf("last = %d\n", last);
  // k2+1 it the begin TS

  char mainFrameOffset = 5; // offset of the pulse wrt trigger (k2 rollmask) 
  char mainFrame = (last+mainFrameOffset)%13;

  
  printf("TS 0 to be saved: %d\n", (((mainFrame - 2) % 13) + ((mainFrame >= 2) ? 0 : 13))%13);
  printf("TS 1 to be saved: %d\n", (((mainFrame - 1) % 13) + ((mainFrame >= 1) ? 0 : 13))%13);
  printf("TS 2 to be saved: %d\n", mainFrame);
  printf("TS 3 to be saved: %d\n", (mainFrame+1)%13);
  printf("TS 4 to be saved: %d\n", (mainFrame+2)%13);
  

}

////////////////////////////
int main( int argc, char *argv[] )
{
  int res;
  
  int ch, sample, chip;
  int i, k, n;
  
  int maxevents = 10;
  char instr [1024];
  FILE *fraw;
  FILE *fout;
  
  /***********************************************************************/
  
  printf("How many events ? ");
  scanf("%d",&maxevents);
  
  fout = fopen("myOUT.txt", "w");
  fprintf(fout,"Total number of events: %d \n",maxevents); 

  char *fname = "RUN_170317_0912.raw";
  if( argc == 2 ) {
    printf("The argument supplied is %s\n", argv[1]);
    fname = argv[1];
  }
  
  fraw = fopen(fname, "r");
  
  clock_t begin = clock();

  if (!fraw){
    printf("Unable to open file!");
    return 1;
  }
  else {

    for (int i=0; i<maxevents; i++) {
      printf("Doing event #%i\n", i);      

      fread(raw, sizeof(raw), 1, fraw);

      printf("Event: %d.  First byte: %x, last byte: %x and the one before the last: %x\n",
	     i, raw[0], raw[RAWSIZE-1], raw[RAWSIZE-2]);
      
      res = decode_raw();      

      /*****************************************************/
      /* do some verification that data look OK on one chip*/
      /*****************************************************/
      chip= 1;
      for(k = 0; k < 1664; k = k + 1){
	if((ev[chip][k] & 0x8000 ) == 0){
	  printf("Wrong MSB at %d %x \n",k,ev[chip][k]);
	}
	if((ev[chip][k] & 0x7E00 ) != 0x0000){
	  printf("Wrong word at %d %d %x\n", i, k,ev[chip][k] );
	}
      }
      if(ev[chip][1923] != 0xc099){
	printf("Wrong Trailer is %x \n",ev[chip][1923]);
      }
      
      rollMaskPrint(ev[chip][1920]);


      if (convertTxt){
	/*****************************************************/
	/*           final convert to readable stuff         */
	/*****************************************************/
	res = format_channels();
	printf("*\n");
	fflush(stdout);
	
	/*****************************************************/
	/*             write event to data file              */
	/*****************************************************/
	for(chip = 0; chip < 4; chip = chip + 1){
	  fprintf(fout, "Event %d Chip %d RollMask %x \n",i, chip, ev[chip][1920]);
	  
	  for(ch =0; ch < 128; ch = ch +1){
	    for (sample=0; sample < 13; sample = sample +1){
	      fprintf(fout, "%d  ", dati[chip][ch][sample]);
	    }
	    fprintf(fout, "\n");
	  }	
	}
      }

    }
  }

  clock_t end = clock();
  double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Run time: %.3f \n", time_spent);
  
  fclose(fout);
  fclose(fraw);


  // Play with roll-mask

  unsigned int r = 0b000000000110000;

  rollMaskPrint(r);
  
  return(0);     
}
