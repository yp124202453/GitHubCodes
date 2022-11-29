.386
.model flat,stdcall
option casemap:none


include windows.inc
include user32.inc
includelib user32.lib
include kernel32.inc
includelib kernel32.lib

;数据段
.data
szCaption   db  "好一个汇编", 0
szText      db  "Hello World!", 0
FileName    db  "d:\1234.exe",0
message       db  "删除文件失败",0

;代码段
.code
start:
	  INVOKE DeleteFile,addr FileName
	  .if eax == 0
			INVOKE MessageBox, NULL, addr message, addr message, MB_OK
			INVOKE ExitProcess, NULL
	  .endif
	  
	  mov eax,0
	  mov ebx,4
      INVOKE MessageBox, eax, addr szText, addr szCaption, ebx
      INVOKE ExitProcess, NULL
end start