#include "pch.h"
#include "framework.h"
#include "SerialPortDemo.h"
#include "SerialPortDemoDlg.h"
#include "afxdialogex.h"
#include <thread>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

thread* pThread = nullptr;
const UINT SLEEP_TIME_INTERVAL = 5;

CSerialPort* CSerialPortDemoDlg::m_pCom = new CSerialPort();

enum class ComSendStatus
{
	Init = 0,
	Ack,
	NAck
};

void GetFileSize(const std::string& strFile, unsigned int& fileSize)
{
	ifstream inStream(strFile, std::ios_base::in | std::ios_base::binary);
	streampos  iStart = inStream.tellg();
	inStream.seekg(0, ios::end);
	streampos iEnd = inStream.tellg();
	inStream.close();
	fileSize = (unsigned int)iEnd;
}

ComSendStatus gComSendStatus = ComSendStatus::Init;

void global_CallBack(byte* data, UINT length)
{
	CString strtemp;
	for (int k = 0; k < length; k++) // 将数组转换为CString型变量
	{
		char bt = *(char*)(data + k); // 字符型
		strtemp += bt;
	}

	if (strtemp == _T("123"))//123为 Ack
	{
		gComSendStatus = ComSendStatus::Ack;
	}
	else
	{
		gComSendStatus = ComSendStatus::NAck;
	}
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSerialPortDemoDlg 对话框



CSerialPortDemoDlg::CSerialPortDemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERIALPORTDEMO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CSerialPortDemoDlg::~CSerialPortDemoDlg()
{
	if (nullptr != m_pCom)
	{
		delete m_pCom;
		m_pCom = nullptr;
	}
}

void CSerialPortDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PortNo, m_PortNo);
	DDX_Control(pDX, IDC_COMBO_Baud, m_Baud);
	DDX_Control(pDX, IDC_BUTTON_Connect, m_btn_Connect);
	DDX_Control(pDX, IDC_BUTTON_Send, m_btn_Send);
	DDX_Control(pDX, IDC_EDIT_Send, m_text_Send);
	DDX_Control(pDX, IDC_EDIT_Recieved, m_text_Recieved);
	DDX_Control(pDX, IDC_BUTTON_BUSINESS, m_btnBusiness);
}

BEGIN_MESSAGE_MAP(CSerialPortDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_Connect, &CSerialPortDemoDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_Send, &CSerialPortDemoDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_BUSINESS, &CSerialPortDemoDlg::OnBnClickedButtonBusiness)
END_MESSAGE_MAP()


// CSerialPortDemoDlg 消息处理程序

BOOL CSerialPortDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	if (DetectPort())
	{

		for (int i = 0; i < sizeof(port); i++)
		{
			if (port[i])
			{
				CString str;
				str.Format(_T("COM %d"), i + 1);
				m_PortNo.AddString(str);
			}
		}
		m_PortNo.SetCurSel(0);
	}


	m_Baud.AddString(_T("9600"));
	m_Baud.AddString(_T("19200"));
	m_Baud.AddString(_T("38400"));
	m_Baud.AddString(_T("57600"));
	m_Baud.AddString(_T("115200"));
	m_Baud.SetCurSel(0);
	//绑定串口数据的回调事件
	//std::function<void(byte*, UINT)> _fun = std::bind(&CSerialPortDemoDlg::m_CallBack, this, std::placeholders::_1, std::placeholders::_2);
	//m_pCom->CallBack(_fun);

	std::function<void(byte*, UINT)> _fun = global_CallBack;
	m_pCom->CallBack(_fun);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSerialPortDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSerialPortDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSerialPortDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CSerialPortDemoDlg::OnBnClickedButtonConnect()
{
	// TODO: 在此添加控件通知处理程序代码
	if (isConnected)
	{
		m_pCom->ClosePort();
		m_pCom->CloseListenTread();


		m_btn_Connect.SetWindowTextW(_T("建立通信"));
		m_PortNo.EnableWindow(true);
		m_Baud.EnableWindow(true);
		m_btn_Send.EnableWindow(false);
		m_btnBusiness.EnableWindow(false);

		isConnected = false;

	}
	else
	{
		CString str;
		CString str1;

		m_PortNo.GetWindowTextW(str);
		AfxExtractSubString(str1, str, 1, ' ');
		int port = _ttoi(str1);

		m_Baud.GetWindowTextW(str1);

		UINT Baud = _ttoi(str1);

		m_pCom->InitPort(port, Baud, 'N', 8, 1, EV_RXCHAR);
		m_pCom->OpenListenThread();

		m_btn_Connect.SetWindowTextW(_T("断开通信"));
		isConnected = true;

		m_PortNo.EnableWindow(false);
		m_Baud.EnableWindow(false);
		m_btn_Send.EnableWindow(true);
		m_btnBusiness.EnableWindow(true);
	}
}


