/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    Spi_Ad9222.h
-- Description:    Spi functions
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/09/20 IM
-- Revision:        
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef _SPI_AD9222_H
#define _SPI_AD9222_H

#include "SlowControl.h"

//Chip Configuration Registers
//      Reg                    Cmd     POR         Comment
#define SPI_AD9222_PORT_CONFIG 0x00 // 0x18  Configure port endianness
#define SPI_AD9222_ID          0x01 // 0x07  Read only
#define SPI_AD9222_GRADE       0x02 // N/A  Read only
//Device Index and Transfer Registers
//      Reg                    Cmd     POR         Comment
#define SPI_AD9222_INDEX2      0x04 // 0x0F  Which channel receives next command
#define SPI_AD9222_INDEX1      0x05 // 0x0F  Which channel receives next command
#define SPI_AD9222_UPDATE      0xFF // 0x00  Which channel receives next command
//ADC Functions
//      Reg                    Cmd     POR         Comment
#define SPI_AD9222_MODES       0x08 // 0x00  Determines modes of operation
#define SPI_AD9222_CLOCK       0x09 // 0x01  Duty cycle stabilizer
#define SPI_AD9222_TEST_IO     0x0D // 0x00  Activates test data

#define SPI_AD9222_OUT_MODE    0x14 // 0x00  Configures the outputs and the format of the data
#define SPI_AD9222_OUT_ADJ     0x15 // 0x00  Determines LVDS or other output properties.
#define SPI_AD9222_OUT_PHASE   0x16 // 0x03  Determines phase of the divider output

#define SPI_AD9222_UPAT1_LSB   0x19 // 0x00  User-defined pattern 1 LSB
#define SPI_AD9222_UPAT1_MSB   0x1A // 0x00  User-defined pattern 1 MSB
#define SPI_AD9222_UPAT2_LSB   0x1B // 0x00  User-defined pattern 2 LSB
#define SPI_AD9222_UPAT2_MSB   0x1C // 0x00  User-defined pattern 2 MSB

#define SPI_AD9222_SER_CTRL    0x21 // 0x00 Serial stream control
#define SPI_AD9222_SER_STAT    0x22 // 0x00 Used to power down individual sections of a converter

// Chip ID and grade register values
#define SPI_AD9222_CHIP_ID_VAL 0x07
#define SPI_AD9222_CHIP_GRADE_OFS  4
#define SPI_AD9222_CHIP_GRADE_LEN     3
#define SPI_AD9222_CHIP_GRADE_65   0
#define SPI_AD9222_CHIP_GRADE_50   3
#define SPI_AD9222_CHIP_GRADE_40   1

#define D_SPI_ADC_READ  0
#define D_SPI_ADC_WRITE 1

int Spi_AdcRegReadWrite(int op, unsigned short adr, unsigned char *val );

#define Spi_AdcRegRead( adr, val) Spi_AdcRegReadWrite(D_SPI_DREAM_READ,  adr, val)
#define Spi_AdcRegWrite(adr, val) Spi_AdcRegReadWrite(D_SPI_DREAM_WRITE, adr, val)

#endif // _SPI_AD9222_H
