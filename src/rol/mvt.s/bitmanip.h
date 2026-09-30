///////////////////////////////////////////////////////////////////////////////
//
//    File Name:  bitmanip.h
//      Version:  0.0
//         Date:  28/10/04
//        Model:  header file defines bit manipulation macros
//
//      Company:  DAPNIA, CEA Saclay
//  Contributor:  IM
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BIT_MANIP_H
#define BIT_MANIP_H


//sergey: check 'length' to make compiler happy
//#define GetMask(offset, length)              ( ((1<<(length))-1) << (offset) )
#define GetMask(offset, length)             ( length<32 ? ( ((1<<(length))-1) << (offset) ) : 0 )

#define GetBits(word, offset, length)        ( ((word) >> (offset)) & (GetMask(0,(length))) )
#define ClrBits(word, offset, length)        (  (word) & (~(GetMask((offset), (length)))) )
#define SetBits(word, offset, length)        (  (word) |   (GetMask((offset), (length))) )

//sergey: check 'length' to make compiler happy
//#define PutBits(word, offset, length, value) ( (ClrBits((word), (offset), (length))) | (((value) << (offset)) & (GetMask((offset),(length)))) )
#define PutBits(word, offset, length, value) ( length<31 ? ( (ClrBits((word), (offset), (length))) | (((value) << (offset)) & (GetMask((offset),(length)))) ) : 0 )


#endif // BIT_MANIP_H
