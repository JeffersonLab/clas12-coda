/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    Spi_Dream.h
-- Description:    Spi functions for pulser generator
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/09/22 IM
-- Revision:        
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef _SPI_Dream_H
#define _SPI_Dream_H

#define D_SPI_DREAM_READ  0
#define D_SPI_DREAM_WRITE 1

#define D_SPI_DREAM_ADR_REG_DUMMY      0
#define D_SPI_DREAM_ADR_REG_CONF1      1
#define D_SPI_DREAM_ADR_REG_CONF2      2
#define D_SPI_DREAM_ADR_REG_TSTSEL1    3
#define D_SPI_DREAM_ADR_REG_TSTSEL2    4
#define D_SPI_DREAM_ADR_REG_VERSION    5
#define D_SPI_DREAM_ADR_REG_GAIN1      6
#define D_SPI_DREAM_ADR_REG_GAIN2      7
#define D_SPI_DREAM_ADR_REG_TRIGINH1   8
#define D_SPI_DREAM_ADR_REG_TRIGINH2   9
#define D_SPI_DREAM_ADR_REG_SCARDINH1 10
#define D_SPI_DREAM_ADR_REG_SCARDINH2 11
#define D_SPI_DREAM_ADR_REG_TRIGLAT   12

#define D_SPI_DREAM_REG_NUM 13

extern unsigned char spi_dream_reg_size[];

#define Spi_DreamRegRead( dream, adr, val) Spi_DreamRegReadWrite(D_SPI_DREAM_READ,  dream, adr, val)
#define Spi_DreamRegWrite(dream, adr, val) Spi_DreamRegReadWrite(D_SPI_DREAM_WRITE, dream, adr, val)

int Spi_DreamRegReadWrite(int op, int dream, unsigned char adr, unsigned short val[4] );

#endif // _SPI_Dream_H
