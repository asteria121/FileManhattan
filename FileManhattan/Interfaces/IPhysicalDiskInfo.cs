namespace FileManhattan.Interfaces
{
    public interface IPhysicalDiskInfo
    {
        int DiskNo { get; }
        char DiskLabel { get; }
        bool IsSSD { get; }
        string DiskModel { get; }
    }
}
