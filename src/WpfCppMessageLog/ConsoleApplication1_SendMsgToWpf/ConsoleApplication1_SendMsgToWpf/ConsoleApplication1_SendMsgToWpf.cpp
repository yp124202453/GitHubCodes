#include <iostream>
#include <windows.h>

#define WIN32_LEAN_AND_MEAN
#define WM_HOTKEY__1 65536

int main()
{
	std::cout << "Hello World!\n";

	MSG msg = { 0 };

	//while (GetMessage(&msg, NULL, 0, 0) != 0)
	//{
	//	if (msg.message == WM_HOTKEY__1)
	//	{
	//		//(L"WM_HOTKEY received\n");
	//	}
	//}

	//手动改变这个 线程id，并向wpf进程的指定线程发送 消息
	int thdId = 21244;//这个线程id 是osid，不是  托管线程中的 id，如5，它只是 托管程序中 进程自己编号 的id 在 os层面不识别它的

	for (int i = 0; i < 10; i++)
	{
		PostThreadMessageW(thdId, 65536, 0, 0);
		Sleep(100);
	}

	PostThreadMessageW(thdId, 65535, 0, 0);

	return 0;
}