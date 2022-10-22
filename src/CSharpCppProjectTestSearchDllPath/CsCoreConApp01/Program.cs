using System;
using System.Runtime.InteropServices;

namespace CsCoreConApp01
{
    class Program
    {
        static void TestNative()
        {
            int x = 100;
            int y = 400;
            int z = Win32.Add(x, y);
            Console.WriteLine($"{x} + {y} = {z}");
        }

        static void Main(string[] args)
        {
            if (!Win32.SetDllSearchPath())
            {
                Console.WriteLine("press any key to exit...");
                Console.ReadKey();
                return;
            }

            TestNative();


            Console.WriteLine("press any key to exit...");
            Console.ReadKey();

            return;

            #region "下面的代码  测试 clr cpp 代码，以及  类、枚举的定义"

            //Console.WriteLine("please input a line ...");
            //string strLine = Console.ReadLine();

            //CInvokeTestMath test = new CInvokeTestMath();

            //int a = 10;
            //int b = 20;
            ////int c = test.AddEx(a, b);
            //int c = test.Calculate(a, b, MyEnum.EAdd);
            //Console.WriteLine($"{a} + {b} = {c}");
            //Console.WriteLine($"{a} - {b} = {test.Calculate(a, b, MyEnum.ESub)}");

            //int m = (int)MyEnum.EAdd;
            //int n = (int)MyEnum.ESub;

            //string str = "123456";
            //IntPtr ptr = Marshal.StringToHGlobalAnsi(str);
            //Console.WriteLine(str);

            //unsafe
            //{
            //    sbyte* bPtr = (sbyte*)ptr;
            //    test.ChangeChars(bPtr, str.Length);
            //    string str2 = Marshal.PtrToStringAnsi(ptr);
            //    Console.WriteLine(str2);
            //    IntPtr arrPtr = Marshal.AllocHGlobal(sizeof(sbyte) * 10);
            //    Marshal.FreeHGlobal(arrPtr);
            //}

            //Console.WriteLine("Hello World!");
            //return;

            #endregion

        }
    }
}
