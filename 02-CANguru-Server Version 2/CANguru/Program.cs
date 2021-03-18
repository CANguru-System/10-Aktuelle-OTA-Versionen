using System;
using System.Windows.Forms;

namespace CANguruX
{
    static class Program
    {
        /// <summary>
        /// Der Haupteinstiegspunkt für die Anwendung.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            System.Threading.Mutex mutex = new System.Threading.Mutex(false, "{86E6517W-71A4-4ea0-A4E9-DA3AF2932C21}");
            if (mutex.WaitOne(0, false))
                Application.Run(new Form1());
            else
                MessageBox.Show("Dieses Programm ist bereits geöffnet");
            Environment.Exit(0x00);
        }
    }
}
