using System.Diagnostics;
using System.Linq;
using Alphaleonis.Win32.Vss;

namespace FileManhattan
{
    public class Vss
    {
        public static int GetVssCount()
        {
            IVssFactory vssImplementation = VssFactoryProvider.Default.GetVssFactory();
            using (IVssBackupComponents backup = vssImplementation.CreateVssBackupComponents())
            {
                backup.InitializeForBackup(null);

                backup.SetContext(VssSnapshotContext.All);

                return backup.QuerySnapshots().Count();
            }
        }

        public static void DeleteVolumeShadowCopy()
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.FileName = "vssadmin.exe";
            psi.Arguments = "delete shadows /all /quiet";
            psi.WindowStyle = ProcessWindowStyle.Hidden;
            Process.Start(psi);
        }
    }
}
