using FileManhattan.Enums;
using System.IO;

namespace FileManhattan.Interfaces
{
    public interface IRemoveDisk
    {
        DriveInfo DriveInformation { get; }
        string DriveLabel { get; }
        long Size { get; }
        DiskRemoveMethod DiskRemoveMethod { get; }
        RemoveAlgorithm Mode { get; set; }
        bool IsWipeMFT { get; set; }
        bool IsWipeClusterTip { get; set; }
        string ProgressStatus { get; set; }
        bool IsComplete { get; set; }
    }
}
