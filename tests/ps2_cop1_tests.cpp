#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void code(ps2::Ee& e,uint32_t ins){e.write(0x100000,ins,4);e.pc=0x100000;e.next=0x100004;e.delay=false;e.first_fault.clear();}
void run(ps2::Ee& e,uint32_t ins){code(e,ins);e.step();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported instruction/access must fail");}
uint32_t transfer(unsigned format,unsigned rt,unsigned fs){return 0x44000000|(format<<21)|(rt<<16)|(fs<<11);}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
ck(e->fcr0==0x2e00&&e->fcr31==0x01000001,"FCR identity/fixed reset bits");
const uint32_t patterns[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0x7fc12345,0xffffffff,1};
for(unsigned f=0;f<32;f++)for(uint32_t bits:patterns){
 e->r[2]={0x1234567800000000ull|bits,0x1122334455667788ull};run(*e,transfer(4,2,f));ck(e->fpr[f]==bits,"MTC1 all FPRs including f0 and raw bit patterns");
 e->r[3]={0,0x8877665544332211ull};run(*e,transfer(0,3,f));ck(e->r[3].low==uint64_t(int64_t(int32_t(bits)))&&e->r[3].high==0x8877665544332211ull,"MFC1 sign extension preserves GPR upper half");
 const unsigned fd=(f+1)%32;run(*e,0x46000006|(f<<11)|(fd<<6));ck(e->fpr[fd]==bits,"MOV.S exact bits for all registers");
}
run(*e,transfer(0,0,1));ck(!e->r[0].low&&!e->r[0].high,"MFC1 r0 ignored");run(*e,transfer(4,0,0));ck(!e->fpr[0],"MTC1 zero to writable f0");
e->r[2].low=~0ull;run(*e,transfer(6,2,31));ck(e->fcr31==0x0183c079,"FCR31 writable mask/fixed bits");run(*e,transfer(2,3,31));ck(e->r[3].low==0x0183c079,"CFC1 status");run(*e,transfer(6,2,0));run(*e,transfer(2,3,0));ck(e->fcr0==0x2e00&&e->r[3].low==0x2e00,"read-only FCR0");
e->r[2].low=0;run(*e,transfer(6,2,31));ck(e->fcr31==0x01000001,"CTC1 clears software flags but retains fixed bits");
// All FPRs round trip raw bits through negative-offset loads/stores and kernel aliases.
for(unsigned f=0;f<32;f++){
 e->fpr[f]=0x80000000u+f;e->r[1].low=0xffffffff80000204ull;run(*e,0xe420fffc|(f<<16));ck(e->read(0x200,4)==0x80000000u+f,"SWC1 all FPRs and signed displacement");
 e->fpr[f]=0;e->r[1].low=0xffffffffa0000204ull;run(*e,0xc420fffc|(f<<16));ck(e->fpr[f]==0x80000000u+f,"LWC1 all FPRs raw bits through alias");
}
for(uint32_t a:{0x70000040u,0x1c000100u,0x1100c100u}){e->r[1].low=a;e->fpr[12]=0x7fc12345;run(*e,0xe42c0000);e->fpr[13]=0;run(*e,0xc42d0000);ck(e->fpr[13]==0x7fc12345,"FPU memory moves use shared scratch/IOP/VU bus");}
e->r[1].low=0xb000f100;e->fpr[12]=0x20;run(*e,0xe42c0000);ck(e->serial.lcr==0x20,"SWC1 uses MMIO bus");
for(unsigned cond=0;cond<2;cond++)for(unsigned kind=0;kind<4;kind++){
 e->fcr31=0x01000001|(cond?0x800000:0);run(*e,0x45000002|(kind<<16));const bool taken=bool(kind&1)==bool(cond);
 ck(e->pc==((kind&2)&&!taken?0x100008u:0x100004u)&&e->delay==(!((kind&2)&&!taken)),"BC1 likely annul versus delay slot");
 ck(e->next==(taken?0x10000cu:((kind&2)?0x10000cu:0x100008u)),"BC1 target/fallthrough");
}
e->r[1].low=0x200;e->fpr[12]=0x12345678;e->fcr31=0x01800001;code(*e,0x45010002);e->write(0x100004,0xe42c0000,4);e->step();e->step();ck(e->pc==0x10000c&&e->read(0x200,4)==0x12345678,"SWC1 executes in BC1 taken delay slot");
for(uint32_t ins:{transfer(2,2,1),transfer(6,2,30),transfer(0,2,1)|1u,0x46000000u,0x46200006u,0x45040000u}){
 const auto fpr=e->fpr;const auto fcr=e->fcr31;code(*e,ins);const auto count=e->instructions;fails([&]{e->step();});ck(e->fpr==fpr&&e->fcr31==fcr&&e->pc==0x100000&&e->instructions==count&&!e->first_fault.empty(),"unsupported COP1 no mutation/progress");
}
for(uint32_t ins:{0xc42c0000u,0xe42c0000u}){
 e->r[1].low=0x201;code(*e,ins);const auto old=e->fpr[12];fails([&]{e->step();});ck(e->fpr[12]==old&&e->pc==0x100000,"unaligned FPU transfer rejected");
 e->r[1].low=0x1f123000;code(*e,ins);fails([&]{e->step();});ck(e->fpr[12]==old,"unmapped FPU transfer preserves register");
}
for(uint32_t ins:{transfer(4,2,1),0xc42c0000u,0xe42c0000u,0x45000000u}){
 e->r[1].low=0x200;e->cop0[12]&=~0x20000000u;const auto before=e->read(0x200,4);const auto old=e->fpr;code(*e,ins);fails([&]{e->step();});ck(e->read(0x200,4)==before&&e->fpr==old,"CU1 unusable stops before memory/register side effects");e->cop0[12]|=0x20000000;
}
// Reported stack store, with nonzero f12 to ensure this is a real store, not a NOP.
e->reset(0x9fc434a4);e->r[29].low=0x70003e90;e->fpr[12]=0x89abcdef;uint32_t swc1=0xe7ac00b8;for(unsigned n=0;n<4;n++)e->bios[0x434a4+n]=uint8_t(swc1>>(8*n));e->step();ck(e->pc==0x9fc434a8&&e->read(0x70003f48,4)==0x89abcdef&&e->first_fault.empty(),"reported SWC1 stores to scratch stack and advances");
struct Entry{uint32_t pc,word;bool delay;};const Entry trace[]={
{0x9fc43460u,0x27bdff00u,false},
{0x9fc43464u,0xffb30050u,false},
{0x9fc43468u,0xffb20040u,false},
{0x9fc4346cu,0x80982du,false},
{0x9fc43470u,0xffbf0070u,false},
{0x9fc43474u,0x27b200c8u,false},
{0x9fc43478u,0xffb40060u,false},
{0x9fc4347cu,0xffb10030u,false},
{0x9fc43480u,0xffb00020u,false},
{0x9fc43484u,0xffa500c8u,false},
{0x9fc43488u,0x82620000u,false},
{0x9fc4348cu,0xffa600d0u,false},
{0x9fc43490u,0xffa700d8u,false},
{0x9fc43494u,0xffa800e0u,false},
{0x9fc43498u,0xffa900e8u,false},
{0x9fc4349cu,0xffaa00f0u,false},
{0x9fc434a0u,0xffab00f8u,false},
{0x9fc434a4u,0xe7ac00b8u,false},
};
e->reset(trace[0].pc);e->r[29].low=0x70003f90;e->r[4].low=0xffffffff9fc43e18ull;e->r[5].low=3;e->r[6].low=0x46;e->r[7].low=0x189;e->r[8].low=0x127;e->r[9].low=0xffffffff9fc43e58ull;e->r[31].low=0xffffffff9fc4115cull;e->fpr[12]=0x7fc12345;
// Format-string byte 0x23 inferred from final r2 and the only LB in this prologue.
e->bios[0x43e18]=0x23;
for(const auto& x:trace)for(unsigned n=0;n<4;n++)e->bios[(x.pc&0x3fffff)+n]=uint8_t(x.word>>(8*n));
for(const auto& x:trace){ck(e->pc==x.pc&&e->delay==x.delay,"log9 prologue PCs/delay slots");e->step();}
ck(e->pc==0x9fc434a8&&e->r[29].low==0x70003e90&&e->r[2].low==0x23&&e->read(0x70003f48,4)==0x7fc12345&&e->read(0x70003f58,8)==3&&e->read(0x70003f60,8)==0x46,"reconstructed log9 prologue preserves arguments and FPU bits");
ck(e->diagnostic().find("f12=7fc12345")!=std::string::npos,"FPR diagnostics");e->reset();for(auto value:e->fpr)ck(value==0,"deterministic FPR reset policy");
std::cout<<"PASS: 32 FPR moves/raw memory, FCR masks, MOV.S, BC1 variants, CU1/fault guards and log9 prologue replay to 9FC434A8\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
