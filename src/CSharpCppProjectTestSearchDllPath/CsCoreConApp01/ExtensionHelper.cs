using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System
{
    internal static class ExtensionHelper
    {
        /// <summary>
        /// 将unicode字符串 转换为16进制 形式 如 \u0061\u0048
        /// </summary>
        /// <param name="strUnicode"></param>
        /// <returns></returns>
        public static string ToUnicodeHex(this string strUnicode)
        {
            if (string.IsNullOrWhiteSpace(strUnicode)) { return string.Empty; }

            StringBuilder sb = new StringBuilder();
            byte[] bytes = Encoding.Unicode.GetBytes(strUnicode);
            int iLen = bytes.Length / 2;
            for (int i = 0; i < bytes.Length; i += 2)
            {
                sb.Append("\\u");
                sb.Append(bytes[i].ToString("X2"));
                sb.Append(bytes[i + 1].ToString("X2"));
            }

            return sb.ToString();
        }
    }
}
