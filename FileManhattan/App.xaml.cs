using System;
using System.IO;
using System.Windows;
using System.Threading;
using FileManhattan.Modules;

namespace FileManhattan
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        public Mutex mtx;
        protected override async void OnStartup(StartupEventArgs e)
        {
            if (!File.Exists(AppDomain.CurrentDomain.BaseDirectory + "\\ManhattanNative.dll"))
            {
                MessageBox.Show("ManhtattanNative.dll이 존재하지 않습니다.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Shutdown();
            }

            bool flagMutex;
            mtx = new Mutex(true, "FILEMANHATTAN", out flagMutex);
            if (!flagMutex)
            {
                MessageBox.Show("FileManhattan이 이미 실행중입니다.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Shutdown();
            }

            await PhysicalDiskInfo.GetPhysicalDiskInformationAsync();
            base.OnStartup(e);
        }
    }
}
