/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    Spi_Pulser.h
-- Description:    Spi functions for pulser generator
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/09/21 IM
-- Revision:       1.1 2013/04/04 IM Pulser_Go function added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef _SPI_Pulser_H
#define _SPI_Pulser_H

// Pulser DAC read and writes
#define D_SPI_PULSER_READ  0
#define D_SPI_PULSER_WRITE 1

int Spi_PulserDacReadWrite(int op, unsigned short val );

#define Spi_PulserDacRead()     Spi_PulserDacReadWrite(D_SPI_PULSER_READ,   0 )
#define Spi_PulserDacWrite(val) Spi_PulserDacReadWrite(D_SPI_PULSER_WRITE, val)


// Pulser enables and disables

#define D_SPI_PULSER_DISABLE  0
#define D_SPI_PULSER_ENABLE   1

int Spi_PulserCtrl(int en);

#define Spi_PulserEnb() Spi_PulserCtrl(D_SPI_PULSER_DISABLE)
#define Spi_PulserDis() Spi_PulserCtrl(D_SPI_PULSER_ENABLE )

int Spi_PulserGo();

#endif // _SPI_Pulser_H
