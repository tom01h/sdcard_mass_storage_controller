#include "defines.h"
#include "serial.h"
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




int main(void)
{
  volatile unsigned char rtn_reg=0x80;
  volatile unsigned int spv_2_0 =0;
  unsigned char response[15];
  unsigned char rca[2];

  static unsigned char buf[32];

  puts("Hello World!\n");

  puts("TIMER INI  ");
  rtn_reg=*SDTIMER;
  putxval(rtn_reg,2);puts("\n");
  puts("STATUS INI  ");
  rtn_reg=*SDSTATS;
  putxval(rtn_reg,2);puts("\n");
  //Reset the hardware
  *SDCNTRL = 1;
  *SDCNTRL = 0;
  //Reset SD Card. CMD 0, Arg 0.
  //No response, wait for timeout
  *SDTXCMD = 0x40;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  while ( (rtn_reg=*SDTIMER) != 0){}
  //Check for SD 2.0 Card,
  *SDTXCMD = 0x48;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x00;
  *SDTXCMD = 0x01;
  *SDTXCMD = 0xAA;

  if (mmc_get_cmd_rsp(response) ){
    puts("CMD8 SUCCESSFUL ");
  }else{
    puts("CMD8 FAILURE\n");
    return 0;
  }
  putxval(response[0],2);puts(" ");
  putxval(response[1],2);puts(" ");
  putxval(response[2],2);puts(" ");
  putxval(response[3],2);puts(" ");
  putxval(response[4],2);puts("\n");
    
  rtn_reg=0x00;
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
        puts("ACMD41(CMD41) FAILURE\n");
        return 0;
      }
    }else{
      puts("ACMD41(CMD55) FAILURE\n");
      return 0;
    }
  }
  puts("ACMD41 SUCCESSFUL ");
  putxval(response[0],2);puts(" ");
  putxval(response[1],2);puts(" ");
  putxval(response[2],2);puts(" ");
  putxval(response[3],2);puts(" ");
  putxval(response[4],2);puts("\n");

  *SDTXCMD=0xC2;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  *SDTXCMD=0x00;
  if (!mmc_get_cmd_bigrsp(response)){
    puts("CMD2 FAILURE\n");
    return 0;
  }
  puts("CMD2 SUCCESSFUL ");
  putxval(response[0],2);puts(" ");
  putxval(response[1],2);puts(" ");
  putxval(response[2],2);puts(" ");
  putxval(response[3],2);puts(" ");
  putxval(response[4],2);puts(" ");
  putxval(response[5],2);puts(" ");
  putxval(response[6],2);puts(" ");
  putxval(response[7],2);puts("\n");

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
    puts("CMD3 FAILURE\n");
    return 0;
  }
  puts("CMD3 SUCCESSFUL ");
  putxval(response[0],2);puts(" ");
  putxval(response[1],2);puts(" ");
  putxval(response[2],2);puts(" ");
  putxval(response[3],2);puts(" ");
  putxval(response[4],2);puts("\n");

  *SDTXCMD=0x47;
  *SDTXCMD=rca[0] ;
  *SDTXCMD=rca[1] ;
  *SDTXCMD=0x0f;
  *SDTXCMD=0x0f;
  if(!mmc_get_cmd_rsp(response)){
    puts("CMD7 FAILURE\n");
    return 0;
  }
  puts("CMD7 SUCCESSFUL\n");


  *SDTXCMD=0x77;
  *SDTXCMD=rca[0] ;
  *SDTXCMD=rca[1] ;
  *SDTXCMD=0;
  *SDTXCMD=0;
  if(!mmc_get_cmd_rsp(response)){
    puts("ACMD6(CMD55) FAILURE\n");
    return 0;
  }
  *SDTXCMD=0x46;
  *SDTXCMD=0;
  *SDTXCMD=0;
  *SDTXCMD=0;
  *SDTXCMD=0x02;
  if(!mmc_get_cmd_rsp(response)){
    puts("ACMD6(CMD6) FAILURE\n");
    return 0;
  }
  puts("ACMD6 SUCCESSFUL\n");

  unsigned int var= 0 << 9;

  *SDTXCMD=0x51;
  *SDTXCMD=(char)((var >> 24) & 0xFF);
  *SDTXCMD=(char)((var >> 16) & 0xFF);
  *SDTXCMD=(char)((var >> 8) & 0xFF);
  *SDTXCMD=(char)(var & 0xFF);
  if (!mmc_get_cmd_rsp(response)){
    return TRANSMISSION_FAILURE;
  }

  unsigned char rsp =  *SDSTATS & 0x08;
  while ( rsp == 0x08) {
    rsp =  *SDSTATS & 0x08;
  }

  puts("READ BLOCK SUCCESSFUL\n");

  char c[512];
  for(int i=0; i<512; i++){
    c[i]=*SDRXDAT;
  }

  for (int i = 0; i < 512; i++) {
    putxval(c[i], 2);
    if ((i & 0xf) == 15) {
      puts("\n");
    } else {
      if ((i & 0x3) == 3) puts(" ");
      puts(" ");
    }
  }
  puts("\n");

  return 0;
}
