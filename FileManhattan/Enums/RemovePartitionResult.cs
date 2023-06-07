namespace FileManhattan.Enums
{
    public enum PartitionRemoveResult
    {
        SUCCESS,
        INVALID_HANDLE,
        RETRIEVE_DISK_INFO_FAILED,
        LOCK_FAILED,
        DISMOUNT_FAILED,
        SEEK_FAILED,
        WRITE_FAILED,
        EXCEPTION
    }
}