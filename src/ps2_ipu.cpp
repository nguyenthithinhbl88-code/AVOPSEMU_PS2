#include "ps2_ee.hpp"
#include <stdexcept>
#include <sstream>
namespace ps2 {
bool Ee::ipu_read(uint32_t a,unsigned n,uint64_t& v)const{
 if((n!=4&&n!=8)||(a&(n-1)))return false;
 switch(a){
 case 0x10002000:v=0;break; // no VDEC/FDEC result or running command in this subset
 case 0x10002004:if(n!=4)return false;v=0;break;
 case 0x10002010:if(n!=4)return false;v=ipu.control;break;
 case 0x10002020:if(n!=4)return false;v=ipu.bp;break;
 case 0x10002030:v=0;break; // reset TOP, no bitstream input/decoding implemented
 case 0x10002034:if(n!=4)return false;v=0;break;
 default:return false;
 }
 ipu.reads++;return true;
}
bool Ee::ipu_write(uint32_t a,unsigned n,uint64_t value){
 if((n!=4&&n!=8)||(a&(n-1)))return false;
 const uint32_t v=uint32_t(value);
 if(a==0x10002000){
  const unsigned cmd=v>>28;
  if(cmd!=0&&cmd!=9)throw std::runtime_error("IPU MPEG/bitstream command unsupported; only BCLR/SETTH bootstrap commands implemented");
  if(cmd==0)ipu.bp=v&127;
  else {ipu.threshold0=uint16_t(v&511);ipu.threshold1=uint16_t((v>>16)&511);}
  ipu.commands++;ipu.writes++;intc_stat|=1u<<8;return true;
 }
 if(a==0x10002010&&n==4){
  if(v&0x40000000){ipu.control=0;ipu.bp=0;ipu.resets++;}
  else {ipu.control=v&0x07f30000;if((ipu.control&0x30000)==0x30000)ipu.control=(ipu.control&~0x30000u)|0x10000;}
  ipu.writes++;return true;
 }
 // BP and TOP are produced by the IPU; software cannot write their state.
 if((a==0x10002020&&n==4)||a==0x10002030||(a==0x10002034&&n==4)){ipu.writes++;return true;}
 return false;
}
std::string Ee::ipu_diagnostic()const{
 std::ostringstream o;o<<"IPU bootstrap only: CTRL="<<std::hex<<ipu.control<<" BP="<<ipu.bp<<" TH0="<<ipu.threshold0<<" TH1="<<ipu.threshold1<<std::dec<<" commands="<<ipu.commands<<" resets="<<ipu.resets<<" reads="<<ipu.reads<<" writes="<<ipu.writes<<"\r\n";
 o<<"BCLR/SETTH complete synchronously with INTC IRQ8. No FIFO/DMA/MPEG, VDEC/FDEC, image output or starvation timing. CMD/TOP remain reset-zero in this subset.\r\n";return o.str();
}
}
