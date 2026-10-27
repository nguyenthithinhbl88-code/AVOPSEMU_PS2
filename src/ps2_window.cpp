#include "ps2_window.hpp"
#include "ps2_ee.hpp"
#include "ps2_ui_layout.hpp"
#include <array>
#include <sstream>
#include <iomanip>
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <chrono>
namespace {
using Clock=std::chrono::steady_clock;
HWND current=nullptr;
constexpr COLORREF bg=RGB(13,19,29),panel=RGB(21,31,45),border=RGB(43,61,80),fg=RGB(233,242,251),muted=RGB(153,177,198),accent=RGB(92,205,255);
void label(HDC dc,const wchar_t* text,RECT r,HFONT font,COLORREF color,UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE){auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,text,-1,&r,flags);SelectObject(dc,old);}
void box(HDC dc,RECT r,COLORREF fill){auto brush=CreateSolidBrush(fill);auto pen=CreatePen(PS_SOLID,1,border);auto ob=SelectObject(dc,brush),op=SelectObject(dc,pen);RoundRect(dc,r.left,r.top,r.right,r.bottom,12,12);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(brush);DeleteObject(pen);}
struct State{
 ps2::Ee ee;HWND output=nullptr,screen=nullptr,viewport=nullptr,run_button=nullptr,window=nullptr;
 std::array<HWND,17> buttons{};
 bool running=false,fast=true,debugger=false;uint64_t measured_instructions=0;Clock::time_point measured_at=Clock::now();double mips=0;Clock::time_point refreshed=Clock::now();
 HFONT font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 HFONT title=CreateFontW(-30,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 HFONT mono=CreateFontW(-14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FIXED_PITCH,L"Consolas");
 HBRUSH background=CreateSolidBrush(bg);
 ps2ui::Layout positions;
 std::wstring loaded=L"Demo CPU có sẵn",source=L"Chương trình thử",message=L"Sẵn sàng • Nạp BIOS hoặc mở ELF, sau đó bấm Chạy.";
 ~State(){DeleteObject(font);DeleteObject(title);DeleteObject(mono);DeleteObject(background);}
};
void layout(State& s){RECT r{};GetClientRect(s.window,&r);s.positions=ps2ui::layout(static_cast<int>(r.right),static_cast<int>(r.bottom),s.debugger);
 for(auto button:s.buttons)ShowWindow(button,SW_HIDE);
 const unsigned primary[]={5,1,10,11,4,9};
 for(unsigned i=0;i<6;i++){auto button=s.buttons[primary[i]];ShowWindow(button,SW_SHOW);MoveWindow(button,20+int(i)*(s.positions.button_width+12),86,s.positions.button_width,40,TRUE);}
 for(unsigned i=0;i<2;i++){auto button=s.buttons[i==0?16:12];ShowWindow(button,SW_SHOW);MoveWindow(button,36,360+int(i)*44,192,36,TRUE);}
 auto move=[](HWND child,const ps2ui::Rect& q){MoveWindow(child,q.x,q.y,q.width,q.height,TRUE);};
 move(s.viewport,s.positions.video);ShowWindow(s.output,s.debugger?SW_SHOW:SW_HIDE);if(s.debugger)move(s.output,s.positions.debug);
 InvalidateRect(s.window,nullptr,FALSE);InvalidateRect(s.viewport,nullptr,FALSE);
}
void paint(State& s,HDC dc){RECT r{};GetClientRect(s.window,&r);FillRect(dc,&r,s.background);
 label(dc,L"AVOPSEMU PS2",RECT{20,10,400,55},s.title,fg);
 label(dc,L"EE / GS LABORATORY  ·  v50",RECT{22,52,420,78},s.font,accent);
 std::wostringstream metrics;metrics<<std::fixed<<std::setprecision(1)<<s.mips<<L" MIPS  ·  "<<(!s.ee.first_fault.empty()?L"DỪNG DO LỖI":s.running?L"ĐANG CHẠY":L"TẠM DỪNG");
 label(dc,metrics.str().c_str(),RECT{460,20,r.right-20,64},s.font,!s.ee.first_fault.empty()?RGB(255,153,153):accent,DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
 const auto& q=s.positions.side;box(dc,RECT{q.x,q.y,q.x+q.width,q.y+q.height},panel);
 label(dc,L"PHIÊN PS2",RECT{36,156,228,186},s.font,accent);
 label(dc,s.source.c_str(),RECT{36,194,228,220},s.font,muted);
 label(dc,s.loaded.c_str(),RECT{36,224,228,256},s.font,fg,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
 label(dc,L"PS2 đang phát triển",RECT{36,268,228,298},s.font,accent);
 label(dc,L"Chưa xác nhận BIOS vào menu.\nThử demo trong menu Công cụ.",RECT{36,302,228,350},s.font,muted,DT_LEFT|DT_WORDBREAK);
 label(dc,L"F5  Chạy / Tạm dừng\nF6  Dừng\nF9  Bật / tắt debugger",RECT{36,464,228,546},s.font,muted,DT_LEFT|DT_WORDBREAK);
 const auto& t=s.positions.status;box(dc,RECT{t.x,t.y,t.x+t.width,t.y+t.height},panel);
 label(dc,s.message.c_str(),RECT{t.x+10,t.y,t.x+t.width-10,t.y+t.height},s.font,muted,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
}
void pause(State& s){s.running=false;SetWindowTextW(s.run_button,L"Chạy  F5");}
void refresh(State& s){
 auto now=Clock::now();double elapsed=std::chrono::duration<double>(now-s.measured_at).count();
 if(elapsed>=0.1){s.mips=s.running&&s.ee.instructions>=s.measured_instructions?(s.ee.instructions-s.measured_instructions)/elapsed/1e6:0;s.measured_instructions=s.ee.instructions;s.measured_at=now;}
 if(s.screen){const auto caption=L"AVOPSEMU PS2 — GS / "+s.source+L" / "+s.loaded;SetWindowTextW(s.screen,caption.c_str());}
 if(s.debugger){auto d=s.ee.diagnostic();SetWindowTextW(s.output,std::wstring(d.begin(),d.end()).c_str());}
 InvalidateRect(s.window,nullptr,FALSE);if(s.viewport)InvalidateRect(s.viewport,nullptr,FALSE);if(s.screen)InvalidateRect(s.screen,nullptr,FALSE);
 s.refreshed=Clock::now();
}
LRESULT CALLBACK video_proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 auto s=reinterpret_cast<State*>(GetWindowLongPtrW(w,GWLP_USERDATA));
 if(m==WM_NCCREATE){s=static_cast<State*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));}
 if(m==WM_CLOSE){ShowWindow(w,SW_HIDE);return 0;}
 if(m==WM_SIZE){InvalidateRect(w,nullptr,FALSE);return 0;}
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT&&s){
  PAINTSTRUCT paint{};HDC dc=BeginPaint(w,&paint);RECT r{};GetClientRect(w,&r);FillRect(dc,&r,s->background);
  SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(130,215,240));
  try{
   if(!(s->ee.gs.regs[0]&3)){
    const int cy=static_cast<int>(r.bottom)/2;
    label(dc,L"MÀN HÌNH GS",RECT{16,cy-64,r.right-16,cy-14},s->title,accent,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    label(dc,s->ee.first_fault.empty()?L"GS chưa bật xuất hình.":L"CPU đã dừng trước khi GS bật xuất hình.",RECT{16,cy,r.right-16,cy+32},s->font,fg,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    label(dc,s->ee.first_fault.empty()?L"Theo dõi phiên hiện tại; xem debugger bằng F9.":L"Bấm Lưu log ở cửa sổ chính để lấy nguyên nhân dừng.",RECT{20,cy+42,r.right-20,cy+104},s->font,muted,DT_CENTER|DT_WORDBREAK);
    EndPaint(w,&paint);return 0;
   }
   auto image=s->ee.gs.scanout();
   int available_w=std::max(1L,r.right-24),available_h=std::max(1L,r.bottom-70);
   double scale=std::min(double(available_w)/image.width,double(available_h)/image.height);
   int width=std::max(1,int(image.width*scale)),height=std::max(1,int(image.height*scale));
   int x=(r.right-width)/2,y=44+(available_h-height)/2;
   BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=LONG(image.width);info.bmiHeader.biHeight=-LONG(image.height);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
   SetStretchBltMode(dc,COLORONCOLOR);StretchDIBits(dc,x,y,width,height,0,0,int(image.width),int(image.height),image.bgra.data(),&info,DIB_RGB_COLORS,SRCCOPY);
   std::wstring caption=L"PS2 GS  /  "+std::to_wstring(image.width)+L" x "+std::to_wstring(image.height);
   if(!image.enabled)caption+=L" / Chưa bật xuất hình";else if(!image.supported)caption+=L" / Định dạng chưa hỗ trợ";
   TextOutW(dc,12,12,caption.c_str(),int(caption.size()));
   const wchar_t* note=L"GS phần mềm • Hiển thị hình ảnh không xác nhận BIOS đã khởi động hoàn tất.";
   TextOutW(dc,12,std::max(44L,r.bottom-22),note,lstrlenW(note));
  }catch(const std::exception&){const wchar_t* error=L"Không lấy được hình ảnh GS";TextOutW(dc,12,12,error,lstrlenW(error));}
  EndPaint(w,&paint);return 0;
 }
 if(m==WM_NCDESTROY){if(s&&s->screen==w)s->screen=nullptr;SetWindowLongPtrW(w,GWLP_USERDATA,0);}
 return DefWindowProcW(w,m,wp,lp);
}
void show_screen(HWND owner,State& s){
 if(!s.screen){auto instance=GetModuleHandleW(nullptr);WNDCLASSW c{};c.lpfnWndProc=video_proc;c.hInstance=instance;c.lpszClassName=L"PSLabPs2GsScreen";c.hCursor=LoadCursor(nullptr,IDC_ARROW);WNDCLASSW existing{};if(!GetClassInfoW(instance,c.lpszClassName,&existing)&&!RegisterClassW(&c))throw std::runtime_error("Cannot register GS screen");
  s.screen=CreateWindowW(c.lpszClassName,L"AVOPSEMU PS2 v50 - GS Screen",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,900,720,owner,nullptr,instance,&s);if(!s.screen)throw std::runtime_error("Cannot create GS screen");
 }
 ShowWindow(s.screen,IsIconic(s.screen)?SW_RESTORE:SW_SHOW);SetForegroundWindow(s.screen);InvalidateRect(s.screen,nullptr,FALSE);
}
std::vector<uint8_t> load(HWND w,bool bios,std::wstring& selected){wchar_t path[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=bios?L"PS2 BIOS\0*.bin\0All\0*.*\0":L"PS2 homebrew ELF\0*.elf\0All\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;if(!GetOpenFileNameW(&o))return {};std::ifstream f(std::filesystem::path(path),std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>64*1024*1024)throw std::runtime_error("Input file unavailable or exceeds 64 MiB");std::vector<uint8_t> bytes(static_cast<size_t>(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))throw std::runtime_error("Read failed");selected=std::filesystem::path(path).filename().wstring();return bytes;}
void save(HWND w,State& s){wchar_t path[32768]=L"avopsemu_ps2_log.txt";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=L"Text log\0*.txt\0All\0*.*\0";o.lpstrDefExt=L"txt";o.Flags=OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;s.message=L"Đã hủy lưu log.";if(GetSaveFileNameW(&o)){std::ofstream f(std::filesystem::path(path),std::ios::binary);f<<s.ee.diagnostic();if(!f)throw std::runtime_error("Cannot save log");s.message=L"Đã lưu log PS2.";}}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<State*>(GetWindowLongPtrW(w,GWLP_USERDATA));
 if(m==WM_CREATE){
  auto owned=std::make_unique<State>();s=owned.get();SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));owned.release();
  s->window=w;
  const wchar_t* labels[]={L"Demo CPU",L"Mở ELF",L"Chạy 1 lệnh",L"Chạy 1.000 lệnh",L"Reset",L"Nạp BIOS",L"Demo thanh ghi",L"Demo GIF",L"Chạy 100.000 lệnh",L"Lưu log",L"Chạy  F5",L"Dừng  F6",L"Cửa sổ GS riêng",L"Demo GPU",L"Demo DMA GPU",L"Demo chain GPU",L"Chạy nhanh: Bật"};
  for(int i=0;i<17;i++){s->buttons[i]=CreateWindowW(L"BUTTON",labels[i],WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,w,reinterpret_cast<HMENU>(INT_PTR(501+i)),nullptr,nullptr);SendMessageW(s->buttons[i],WM_SETFONT,reinterpret_cast<WPARAM>(s->font),TRUE);}
  s->run_button=s->buttons[10];
  s->output=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOHSCROLL,0,0,1,1,w,nullptr,nullptr,nullptr);
  SendMessageW(s->output,EM_SETLIMITTEXT,1024*1024,0);SendMessageW(s->output,WM_SETFONT,reinterpret_cast<WPARAM>(s->mono),TRUE);
  auto instance=GetModuleHandleW(nullptr);WNDCLASSW v{};v.lpfnWndProc=video_proc;v.hInstance=instance;v.lpszClassName=L"AVOPSEMU_PS2_Viewport";v.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&v);
  s->viewport=CreateWindowW(v.lpszClassName,L"",WS_CHILD|WS_VISIBLE,0,0,1,1,w,nullptr,instance,s);
  if(!s->viewport)return -1;
  HMENU menu=CreateMenu(),file=CreatePopupMenu(),view=CreatePopupMenu(),tools=CreatePopupMenu();
  AppendMenuW(file,MF_STRING,506,L"Nạp BIOS...");AppendMenuW(file,MF_STRING,502,L"Mở ELF...");AppendMenuW(file,MF_STRING,510,L"Lưu log...");AppendMenuW(file,MF_STRING,520,L"Thoát");
  AppendMenuW(view,MF_STRING,518,L"Bật / tắt debugger  F9");AppendMenuW(view,MF_STRING,513,L"Mở cửa sổ GS riêng");AppendMenuW(view,MF_STRING,519,L"Hướng dẫn");
  const unsigned commands[]={501,507,508,514,515,516,503,504,509};const wchar_t* names[]={L"Demo CPU",L"Demo thanh ghi CPU / GS",L"Demo GIF",L"Demo GPU",L"Demo DMA GPU",L"Demo chain GPU",L"Chạy 1 lệnh",L"Chạy 1.000 lệnh",L"Chạy 100.000 lệnh"};
  for(unsigned i=0;i<9;i++)AppendMenuW(tools,MF_STRING,commands[i],names[i]);
  AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(file),L"Tệp");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(view),L"Hiển thị");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(tools),L"Công cụ");SetMenu(w,menu);layout(*s);
  s->ee.demo();refresh(*s);if(!SetTimer(w,1,16,nullptr)){s->message=L"Không tạo được bộ hẹn giờ chạy.";refresh(*s);}return 0;
 }
 if(m==WM_GETMINMAXINFO){auto info=reinterpret_cast<MINMAXINFO*>(lp);info->ptMinTrackSize={1024,760};return 0;}
 if(m==WM_SIZE&&s&&s->viewport){layout(*s);return 0;}
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT&&s){PAINTSTRUCT p{};auto dc=BeginPaint(w,&p);RECT r{};GetClientRect(w,&r);auto mem=CreateCompatibleDC(dc);auto bitmap=CreateCompatibleBitmap(dc,std::max(1,static_cast<int>(r.right)),std::max(1,static_cast<int>(r.bottom)));
  if(mem&&bitmap){auto old=SelectObject(mem,bitmap);paint(*s,mem);BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);}else paint(*s,dc);
  if(bitmap)DeleteObject(bitmap);if(mem)DeleteDC(mem);EndPaint(w,&p);return 0;}
 if((m==WM_CTLCOLOREDIT||m==WM_CTLCOLORSTATIC)&&s){auto dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,muted);SetBkColor(dc,bg);return reinterpret_cast<LRESULT>(s->background);}
 if(m==WM_DRAWITEM&&s){auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);if(item->CtlType==ODT_BUTTON){const bool primary=item->CtlID==511;box(item->hDC,item->rcItem,primary?accent:((item->itemState&ODS_SELECTED)?RGB(35,57,76):panel));wchar_t text[128]{};GetWindowTextW(item->hwndItem,text,128);label(item->hDC,text,item->rcItem,s->font,primary?bg:fg,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(item->itemState&ODS_FOCUS){auto r=item->rcItem;InflateRect(&r,-4,-4);DrawFocusRect(item->hDC,&r);}return TRUE;}}
 if(m==WM_TIMER&&wp==1&&s&&s->running){
  try{
   auto until=Clock::now()+std::chrono::milliseconds(s->fast?12:4);
   for(unsigned n=0;n<250000;n++){s->ee.step();if((n&63)==63&&Clock::now()>=until)break;}
   if(s->viewport&&!IsIconic(w))InvalidateRect(s->viewport,nullptr,FALSE);
   if(s->screen&&IsWindowVisible(s->screen))InvalidateRect(s->screen,nullptr,FALSE);
   if(Clock::now()-s->refreshed>=std::chrono::milliseconds(250))refresh(*s);
  }catch(const std::exception& e){pause(*s);std::string error=e.what();s->message=L"Đã dừng do lỗi: "+std::wstring(error.begin(),error.end())+L". Bấm Lưu log để gửi chẩn đoán.";refresh(*s);}
  return 0;
 }
 if(m==WM_COMMAND&&s){
  unsigned command=LOWORD(wp);
  if(command==520){DestroyWindow(w);return 0;}
  if(command==518){s->debugger=!s->debugger;CheckMenuItem(GetMenu(w),518,MF_BYCOMMAND|(s->debugger?MF_CHECKED:MF_UNCHECKED));layout(*s);refresh(*s);return 0;}
  if(command==513){try{show_screen(w,*s);}catch(const std::exception& e){s->message=L"Không mở được cửa sổ GS.";(void)e;}refresh(*s);return 0;}
  if(command==517){s->fast=!s->fast;SetWindowTextW(s->buttons[16],s->fast?L"Chạy nhanh: Bật":L"Chạy nhanh: Tắt");refresh(*s);return 0;}
  if(command==519){MessageBoxW(w,L"Nạp BIOS PS2 của bạn → Chạy.\nHoặc Mở ELF để chạy homebrew.\n\nF5: Chạy / Tạm dừng\nF6: Dừng (giữ PC)\nF9: Debugger\n\nReset đặt lại CPU về entry hiện có và giữ RAM.\nLưu log sẽ tạm dừng để lấy chẩn đoán.\nDemo nằm trong Công cụ; cửa sổ GS riêng nằm trong Hiển thị.\n\nPS2 đang phát triển, chưa xác nhận BIOS tới menu hoặc chạy game.",L"Hướng dẫn AVOPSEMU PS2",MB_OK);return 0;}
  if(command<501||command>516)return 0;
  bool was_running=s->running;pause(*s);
  try{switch(command){
   case 501:s->ee.demo();s->loaded=L"Demo CPU";s->source=L"Demo tự viết";s->message=L"Đã nạp demo CPU. Bấm Chạy để thử.";break;
   case 502:{std::wstring name;auto b=load(w,false,name);if(!b.empty()){s->ee.load_elf(b);s->loaded=name;s->source=L"Homebrew ELF";s->message=L"Đã mở ELF. Bấm Chạy để thử chương trình.";}else s->message=L"Đã hủy mở ELF; phiên hiện tại được giữ lại.";break;}
   case 503:s->ee.step();s->message=L"Đã chạy 1 lệnh.";break;
   case 504:for(unsigned n=0;n<1000;n++)s->ee.step();s->message=L"Đã chạy 1.000 lệnh EE.";break;
   case 505:s->ee.reset(s->ee.reset_entry);s->message=L"Đã reset CPU, giữ RAM. Bấm Chạy để bắt đầu.";break;
   case 506:{std::wstring name;auto b=load(w,true,name);if(!b.empty()){s->ee.load_bios(b);s->loaded=name;s->source=L"BIOS PS2";show_screen(w,*s);s->message=L"Đã nạp BIOS và mở cửa sổ GS riêng. Bấm F5 trong cửa sổ chính để chạy.";}else s->message=L"Đã hủy nạp BIOS; phiên hiện tại được giữ lại.";break;}
   case 507:s->ee.register_demo();s->loaded=L"Demo thanh ghi";s->source=L"Demo tự viết";s->message=L"Đã nạp demo thanh ghi CPU / GS.";break;
   case 508:s->ee.gif_demo();s->loaded=L"Demo GIF";s->source=L"Demo tự viết";s->message=L"Đã nạp demo GIF. Bấm Chạy để thử.";break;
   case 509:for(unsigned n=0;n<100000;n++)s->ee.step();s->message=L"Đã chạy 100.000 lệnh EE.";break;
   case 510:save(w,*s);break;
   case 511:if(was_running){s->message=L"Đã tạm dừng, giữ PC. Bấm Chạy để tiếp tục.";}else{if(!s->ee.first_fault.empty())throw std::runtime_error("Reset/load required after the recorded first fault");if(!SetTimer(w,1,16,nullptr))throw std::runtime_error("Cannot start run scheduler");s->running=true;SetWindowTextW(s->run_button,L"Tạm dừng  F5");s->message=L"Đang chạy. Bấm Tạm dừng hoặc Dừng bất cứ lúc nào.";}break;
   case 512:s->message=L"Đã dừng, giữ PC. Reset sẽ đặt lại CPU về entry.";break;
   case 516:s->ee.chain_demo();s->loaded=L"Demo chain GPU";s->source=L"Demo tự viết";s->message=L"Đã nạp demo chain GPU. Bấm Chạy để thử DMA chain.";break;
   case 515:s->ee.dma_demo();s->loaded=L"Demo DMA GPU";s->source=L"Demo tự viết";s->message=L"Đã nạp demo DMA GPU. Bấm Chạy để truyền dữ liệu tới GS.";break;
   case 514:s->ee.gpu_demo();s->loaded=L"Demo GPU";s->source=L"Demo tự viết";s->message=L"Đã nạp demo GPU. Bấm Chạy để vẽ qua CPU / GIF.";break;
  }}catch(const std::exception& e){pause(*s);std::string error=e.what();s->message=L"Đã dừng: "+std::wstring(error.begin(),error.end());}
  refresh(*s);return 0;
 }
 if(m==WM_DESTROY&&s){KillTimer(w,1);pause(*s);if(s->screen)DestroyWindow(s->screen);PostQuitMessage(0);return 0;}
 if(m==WM_NCDESTROY){SetWindowLongPtrW(w,GWLP_USERDATA,0);delete s;current=nullptr;}
 return DefWindowProcW(w,m,wp,lp);
}
}
HWND open_ps2_window(HWND owner){if(current){ShowWindow(current,SW_RESTORE);SetForegroundWindow(current);return current;}auto instance=GetModuleHandleW(nullptr);WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=instance;c.lpszClassName=L"AVOPSEMU_PS2_Studio";c.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);WNDCLASSW existing{};if(!GetClassInfoW(instance,c.lpszClassName,&existing)&&!RegisterClassW(&c))return nullptr;current=CreateWindowW(c.lpszClassName,L"AVOPSEMU PS2 v50 - EE / GS Studio",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,owner,nullptr,instance,nullptr);if(current)ShowWindow(current,SW_SHOW);return current;}
