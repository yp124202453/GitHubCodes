using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinFormApp_ResetEvent
{
    public partial class Form1 : Form
    {
#if ManualResetEventMacro

        private ManualResetEvent resetEvent = new ManualResetEvent(false);

#else

        private AutoResetEvent resetEvent = new AutoResetEvent(false);

#endif

        private readonly object objLocker = new object();

        public Form1()
        {
            InitializeComponent();
        }

        private void btnStartTest_Click(object sender, EventArgs e)
        {
            Task.Factory.StartNew(() =>
            {
                string strTime;

                //string strTid = Thread.CurrentThread.ManagedThreadId.ToString();
                string strTid = Win32.GetCurrentThreadId().ToString();

                while (true)
                {
                    strTime = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss fff");
                    this.Invoke(new Action(() =>
                    {
                        txt.Text += $"{strTime} tid = {strTid}\r\n";
                    }));

                    resetEvent.WaitOne();//等待 set 将 bool 值设置为 true 即收到了信号

#if ManualResetEventMacro

                    //AutoResetEvent 等到信号后 会自动 Reset ，所以不需要调用 ReSet
                    lock (objLocker)
                    {
                        resetEvent.Reset();//将信号 bool 变量设置为 false ,再次等  set 后 ，waitone 就不阻塞线程了，可以继续运行
                    }

#endif

                    if (chkCloseTask.Checked)
                    {
                        this.Invoke(new Action(() =>
                        {
                            txt.Text += $"{strTime} tid = {strTid} task closed \r\n";
                        }));

                        break;
                    }
                }
            });
        }

        private void btnSet_Click(object sender, EventArgs e)
        {
            lock (objLocker)
            {
                resetEvent.Set();
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
#if ManualResetEventMacro
            resetEvent.Reset();
#endif

            Thread.Sleep(10);
            resetEvent.Close();
        }
    }
}
