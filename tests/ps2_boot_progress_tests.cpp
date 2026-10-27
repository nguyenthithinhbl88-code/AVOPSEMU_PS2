#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"invalid access must fail");}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
for(unsigned i=0;i<16;i++){
 uint32_t a=0x1000f400+i*16;e->write(a,0x88776655,4);
 if(i!=1&&i!=3&&i!=4){ck(e->read(a,1)==0x55&&e->read(a+1,1)==0x66&&e->read(a+2,2)==0x8877,"MCH byte/halfword lane reads");e->write(a+1,0xaa,1);e->write(a+2,0xbbcc,2);ck(e->read(a,4)==0xbbccaa55,"MCH partial writes preserve other lanes");}
}
e->write(0xb000f480,0x88227c30,4);ck(e->read(0x9000f480,2)==0x7c30&&e->read(0xb000f482,2)==0x8822,"reported MCH LHU and aliases");
const auto reads=e->mch_reads,writes=e->mch_writes;fails([&]{e->read(0xb000f481,2);});fails([&]{e->write(0xb000f481,0,2);});fails([&]{e->read(0xb000f484,1);});fails([&]{e->write(0xb000f484,0,1);});fails([&]{e->read(0xb000f480,8);});ck(e->mch_reads==reads&&e->mch_writes==writes,"bad MCH widths/holes atomic");
e->reset(0x9fc42140);e->r[3].low=0xffffffffb0010000ull;e->write(0xb000f480,0x7c30,4);const uint32_t lhu=0x9463f480;for(unsigned n=0;n<4;n++)e->bios[0x42140+n]=uint8_t(lhu>>(8*n));e->step();ck(e->pc==0x9fc42144&&e->r[3].low==0x7c30&&e->first_fault.empty(),"reported negative-offset LHU advances and zero-extends");
// Fetch happens once, even when code is read from a side-effect-counted MMIO source.
e->reset(0xb000f100);e->serial.lcr=0;e->step();ck(e->serial.reads==1&&e->pc==0xb000f104,"single instruction fetch, no trace-induced second MMIO read");fails([&]{e->step();});ck(e->trace.size()==2&&e->trace.at(1).fetch_fault&&!e->first_fault.empty(),"fetch fault retained as binary trace entry");auto report=e->diagnostic();ck(report.find("b000f104 fetch fault")!=std::string::npos,"deferred fetch-fault formatting");
// Ring trace order and instruction word snapshots survive overwriting the source memory.
e->reset();for(unsigned i=0;i<100;i++)e->write(e->pc+i*4,0,4);for(unsigned i=0;i<100;i++){e->step();}ck(e->trace.size()==64&&e->trace.at(0).pc==0x100090&&e->trace.at(63).pc==0x10018c,"fixed trace ring retains chronological last 64 attempts");e->write(0x10018c,0xffffffff,4);ck(e->trace.at(63).word==0&&e->diagnostic().find("10018c ins=0 delay=0")!=std::string::npos,"trace preserves original opcode bits");
// Screenshot's seven-instruction Count loop runs normally; diagnostics only observe it.
e->reset(0x9fc42930);const uint32_t loop[]={0x40024800,0x00431023,0x0044102b,0,0,0x1440fffa,0};for(unsigned i=0;i<7;i++)for(unsigned n=0;n<4;n++)e->bios[0x42930+i*4+n]=uint8_t(loop[i]>>(8*n));e->r[3].low=0;e->r[4].low=30;
for(unsigned i=0;i<14;i++){e->step();}const auto pc=e->pc,count=e->cop0[9];const auto instructions=e->instructions;report=e->boot_status();ck(report.find("Observed Count polling loop")!=std::string::npos&&e->pc==pc&&e->cop0[9]==count&&e->instructions==instructions,"Count observation has no side effects");
for(unsigned i=0;i<100&&e->pc!=0x9fc4294c;i++){e->step();}ck(e->pc==0x9fc4294c&&e->instructions==42&&e->cop0[9]==42,"Count loop exits with exact executed instruction/tick count");
// Primary memory fast paths preserve byte order and edge behavior.
for(uint32_t a:{0x0u,0x1fffff8u,0x70003ff8u,0x1c1ffff8u,0x1100fff8u}){e->write(a,0x8877665544332211ull,8);ck(e->read(a,8)==0x8877665544332211ull&&e->read(a+2,2)==0x4433&&e->read(a+4,4)==0x88776655,"fast memory byte order and region edges");}
e->reset(0x100000);e->write(0x100000,0x46000000,4);fails([&]{e->step();});ck(e->trace.at(0).word==0x46000000&&e->pc==0x100000&&!e->instructions,"unsupported instruction still retains failing PC");
std::cout<<"PASS: MCH lane access and log LHU, single fetch, binary trace ring/faults, Count wait observation/exact exit and fast memory edges\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
