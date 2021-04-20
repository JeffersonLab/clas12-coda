/**
*   Copyright (c) 2017.  Jefferson Lab (JLab). All rights reserved. Permission
*   to use, copy, modify, and distribute  this software and its documentation for
*   educational, research, and not-for-profit purposes, without fee and without a
*   signed licensing agreement.
*
*   Author : G.Gavalian
*   Date   : 02/24/2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

/**
* Packs sample of 16 pulse elements into sequence of bytes.
* First byte represents the minimum number out of 16.
* then in each bin minimum is subtracted, and the resulting
* number is subdivided into lower 4 bits and higher 8 bits
* lower 4 bits are written into 8 bytes (16 samples), then
* a control byte is written idicating how many leading zeros
* are in the high 8 bit sample (upper 4 bits), and how many
* non zero elements in the higher 8 bit sample (lower 4 bits)
* the upper 8 bits array is then written in sequence.
* returns the number of bytes that were used to pack entire
* 16 sample pulse.
*/
int
pack16(const char *raw, int position, const char *encoded, int epos)
{
  uint8_t  high[16];
  uint8_t  highBits;
  uint8_t  lowBits_H, lowBits_L;
  int      bytesWritten = 0;
  int      i,j,result;
  int      leading_zeros, trailing_zeros;
  int      highBitsCount;

  uint16_t  *pointer = (uint16_t *) &raw[position];
  uint8_t   *enc     = (uint8_t *)  &encoded[0];
  uint16_t  *ped     = (uint16_t *) &encoded[epos];

  /* set output index on 3rd byte */
  result = epos+2;

  /* find minimum sample and put it in first 2 bytes */
  uint16_t min = pointer[0];
  for(i = 0; i < 16; i++) if(pointer[i]<min) min = pointer[i];
  ped[0] = (uint16_t) (min&0x0FFF);
  printf("position = %d , min = %d pedestal = %X\n", position,min,ped[0]);

  /* record low bits only for every sample, regardless of anything - 8 bytes, 4bits per sample */
  for(i = 0; i < 8; i++)
  {
    high[i*2  ] = ((pointer[ i*2  ] - min)>>4)&0xFF;
    high[i*2+1] = ((pointer[ i*2+1] - min)>>4)&0xFF;
    lowBits_H = (pointer[i*2] - min)&(0x0F);
    lowBits_L = (pointer[i*2 + 1] - min)&(0x0F);
    //if(position==64){
      enc[result] = (uint8_t) (lowBits_H<<4)|(lowBits_L);
    /*printf("%d :  low = %d , high = %d  encoded = %d [%X] (epos = %d )\n",
              i,lowBits_L,lowBits_H, enc[result],enc[result], result);*/
    //}
    result++;
  }

  /* count the number of samples with zero high part, starting from first sample; if 16 - return */
  leading_zeros = 0;
  j = 0;
  while(j<16&&high[j]==0) j++;
  leading_zeros = j;
  //printf("leading zeros = %d\n",leading_zeros);
  if(leading_zeros>=15) return result;

  /* count the number of samples with zero high part, starting from last sample going backward */
  j = 15;
  while(j>=0&&high[j]==0) j--;
  trailing_zeros = j;

  /* calculate how many samples with non-zero high part */
  highBitsCount = trailing_zeros - leading_zeros + 1;

  /* add '0x2000' to min sample value (first 2 bytes in output array) */
  ped[0] = (uint16_t) ((min&0x0FFF)|(0x2000));
  printf("ped[0]=0x%0x\n",ped[0]);
  //printf(" leading = %d trailing = %d  count = %d\n",
  // leading_zeros, trailing_zeros, highBitsCount);

  /* record the number of leading zeros and the number of samples with non-zero high part */
  enc[result] = (uint8_t) ((leading_zeros<<4)|highBitsCount);
  result++;

  /* record high parts of samples with non-zero high part */
  for(i = 0; i < highBitsCount; i++)
  {
    enc[result] = high[leading_zeros+i];
    //printf(" high item %d = %d\n",i,high[leading_zeros+i]);
    result++;
  }

  return(result);
}

/**
* Packs pulse into a char array. return number of bytes that were
* used by the packed pulse. The input buffer raw has to have a size
* of at least 200 bytes, 100 short words. The pulse is packed into
* groups of 16 samples (6 - 16 samples) 96 pulse samples in total.
*  arguments:
*    raw - buffer representing the raw pulse each 16 bits is one sample
*    encoded - buffer to write encoded data.
*  return:
*     number of bytes in the encoded buffer representing the pulse.
*/
int
pack(const int nsamples, const char *raw, const char *encoded)
{
  int i, epos = 0;
  int position = 0;
  int nframes = (nsamples/16)+1;

  //printf("pack: nsamples = %d, nframes = %d\n",nsamples,nframes);

  for(i = 0; i < nframes; i++)
  {
    //printf("pack: iter = %d epos = %d\n",i,epos);
    epos = pack16(raw, position, encoded, epos);
    position = position + 16*2; /* jump 16 samples (32 bytes) */
  }

  return(epos);
}
