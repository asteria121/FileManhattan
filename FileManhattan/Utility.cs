using System;
using System.Diagnostics;
using System.Management;
using System.Runtime.InteropServices;
using System.Security.Principal;

namespace FileManhattan
{
    public static class Utility
    {
        [DllImport("ManhattanNative.dll")]
        public static extern bool IsSolidStateDrive(int driveNo);

        private static Stopwatch? TimeCalc;
        private static readonly string[] sizeSuffixes = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

        public static int GetPhysicalDriveNumber(string driveLabel)
        {
            string query = string.Format("SELECT * FROM Win32_Volume WHERE Label='{0}'", driveLabel);
            ManagementObjectSearcher searcher = new ManagementObjectSearcher(query);
            foreach (ManagementObject obj in searcher.Get())
            {
                string? path = obj["DeviceID"].ToString();
                string? driveLetter = path?.Substring(0, 2);

                if (driveLetter == null)
                    return -1;

                ManagementObject disk = new ManagementObject("win32_logicaldisk.deviceid=\"" + driveLetter + "\"");
                disk.Get();
                string? physicalDeviceID = disk["physicaldeviceid"].ToString();
                if (physicalDeviceID == null)
                    return -1;
                else
                {
                    int index = physicalDeviceID.LastIndexOf("\\") + 1;
                    return index;
                }
            }
            return -1;
        }

        public static string GetTimeFormat(long seconds)
        {
            TimeSpan time = TimeSpan.FromMilliseconds(seconds);
            string format = "{0:%s}초";
            if (time.Minutes > 0)
                format = "{0:%m}분 " + format;
            if (time.Hours > 0)
                format = "{0:%h}시간 " + format;
            if (time.Hours > 0)
                format = "{0:%d}일 " + format;

            return string.Format(format, time);
        }

        public static Stopwatch InitializeTimer()
        {
            if (TimeCalc == null)
                TimeCalc = new Stopwatch();

            return TimeCalc;
        }

        public static void KillAllProcess()
        {
            int currentPid = Process.GetCurrentProcess().Id;

            foreach (var process in Process.GetProcesses())
            {
                if (process.Id == currentPid)
                    process.Kill();
            }
        }

        public static bool IsAdministrator()
        {
            WindowsIdentity identity = WindowsIdentity.GetCurrent();
            if (null != identity)
            {
                WindowsPrincipal principal = new WindowsPrincipal(identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
            return false;
        }

        public static string ConvertFileSize(long? fileSize)
        {
            if (fileSize == null)
                return "0 B";

            double size = Convert.ToDouble(fileSize);
            int suffixIndex = 0;

            while (size >= 1024 && suffixIndex < sizeSuffixes.Length - 1)
            {
                size /= 1024;
                suffixIndex++;
            }

            return string.Format("{0:0.#} {1}", size, sizeSuffixes[suffixIndex]);
        }
    }
}
