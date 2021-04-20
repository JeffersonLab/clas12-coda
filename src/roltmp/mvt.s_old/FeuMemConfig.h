/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    FeuMemConfig.h
-- Description:    Functions to configure various FEU memories
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2012/09/07 IM
-- Revision:       1.0 2012/09/20 IM Trigger generator memory configuration added
--                 1.1 2014/03/29 IM Pedestal & Threshold memeory initialisation functions added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

// Pedestal and threshold memory initialisation
int PedMemInit( int feu, unsigned char feu_ip[4], short feu_port );
int ThrMemInit( int feu, unsigned char feu_ip[4], short feu_port );
// Pedestal and threshold memory configuration
int  FeuMemConfig( char *conf_file_name, int mem_type, int feu, unsigned char feu_ip[4], short feu_port );
// Trigger generator memory configuration
int  TrgMemConfig( char *conf_file_name, int feu, unsigned char feu_ip[4], short feu_port );
void MemConfig_CleanUp();