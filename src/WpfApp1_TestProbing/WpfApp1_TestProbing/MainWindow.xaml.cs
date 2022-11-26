using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace WpfApp1_TestProbing
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            int ix = 100;
            int iy = 200;
            MessageBox.Show(this, $"{ix} + {iy} = {WpfLibrary1_Math.MathEx.Add(ix, iy)}");

            string strFilePath = string.Empty;
            string strAllText = WpfLibrary1_File.Class1.GetAllText(strFilePath);
            if (string.IsNullOrEmpty(strAllText))
            {
                MessageBox.Show(this, "file is empty or file not exits");
            }
            else
            {
                MessageBox.Show(this, strAllText);
            }
        }
    }
}
