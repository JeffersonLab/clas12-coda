/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
--
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Front End Unit
--
-- Module Name:    i2c_Max16031.h
-- Description:    Functions to control I2C Max16031 system monitor chip
--
-- Target Devices: V6 / FEU development kit
-- Tool versions:  Xilinx SDK 14.3
--
-- Create Date:    0.0 2013/05/31 IM
-- Revision:
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef I2C_MAX16031_H
#define I2C_MAX16031_H

int I2C_Max16031_ReadByte( int adr, unsigned char *c);
int I2C_Max16031_WriteByte(int adr, unsigned char c);

#endif // #ifndef I2C_MAX16031_H

