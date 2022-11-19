using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace WpfCppMessageLog
{
    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential)]
    public struct tagMSG
    {
        /// HWND->HWND__*
        public System.IntPtr hwnd;

        /// UINT->int
        public int message;

        /// WPARAM->UINT_PTR->int
        public int wParam;

        /// LPARAM->LONG_PTR->int
        public int lParam;

        /// DWORD->int
        public int time;

        /// POINT->tagPOINT
        public Point pt;
    }


    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {

        private const string strDateFormat = "yyyy-MM-dd HH-mm-ss fff";

        public static string LogFilePath { get; set; } = string.Empty;

        private static StreamWriter _sw;

        public static int LogThreadId { get; private set; } = 0;


        [System.Runtime.InteropServices.DllImportAttribute("user32.dll", EntryPoint = "GetMessageW")]
        [return: System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.Bool)]
        public static extern bool GetMessageW(ref tagMSG lpMsg, System.IntPtr hWnd, int wMsgFilterMin, int wMsgFilterMax);

        [DllImport("Kernel32", EntryPoint = "GetCurrentThreadId", ExactSpelling = true)]
        public static extern Int32 GetCurrentWin32ThreadId();

        public App()
        {
            LogFilePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, $"{DateTime.Now.ToString(strDateFormat)}.log");

            Thread thd = new Thread(() =>
            {
                _sw = new StreamWriter(LogFilePath, true, Encoding.Unicode);
                //LogThreadId = Thread.GetCurrentProcessorId();
                //Trace.WriteLine($"LogThreadId = {LogThreadId}");

                //Thread thd2 = Thread.CurrentThread;
                //int id2 = thd2.ManagedThreadId;

                //Trace.WriteLine($"id2 = {id2}");

                LogThreadId = GetCurrentWin32ThreadId();
                Trace.WriteLine(LogThreadId);

                tagMSG msg = new tagMSG();
                IntPtr hWnd = IntPtr.Zero;
                while (GetMessageW(ref msg, hWnd, 0, 0))
                {
                    switch (msg.message)
                    {
                        case 65535:
                            //_sw.WriteLine($"{DateTime.Now.ToString(strDateFormat)}");
                            _sw.Flush();
                            _sw.Close();
                            return;
                            break;
                        case 65536:
                            _sw.WriteLine($"{DateTime.Now.ToString(strDateFormat)}");
                            break;
                        default:
                            break;
                    }
                }

            });

            thd.Start();
        }

    }
}
