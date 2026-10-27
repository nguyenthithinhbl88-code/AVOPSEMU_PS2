#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported BA0 request must fail");}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
 for(unsigned i=0;i<18;i++)if(i!=3){const auto a=0x1a000000+i*2;e->write(a|0xa0000000,i,2);ck(e->read(a|0x80000000,2)==i,"halfword bank and aliases");}
 e->write(0xba000006,3,2);ck(e->read(0xba000006,2)==0&&e->read(0xba000006,2)==0,"idle status no longer fabricates phase transitions");
 e->write(0xba000002,0x43,2);e->write(0xba000010,0x7e,2);e->write(0xba000012,0x1234,2);e->write(0xba000000,0x82,2);
 ck(e->dve_registers[0x7e]==0x1234,"DVE selected write");
 const auto reads=e->ba0_reads;const auto busy=e->ba0_busy_reads;const auto report=e->diagnostic();ck(e->ba0_reads==reads&&e->ba0_busy_reads==busy&&report.find("pending_status_reads=2")!=std::string::npos,"snapshot does not complete command");
 ck(e->read(0xba000006,2)==0&&e->read(0xba000006,2)==0&&e->read(0xba000006,2)==2,"bounded command completion");
 ck(e->read(0xba000006,2)==2,"ready sticky");e->write(0xba000006,0,2);ck(e->read(0xba000006,2)==0,"status acknowledgement");
 e->write(0xba000002,0x42,2);e->write(0xba000010,0x7e,2);e->write(0xba000000,0x81,2);ck(e->read(0x9a000010,2)==0x1234,"DVE read FIFO");
 e->write(0xba000002,0x41,2);e->write(0xba000000,0x81,2);ck(e->read(0xba000006,2)==1&&e->ba0_command_errors==1,"unsupported command error");
 e->write(0xba000002,0x43,2);e->write(0xba000000,0x8f,2);ck(e->ba0_command_errors==2,"FIFO count bounds");
 e->write(0xba00000a,1,2);ck(!(e->read(0xba000006,2)&1),"power-on clears error");
 for(unsigned n:{1u,4u,8u}){fails([&]{e->read(0xba000010,n);});fails([&]{e->write(0xba000010,0,n);});}
 fails([&]{e->read(0xba000023,2);});fails([&]{e->write(0xba000024,0,2);});fails([&]{e->write128(0xba000010,{0,0});});
 e->reset(0x80007c9c);e->r[4].low=3;e->r[18].low=0x7e;
 const uint32_t code[]={0x3c02ba00,0x34420006,0x3c03ba00,0xa4440000,0x34630002,0x24040042,0x3c02ba00,0xa4640000,0x34420010,0xa4520000};
 for(unsigned i=0;i<10;i++)e->write(0x80007c9c+i*4,code[i],4);
 for(unsigned i=0;i<10;i++)e->step();
 ck(e->pc==0x80007cc4&&e->instructions==10&&e->first_fault.empty()&&e->ba0_last_writes[8]==0x7e&&e->ba0_last_writes[1]==0x42,"exact ten-instruction user log tail now passes SH BA000010");
 ck(e->gs.regs[0]==0&&!e->gs.gif_words,"does not force display");e->reset();ck(!e->ba0_commands&&!e->ba0_busy_reads&&!e->dve_registers[0x7e],"reset clears bridge state");
 std::cout<<"PASS: BA0/DVE latches, FIFO bounds, command status, snapshots, aliases and exact log SH BA000010 -> 80007CC4\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
