using FileManhattan.Enums;

namespace FileManhattan.Interfaces
{
    public interface ICryptoFile
    {
        string FileName { get; set; }
        long FileSize { get; set; }
        bool IsEncrypt { get; set; }
        bool IsRemoveAfter { get; set; }
        RemoveAlgorithm Mode { get; set; }
        string ProgressStatus { get; set; }
        bool IsComplete { get; set; }
    }
}
