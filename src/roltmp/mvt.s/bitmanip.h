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

	#define GetMask(offset, length)              ( ((1<<(length))-1) << (offset) )
	#define GetBits(word, offset, length)        ( ((word) >> (offset)) & (GetMask(0,(length))) )
	#define ClrBits(word, offset, length)        (  (word) & (~(GetMask((offset), (length)))) )
	#define SetBits(word, offset, length)        (  (word) |   (GetMask((offset), (length))) )
	#define PutBits(word, offset, length, value) ( (ClrBits((word), (offset), (length))) | (((value) << (offset)) & (GetMask((offset),(length)))) )

#endif // BIT_MANIP_H
