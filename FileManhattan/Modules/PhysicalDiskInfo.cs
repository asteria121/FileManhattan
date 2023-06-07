using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Management;
using System.Threading.Tasks;
using FileManhattan.Interfaces;

namespace FileManhattan.Modules
{
    public class PhysicalDiskInfo : IPhysicalDiskInfo
    {
        public int DiskNo { get; }
        public char DiskLabel { get; }
        public bool IsSSD { get; }
        public string DiskModel { get; }

        private static List<PhysicalDiskInfo>? instance;

        public PhysicalDiskInfo(int diskNo, char diskLabel, bool isSSD, string diskModel)
        {
            DiskNo = diskNo;
            DiskLabel = diskLabel;
            IsSSD = isSSD;
            DiskModel = diskModel;
        }

        public static bool IsSolidStateDriveByLabel(char driveLabel)
        {
            if (instance != null)
            {
                foreach (var disk in instance)
                {
                    if (disk.DiskLabel == driveLabel)
                        return disk.IsSSD;
                }
            }

            return false;
        }

        public static async Task<List<PhysicalDiskInfo>> GetPhysicalDiskInformationAsync()
        {
            if (instance != null)
                return instance;

            List<PhysicalDiskInfo> tmp = new List<PhysicalDiskInfo>();

            var scope = new ManagementScope(@"\\.\root\cimv2");
            scope.Connect();

            // 물리 디스크 목록 가져오기
            var query = new SelectQuery("SELECT * FROM Win32_DiskDrive");
            var searcher = new ManagementObjectSearcher(scope, query);

            try
            {
                await Task.Run(() =>
                {
                    foreach (ManagementObject disk in searcher.Get())
                    {
                        var deviceId = disk["DeviceID"];
                        var diskNumber = Convert.ToInt32(disk["Index"]);
                        var partitions = new ManagementObjectSearcher("ASSOCIATORS OF {Win32_DiskDrive.DeviceID='" + deviceId + "'} WHERE AssocClass = Win32_DiskDriveToDiskPartition");

                        foreach (var partition in partitions.Get())
                        {
                            var logicalDisks = new ManagementObjectSearcher("ASSOCIATORS OF {Win32_DiskPartition.DeviceID='" + partition["DeviceID"] + "'} WHERE AssocClass = Win32_LogicalDiskToPartition");
                            foreach (var logicalDisk in logicalDisks.Get())
                            {
                                PhysicalDiskInfo info = new PhysicalDiskInfo(diskNumber, $"{logicalDisk["Name"]}".ToUpper()[0], Utility.IsSolidStateDrive(diskNumber), $"{disk["Model"]}");
                                tmp.Add(info);
                            }
                        }

                    }
                    instance = tmp.OrderBy(x => x.DiskLabel).ToList();
                });
                
            }
            catch
            {
                
            }
            finally
            {
                searcher.Dispose();
            }

            instance ??= new List<PhysicalDiskInfo>();
            return instance;
        }
    }
}
