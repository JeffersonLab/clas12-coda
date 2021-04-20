/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
--
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Front End Unit
--
-- Module Name:    i2c_eeprom.h
-- Description:    Functions to control I2C EEPROMs
--
-- Target Devices: V6 / FEU development kit
-- Tool versions:  Xilinx SDK 14.3
--
-- Create Date:    0.0 2013/05/31 IM
-- Revision:
--
-- Comments: Based on the code developed by D. Calvet (calvet@hep.saclay.cea.fr)
--           for the T2K 280 m TPC readout
--
--------------------------------------------------------------------------------
*/
#ifndef I2C_EEPROM_H
#define I2C_EEPROM_H

int I2C_Eeprom_ReadByte( int adr, unsigned char *c);
int I2C_Eeprom_WriteByte(int adr, unsigned char c);

#endif // #ifndef I2C_EEPROM_H

