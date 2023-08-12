using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace WpfApp1_TestProbing
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        public App()
        {
            //if (!Win32.SetDllSearchPath())
            //{
            //    MessageBox.Show("设置搜索路径失败");
            //}

            ShutdownMode = ShutdownMode.OnMainWindowClose;

            //string dir = Directory.GetCurrentDirectory();
            //Dispatcher.Invoke(new Action(() =>
            //{            
            //    MessageBox.Show(dir);
            //}));

            //MessageBox.Show(dir);
            //after messagebox show , the main window can't display ,contains a thread name is "RootHwndWatch" 
            //all thread suspend,i don't know the reason 
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            //this msg box is ok
            string dir = Directory.GetCurrentDirectory() + $" parameters: count = {e.Args.Length} {string.Join(" , ", e.Args)}";
            MessageBox.Show(dir);
        }
    }
}
