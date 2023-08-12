using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WinFormApp_ResetEvent
{
    public static class Win32
    {
        [DllImport("kernel32.dll", EntryPoint = "GetCurrentThreadId")]
        public static extern int GetCurrentThreadId();
    }
}
