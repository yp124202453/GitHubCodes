using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace ModifyConfigApp
{
    internal class Program
    {
        const string strSha = "\"sha512\"\\s*:\\s*\"\"";
        const string strShaPath = "\"sha512\"\\s*:\\s*\"\",\\s*\"path\"\\s*:\\s*\".\"";

        //const string strNew = "\"sha512\": \"\",\r\n\t\"path\": \".\"";
        const string strPath = ",\r\n\t\"path\": \".\"";

        static Regex shaReg = new Regex(strSha, RegexOptions.IgnoreCase | RegexOptions.Compiled);
        static Regex shaPathReg = new Regex(strShaPath, RegexOptions.IgnoreCase | RegexOptions.Compiled);

        static void Main(string[] args)
        {
            if (args.Length > 0)
            {
                ModifyConfig(args[0]);
            }
        }

        private static void ModifyConfig(string strFilePath)
        {
            if (string.IsNullOrWhiteSpace(strFilePath) || !File.Exists(strFilePath)) { return; }
            try
            {
                //Console.WriteLine("press enter to go on..");
                //Console.ReadLine();
                Console.WriteLine(strFilePath);
                string strAllText = File.ReadAllText(strFilePath, Encoding.ASCII);
                List<Match> shaMatches = shaReg.Matches(strAllText).OfType<Match>().Where(x => x.Success).ToList();
                shaMatches.ForEach(m => Console.WriteLine($"Index = {m.Index}"));

                Console.WriteLine();

                List<Match> shaPathMatches = shaPathReg.Matches(strAllText).OfType<Match>().Where(x => x.Success).ToList();
                shaPathMatches.ForEach(m => Console.WriteLine($"Index = {m.Index}"));

                bool bWriteFile = false;
                for (int i = shaMatches.Count - 1; i >= 0; --i)
                {
                    Match m = shaMatches[i];
                    if (shaPathMatches.Any(x => x.Index == m.Index))
                    {
                        continue;
                    }

                    bWriteFile = true;
                    strAllText = strAllText.Insert(m.Index + m.Value.Length, strPath);
                }

                if (bWriteFile)
                {
                    //strAllText = strAllText.Replace(strOld, strNew);
                    File.WriteAllText(strFilePath, strAllText, Encoding.ASCII);
                }

            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
                Trace.WriteLine(ex.ToString());
            }
        }

    }
}
