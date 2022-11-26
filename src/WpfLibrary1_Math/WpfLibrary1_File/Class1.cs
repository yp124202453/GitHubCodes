using System;
using System.IO;

namespace WpfLibrary1_File
{
    public static class Class1
    {
        public static string GetAllText(this string strFilePath)
        {
            if (string.IsNullOrWhiteSpace(strFilePath) || !File.Exists(strFilePath)) { return string.Empty; }
            return File.ReadAllText(strFilePath);
        }
    }
}
