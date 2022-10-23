
// MFCApplication1_AutoDumpDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "MFCApplication1_AutoDump.h"
#include "MFCApplication1_AutoDumpDlg.h"
#include "afxdialogex.h"

#include "DumpHelper.h"
#include "CommonHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//禁用内联 or 关闭内联
#pragma inline_depth(0)

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


// CMFCApplication1AutoDumpDlg 对话框



CMFCApplication1AutoDumpDlg::CMFCApplication1AutoDumpDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCAPPLICATION1_AUTODUMP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	SetUnhandledExceptionFilter(DumpCallback);
}

void CMFCApplication1AutoDumpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMFCApplication1AutoDumpDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_TestDump, &CMFCApplication1AutoDumpDlg::OnBnClickedButtonTestdump)
END_MESSAGE_MAP()


// CMFCApplication1AutoDumpDlg 消息处理程序

BOOL CMFCApplication1AutoDumpDlg::OnInitDialog()
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

	ShowWindow(SW_MAXIMIZE);

	ShowWindow(SW_MINIMIZE);

	// TODO: 在此添加额外的初始化代码

	ShowWindow(SW_NORMAL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCApplication1AutoDumpDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMFCApplication1AutoDumpDlg::OnPaint()
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
HCURSOR CMFCApplication1AutoDumpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int CMFCApplication1AutoDumpDlg::Total(const vector<int>& vct)
{
	int sum = 0;
	for (int i = 0; i <= vct.size(); ++i)
	{
		//注 operator[] 在 debug 下 进行 下标异常检测
		//注 operator[] 在 release 下 不进行 下标异常检测 即 虽然下标异常仍然能够进行索引取值，显然：计算的结果没有保障了
		//用 at 方法在 debug release 下 都进行下标异常检测，虽然安全，但性能又受影响，矛盾，解决办法 用 迭代器进行遍历
		//这块是 演示在 debug ， release 模式下要 抛出 异常。所以必须用 at 方法
		sum += vct.at(i);
	}

	return sum;
}

void CMFCApplication1AutoDumpDlg::OnBnClickedButtonTestdump()
{
#pragma region "测试查找所有的子文件夹相关代码"

	/*CString strPathEx(__wargv[0]);
	CStringA strPathA(strPathEx.GetBuffer());

	string strPath(strPathA.GetBuffer());
	size_t iIndex = strPath.find_last_of('\\');
	if (iIndex == string::npos)
	{
		MessageBox(_T("exe path error"));
		return;
	}

	strPath = strPath.substr(0, iIndex);

	vector<string> vctDirs;
	CommonHelper::GetAllDirecories(strPath, vctDirs, true);
	return;*/

#pragma endregion

#pragma region "触发 vector 索引超边界异常 测试 生成 dump 文件"

	//注意要在 程序运行模式时， 弹出错误，点击重试后，即可生成 dump 文件
	vector<int> vct(11, 0);
	for (int i = 0; i < 11; ++i)
	{
		vct.at(i) = i;
	}

	int sum = Total(vct);

	CString str;
	str.Format(_T("sum = %d"), sum);
	MessageBox(str);

#pragma endregion
}
