using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace CsCoreConApp01
{
    internal static class Win32
    {
        //如下枚举请参见 
        //https://learn.microsoft.com/zh-cn/windows/win32/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories?f1url=%3FappId%3DDev16IDEF1%26l%3DZH-CN%26k%3Dk(SetDefaultDllDirectories)%3Bk(DevLang-csharp)%26rd%3Dtrue
        //https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories?f1url=%3FappId%3DDev16IDEF1%26l%3DZH-CN%26k%3Dk(SetDefaultDllDirectories)%3Bk(DevLang-csharp)%26rd%3Dtrue
        public const uint LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000;
        public const uint LOAD_LIBRARY_SEARCH_USER_DIRS = 0x00000400;

        [DllImport("kernel32", CallingConvention = CallingConvention.StdCall)]
        public static extern bool SetDefaultDllDirectories(uint uiFlags);

        [DllImport("kernel32", CallingConvention = CallingConvention.StdCall, SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr AddDllDirectory(string strSubDir);

        [DllImport("kernel32", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetLastError();

        [DllImport("CppTestDll", CallingConvention = CallingConvention.StdCall)]
        public static extern int Add(int numberA, int numberB);

        [DllImport("CppTestDll", CallingConvention = CallingConvention.StdCall)]
        public static extern int Sub(int numberA, int numberB);

        /// <summary>
        /// 将程序根目录下的所有子目录 （除了与当前进程位数不兼容的目录）设置为 dll 搜索路径
        /// </summary>
        /// <returns></returns>
        public static bool SetDllSearchPath()
        {
            try
            {
                if (Win32.SetDefaultDllDirectories(Win32.LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | Win32.LOAD_LIBRARY_SEARCH_USER_DIRS))
                {
                    Trace.WriteLine("Win32.SetDefaultDllDirectories success");
                }
                else
                {
                    Trace.WriteLine("Win32.SetDefaultDllDirectories failed");
                }

                //获取应用程序根目录所有的目录包括嵌套的子目录
                List<string> dirLst = Directory.GetDirectories(AppDomain.CurrentDomain.BaseDirectory, "*", SearchOption.AllDirectories).ToList();
                string strRemoveDir = null;
                if (Environment.Is64BitProcess)
                {
                    //64位进程运行时，剔除32位的dll目录
                    strRemoveDir = "x86";
                }
                else
                {
                    //32位进程运行时，剔除64位的dll目录
                    strRemoveDir = "x64";
                }

                dirLst.RemoveAll(x =>
                {
                    string strDirName = x.TrimEnd('\\');
                    strDirName = strDirName.Substring(strDirName.LastIndexOf('\\') + 1);
                    if (strRemoveDir.Equals(strDirName, StringComparison.OrdinalIgnoreCase)) { return true; }
                    return false;
                });
                IntPtr ptr = IntPtr.Zero;

                foreach (string dir in dirLst)
                {
                    ptr = Win32.AddDllDirectory(dir);

                    if (IntPtr.Zero != ptr)
                    {
                        Trace.WriteLine($"Win32.AddDllDirectory success : {dir}");
                    }
                    else
                    {
                        int iError = Win32.GetLastError();
                        Trace.WriteLine($"Win32.AddDllDirectory failed , LastError = {iError}  : {dir}");
                    }
                }

                return true;
            }
            catch (Exception)
            {
                throw;
            }

        }

    }
}

