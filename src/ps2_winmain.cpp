#include "ps2_window.hpp"
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
 auto window=open_ps2_window(nullptr);if(!window)return 1;
 MSG message{};int result;
 while((result=GetMessageW(&message,nullptr,0,0))>0){
  if(message.message==WM_KEYDOWN&&!(message.lParam&(LPARAM(1)<<30))){
   const auto root=GetAncestor(message.hwnd,GA_ROOT);
   if(root==window){unsigned command=message.wParam==VK_F5?511:message.wParam==VK_F6?512:message.wParam==VK_F9?518:0;if(command){SendMessageW(window,WM_COMMAND,command,0);continue;}}
  }
  if(GetAncestor(message.hwnd,GA_ROOT)==window&&IsDialogMessageW(window,&message))continue;
  TranslateMessage(&message);DispatchMessageW(&message);
 }
 return result==-1?1:static_cast<int>(message.wParam);
}
