using FileManhattan.Enums;

namespace FileManhattan.Interfaces
{
    public interface IRemoveFile
    {
        string FileName { get; set; }
        long FileSize { get; set; }
        RemoveAlgorithm Mode { get; set; }
        string ProgressStatus { get; set; }
        bool IsComplete { get; set; }
    }
}
