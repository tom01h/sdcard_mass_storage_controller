/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */

#include "lib.h"
volatile unsigned char *SDTXCMD = ((volatile unsigned char *)0x9a102000);
volatile unsigned char *SDRXCMD = ((volatile unsigned char *)0x9a102004);
volatile unsigned char *SDTXDAT = ((volatile unsigned char *)0x9a102008);
volatile unsigned char *SDRXDAT = ((volatile unsigned char *)0x9a10200c);
volatile unsigned char *SDSTATS = ((volatile unsigned char *)0x9a102010);
volatile unsigned char *SDCNTRL = ((volatile unsigned char *)0x9a102014);
volatile unsigned char *SDTIMER = ((volatile unsigned char *)0x9a102018);

#define BUSY 0x80
#define CRC_TOKEN 0x29
#define TRANSMISSION_FAILURE 1
#define TRANSMISSION_SUCCESSFUL 0

/*-----------------------------------------------------------------------*/
/* get command response
/*-----------------------------------------------------------------------*/

_Bool mmc_get_cmd_bigrsp (unsigned char rsp[15])
{
  unsigned char rtn_reg=0;
  unsigned char rtn_reg_timer=0;
  int arr_cnt=0;
  rtn_reg_timer= *SDTIMER;
  while(rtn_reg_timer != 0){
    rtn_reg = *SDSTATS;
    if(( rtn_reg & 0x2) != 0x2){ //RX Fifo not Empty
      rsp[arr_cnt]= *SDRXCMD;
      arr_cnt++;
    }
    if (arr_cnt==15)
      return 1;
    rtn_reg_timer= *SDTIMER;
  }
  return 0;
}

_Bool mmc_get_cmd_rsp (unsigned char rsp[15])
{
  volatile unsigned char rtn_reg=0;
  volatile unsigned char rtn_reg_timer=0;
  int arr_cnt=0;
  rtn_reg_timer= *SDTIMER;
  while (rtn_reg_timer != 0){
    rtn_reg = *SDSTATS;
    if(( rtn_reg & 0x2) != 0x2){ //RX Fifo not Empty
      rsp[arr_cnt]= *SDRXCMD;
      arr_cnt++;
    }
    if (arr_cnt==5)
      return 1;
    rtn_reg_timer= *SDTIMER;
  }
  return 0;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	return (*SDSTATS>>4);
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  DSTATUS stat;
  int result;

  unsigned char response[15];
  unsigned char rca[2];
  unsigned char rtn_reg=0x00;

  //Reset the hardware
  *SDCNTRL = 1;
  *SDCNTRL = 0;
  // CMD 0
  *SDTXCMD = 0x40;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  while ( (rtn_reg=*SDTIMER) != 0){}
  // CMD8
  *SDTXCMD = 0x48;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x01;
  *SDTXCMD = 0xAA;
  if (mmc_get_cmd_rsp(response) ){
  }else{
    return STA_NOINIT;
  }
    
  // ACMD41
  while((rtn_reg & BUSY)==0){
    *SDTXCMD=0x77;
    *SDTXCMD=0x00;
    *SDTXCMD=0x00;
    *SDTXCMD=0x00;
    *SDTXCMD=0x00;
    if(mmc_get_cmd_rsp(response) && (response[4]==0)){
      *SDTXCMD=0x69;
      *SDTXCMD=0x40;
      *SDTXCMD=0x30;
      *SDTXCMD=0x00;
      *SDTXCMD=0x00;
      if (mmc_get_cmd_rsp(response)){
        rtn_reg = response[0];
      }else{
        return STA_NOINIT;
      }
    }else{
      return STA_NOINIT;
    }
  }

  // CMD2
  *SDTXCMD=0xC2;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  if (!mmc_get_cmd_bigrsp(response)){
    return STA_NOINIT;
  }

  // CMD3
  *SDTXCMD=0x43;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  if (mmc_get_cmd_rsp(response)){
    rca[0] = response[0];
    rca[1] = response[1];
  }
  else{
    return STA_NOINIT;
  }

  // CMD7
  *SDTXCMD=0x47;
  *SDTXCMD=rca[0] ;
  *SDTXCMD=rca[1] ;
  *SDTXCMD=0x0f;
  *SDTXCMD=0x0f;
  if(!mmc_get_cmd_rsp(response)){
    return STA_NOINIT;
  }

  // ACMD6
  *SDTXCMD=0x77;
  *SDTXCMD=rca[0] ;
  *SDTXCMD=rca[1] ;
  *SDTXCMD=0;
  *SDTXCMD=0;
  if(!mmc_get_cmd_rsp(response)){
    return STA_NOINIT;
  }
  *SDTXCMD=0x46;
  *SDTXCMD=0;
  *SDTXCMD=0;
  *SDTXCMD=0;
  *SDTXCMD=0x02;
  if(!mmc_get_cmd_rsp(response)){
    return STA_NOINIT;
  }

  *SDCNTRL = 0x80;

  return (*SDSTATS>>4);
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
  DRESULT res;
  int result;

  unsigned char response[15];
  unsigned int var;
  unsigned char c;

  for(int i=0; i < count; i++){
    var = sector+i;

    *SDTXCMD=0x51;
    *SDTXCMD=(char)((var >> 24) & 0xFF);
    *SDTXCMD=(char)((var >> 16) & 0xFF);
    *SDTXCMD=(char)((var >> 8) & 0xFF);
    *SDTXCMD=(char)(var & 0xFF);
    if (!mmc_get_cmd_rsp(response)){
      return RES_ERROR;
    }

    for(int j=0; j<512; j++){
      buff[i*512+j]=*SDRXDAT;
    }

    // DUMMY CRC READ
    unsigned char rsp =  *SDSTATS & 0x08;
    while ( rsp != 0x08) {
      c=*SDRXDAT;
      rsp =  *SDSTATS & 0x08;
    }

  }

  return 0;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	return RES_PARERR;
}