void CSerialPortDemoDlg::OnBnClickedButtonSend()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	CStringA data;
	m_text_Send.GetWindowTextW(str);
	data = str;
	SendCmd(data);

}

void CSerialPortDemoDlg::m_CallBack(byte* data, UINT length)
{
	int L = length;
	CString strtemp;
	for (int k = 0; k < L; k++) // 将数组转换为CString型变量
	{
		BYTE bt = *(char*)(data + k); // 字符型
		strtemp += bt;
	}
	strtemp += "\r\n";
	int TextL = m_text_Recieved.GetWindowTextLengthW();

	m_text_Recieved.SetSel(TextL, TextL, true);
	m_text_Recieved.ReplaceSel(strtemp, false);
}
int CSerialPortDemoDlg::DetectPort()
{
	memset(port, 0, sizeof(port));

	CString com;

	int cnt = 0;

	for (int i = 0; i < 10; i++)
	{
		if (!m_pCom->DetectPort(i + 1))     // 如果没有该设备，或者被其他应用程序在用    *******************
		{
			port[i] = false;
		}
		else
		{
			cnt++;     // 如果存在，则记录下来。
			port[i] = true;
		}
	}

	if (cnt)     // 如果串口存在，则执行相应的初始化（采用控件）
	{
	}
	else     // 如果不存在，则显示错误信息，而不进行串口操作，防止系统异常造成界面的初始化不完全
	{
		CString str = _T("没有可用串口");
		AfxMessageBox(str, MB_OK);
	}
	return cnt;
}
int CSerialPortDemoDlg::SendCmd(CStringA cmd)
{
	int len = cmd.GetLength();
	if (len < 1) { return 0; }

	byte* data = new byte[len];
	if (nullptr == data)
	{
		throw "data is nullptr not enough memory to use";
	}

	memcpy(data, cmd, len);
	m_pCom->WriteData(data, len);
	delete[] data;
	data = nullptr;

	return len;
}

void TestBusinessTmp();
void TestBusiness();
void CSerialPortDemoDlg::OnBnClickedButtonBusiness()
{
	pThread = new thread(&TestBusiness);
}

