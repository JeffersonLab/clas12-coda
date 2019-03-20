// You can use this function as a starting point for your decompression routine.

// 'data' is a single word (4-byte) from the FADC250.  ALL words pass through the function.
// A compressed data header (type = 8) for a 16 sample sector is recognized and decompression
// is performed when the last compressed data word is read.  Decompressed data is copied into 
// an array 'adc_samples_decomp'.  The array should be initialized to '0' for each event.
//
//	adc_samples_decomp = (unsigned int *)malloc( 16*NUM_SAMPLES*sizeof(unsigned int) );
//
//

static unsigned int type_last = 15;	/* initialize to type FILLER WORD */

static unsigned int chan_last = 99;	/* initialize to non-existant channel */
static unsigned int chan = 0;		

static unsigned int sector = 0;
static unsigned int comp_index = 0;
static unsigned int num_comp = 1;
static unsigned int num_up = 0;
static unsigned int lz_byte_up = 0;
static unsigned int min_value = 0;
static unsigned int comp_data[6] = {0,0,0,0,0,0};
static unsigned int num_samples = 0;

void decompress_reset()
{
	chan_last = 99;					// reset 'chan_last' for each EVENT allows
  num_samples = 0;
}

int decompress(unsigned int data, unsigned short *adc_samples_decomp)
{
  unsigned int up_byte[16];
  unsigned int add[16];
  int ii,jj;
  int sample[16];
  int byte_index, up_index, shift, index;
  
  unsigned int type, new_type, channel_offset = 0;
  	
/* Handles Hall-B compression - 10/18 - EJ */

// Detects ADC compressed data header (type 8) and sets up to decompress compressed data words 
// that follow (comp_index 

	if( comp_index )					// compressed data word
	{
            type = 17;						// compressed data words as type 17
            new_type = 0;
	    comp_data[comp_index - 1] = data;			// store compressed data words
	    // printf("----- comp data = %8X\n", data);
	    
	    if( comp_index < num_comp )
	    { 
	        comp_index++;
	    }
	    else						// LAST COMPRESSED WORD - DO DECOMPRESSION
	    { 
	        // printf("-------- Compressed data\n");
	        // for(ii=0;ii<num_comp;ii++)
	        //	printf("%8X\n", comp_data[ii]);
	        // printf("\n");
		
	        for(ii=0;ii<8;ii++)				// restore baseline from low array
	        {
	            jj = 4*ii; 
		    sample[ii]     = ( ( comp_data[0] >> jj) & 0xF ) + min_value;
		    sample[ii + 8] = ( ( comp_data[1] >> jj) & 0xF ) + min_value;
	        }
			
	        // printf("-------- Restored baseline from low array\n");
	        // for(ii=0;ii<16;ii++)
	        //    printf(" %6d",sample[ii]);
	        // printf("\n\n");
			
	        for(ii=0;ii<16;ii++)				// initialize arrays
	        {
	            up_byte[ii] = 0;
		    add[ii] = 0;
	        }
			
	        jj = 0;						// fill byte array
	        while( jj < num_up )
	        {
		    up_index = 2 + jj;
		    for(ii = 0; ii < 4; ii++)
		    {
		        byte_index = ii + (4 * jj);
		        shift = 8 * ii;
		        up_byte[byte_index] = (comp_data[up_index] >> shift) & 0xFF;
		    }
		    jj++;
	        }
	
	        ii = 0;						// fill add array
	        index = lz_byte_up;
	        while( index < 16 )
	        {
	            add[index] = up_byte[ii] << 4;
		    ii++;
		    index = ii + lz_byte_up;
	        }
			
	        // printf("-------- ADD array\n");
	        // for(ii=0;ii<16;ii++)
	        //    printf(" %6d",add[ii]);
	        // printf("\n\n");
			
	        for(ii=0;ii<16;ii++)				// final sample for sector
	        {
	            sample[ii] = sample[ii] + add[ii];
	        }
			
	        //printf("-------- Sample array\n");
	        // for(ii=0;ii<16;ii++)
	        //    printf(" %6d",sample[ii]);
	        // printf("\n\n");
			
          // Ignore channel_offset
	        //channel_offset = (NUM_SAMPLES - 1)*chan; 
		
	        for(ii=0;ii<16;ii++)				// copy sector to full decompressed array
	        {
        
        
		      *(adc_samples_decomp + channel_offset + 16*sector + ii) = sample[ii];
          num_samples++;
	        }
			
	        comp_index = 0;					// done - reset index
	        num_comp = 1;
	    } 
	}
	else							// normal typed word
	{	
            if( data & 0x80000000 )				// data type defining word
            {
            	new_type = 1;
            	type = (data & 0x78000000) >> 27;
            }
            else						// data type continuation word
            {
            	new_type = 0;
            	type = type_last;
            }
        
	    if( type == 2 )					// event header
	    {
	    	chan_last = 99;					// reset 'chan_last' for each EVENT allows
	    }							// decompression algorithm to distinguish
	    							// between sample sectors and consecutive
	    							// events with same channel number

//       -------------------------------------------------------------------------------------------------------	    							 
      if( type == 5 )					// ***** compressed data header *****
	    {
		    chan = (data >> 23) & 0xF;			// channel number
		    if( chan == chan_last )				// compressed data in sectors of 16 samples
		      sector++;					// same channel - increment sector
		    else
		    {
			    sector = 0;				// new channel - initialize to sector 0
			    chan_last = chan;			// save channel number
		    }
		
		    num_up = data & 0x7;				// number of words in up array
		    num_comp = 2 + num_up;				// number of compressed data words to follow header
				
		    min_value = (data & 0xFFF00) >> 8;		// minimum sample value
		    lz_byte_up = ((data & 0x200000) >> 17) | ((data & 0xF0) >> 4);	// number of leading zero bytes in up array
			
	        // printf("%8X - COMPRESSED DATA HEADER: CH = %d  NUM WORDS = %d  MIN = %d  LZ = %d  NUM UP = %d\n", 
	        //        	    data, chan, num_comp, min_value, lz_byte_up, num_up );

	      comp_index = 1;				// identify NEXT word as a 32-bit compressed value
	    }
//       -------------------------------------------------------------------------------------------------------	    							 
	
	    type_last = type;	    				// save type of current data word
	
	}
  return num_samples;
}        