void TestBusinessTmp()
{
	bool bSuccess = false;
	byte buf[10] = { 0 };
	//CString str;
	CStringA data;
	//m_text_Send.GetWindowTextW(str);
	//data = str;

	data = "123";
	int i = 3;
	//while (i--)
	{
		data = "123";
		CSerialPortDemoDlg::SendCmd(data);
		buf[3] = 0;
		bSuccess = CSerialPortDemoDlg::m_pCom->ReadBytes(buf, 3);
		if (bSuccess
			&& buf[0] == 0x31
			&& buf[1] == 0x32
			&& buf[2] == 0x33)
		{
			//MessageBox(L"recv 3 bytes 123 is ok", L"Tip");
			AfxMessageBox(L"recv 3 bytes 123 is ok");
		}
		else
		{
			//MessageBox(L"recv buffer error", L"Tip");
			AfxMessageBox(L"recv buffer error");
		}
	}

	{
		data = "456";
		CSerialPortDemoDlg::SendCmd(data);
		buf[3] = 0;
		bSuccess = CSerialPortDemoDlg::m_pCom->ReadBytes(buf, 3);
		if (bSuccess
			&& buf[0] == 0x34
			&& buf[1] == 0x35
			&& buf[2] == 0x36)
		{
			//MessageBox(L"recv 3 bytes 456 is ok", L"Tip");
			AfxMessageBox(L"recv 3 bytes 456 is ok");
		}
		else
		{
			//MessageBox(L"recv buffer error", L"Tip");
			AfxMessageBox(L"recv buffer error");
		}
	}

	//{
	//	data = "789";
	//	CSerialPortDemoDlg::SendCmd(data);
	//	buf[3] = 0;
	//	bSuccess = CSerialPortDemoDlg::m_pCom->ReadBytes(buf, 3);
	//	if (bSuccess
	//		&& buf[0] == 0x37
	//		&& buf[1] == 0x38
	//		&& buf[2] == 0x39)
	//	{
	//		MessageBox(L"recv 3 bytes 789 is ok", L"Tip");
	//	}
	//	else
	//	{
	//		MessageBox(L"recv buffer error", L"Tip");
	//	}
	//}

	AfxMessageBox(L"business success");
	pThread = nullptr;
}

void TestBusiness()
{
	bool bSuccess = false;
	byte buf[10] = { 0 };
	//CString str;
	CStringA data;
	//m_text_Send.GetWindowTextW(str);
	//data = str;

	data = "123";
	{
		data = "123";
		gComSendStatus = ComSendStatus::Init;
		CSerialPortDemoDlg::SendCmd(data);

		while (true)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			if (gComSendStatus == ComSendStatus::Ack)
			{
				AfxMessageBox(L"recv 3 bytes 123 is ok");
				break;
			}
			else if (gComSendStatus == ComSendStatus::Ack)
			{
				AfxMessageBox(L"recv buffer error");
				break;
			}
		}
	}

	if (gComSendStatus == ComSendStatus::Ack)
	{
		//发送文件
		string strFile = "F:\\tmp.zip";
		FILE* fp;
		size_t readLen = 0;
		const size_t bufLen = 1024;
		byte buf[bufLen];
		fopen_s(&fp, strFile.c_str(), "rb");
		bool bSendSuccess = true;
		while (true)
		{
			if (bSendSuccess)
			{
				readLen = fread_s(buf, bufLen, 1, bufLen, fp);
				if (readLen == 0)
				{
					break;
				}
			}
			/*else
			{
				//bSendSuccess == false 时，重新发送上一次发送的数据
			}*/

			//发送字节流
			gComSendStatus = ComSendStatus::Init;
			CSerialPortDemoDlg::m_pCom->WriteData(buf, readLen);

			//等待回复 ACK
			while (true)
			{
				Sleep(SLEEP_TIME_INTERVAL);
				if (gComSendStatus == ComSendStatus::Ack)
				{
					bSendSuccess = true;
					break;
				}
				else if (gComSendStatus == ComSendStatus::Ack)
				{
					bSendSuccess = false;
					break;
				}
			}
		}

		fclose(fp);
	}

	{
		data = "456";
		gComSendStatus = ComSendStatus::Init;
		CSerialPortDemoDlg::SendCmd(data);

		/*	while (true)
			{
				Sleep(SLEEP_TIME_INTERVAL);
				if (gComSendStatus == ComSendStatus::Ack)
				{
					AfxMessageBox(L"recv 3 bytes 123 is ok");
					break;
				}
				else if (gComSendStatus == ComSendStatus::Ack)
				{
					AfxMessageBox(L"recv buffer error");
					break;
				}
			}*/
	}

	AfxMessageBox(L"business success");
	pThread = nullptr;
}
