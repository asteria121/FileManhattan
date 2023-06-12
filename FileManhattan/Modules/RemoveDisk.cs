using FileManhattan.Enums;
using FileManhattan.Interfaces;
using MahApps.Metro.Controls.Dialogs;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows;

namespace FileManhattan.Modules
{
    public class RemoveDisk : IRemoveDisk
    {
        [DllImport("ManhattanNative.dll")]
        private static extern RemoveClusterTipResult RemoveClusterTip(string inputFile, RemoveAlgorithm mode);

        [DllImport("ManhattanNative.dll")]
        private static extern RemoveEmptyResult RemoveEmptySpace(string inputDisk, RemoveAlgorithm mode, EmptyCallback emptyUpdateCallback);

        [DllImport("ManhattanNative.dll")]
        private static extern RemoveMFTResult WipeMFT(string inputDisk);

        [DllImport("ManhattanNative.dll")]
        private static extern PartitionRemoveResult SecureRemovePartition(string inputDisk, RemoveAlgorithm mode, EmptyCallback partitionUpdateCallback);

        #region UIUpdateCallback
        [StructLayout(LayoutKind.Sequential, Pack = 1)]  // 패딩 제거
        public struct EmptyProgressInfo
        {
            public double progressRate;
            public long totalBytesWritten;
            public string currentWork;
            public int isFinished;
        }

        delegate void EmptyCallback(EmptyProgressInfo info);
        static void EmptyUpdateCallback(EmptyProgressInfo info)
        {
            // 0을 나누는 오류가 발생하지 않도록 방지한다.
            if (TotalBytesWritten + info.totalBytesWritten == 0)
                return;

            if (info.isFinished != 0)
            {
                if (CurrentDrive != null)
                    BytesWrittenOnFile += CurrentDrive.Size;
                return;
            }

            double totalRate = (double)(TotalBytesWritten + BytesWrittenOnFile + info.totalBytesWritten) / TotalBytes;
            // 클러스터 크기 단위로 덮어씌우기 때문에 파일 전체 용량보다 쓰기 용량이 클 수 있음.
            // 1.0이 넘어가면 예외가 발생하므로 1.0을 넘으면 1.0으로 고정시킨다.
            if (totalRate > 1.0) totalRate = 1.0;

            long eta = (long)(Utility.InitializeTimer().ElapsedMilliseconds * (1 - totalRate) / totalRate);

            //string bytesPerSec = Utility.ConvertFileSize((TotalBytesWritten + BytesWrittenOnFile + info.totalBytesWritten) / Utility.InitializeTimer().ElapsedMilliseconds * 1000) + "/s";
            progressDialog?.SetProgress(totalRate);
            progressDialog?.SetMessage($"현재 작업중: {CurrentDrive?.DriveLabel}:\\" +
                $"\r\n{Utility.ConvertFileSize(BytesWrittenOnFile + info.totalBytesWritten)} / {Utility.ConvertFileSize(CurrentDrive?.Size)} ({(int)(info.progressRate * 100)}%)" +
                $"\r\n{info.currentWork}" +
                $"\r\n남은 시간: 약 {Utility.GetTimeFormat(eta)}" +
                $"\r\n남은 작업 수: {GetStandbyTasks()} 개");
        }

        // finally 문을 통해 빠져나올때 TotalBytes에 값이 더해짐.
        // BytesWrittenOnFile은 완료 콜백을 받았을 때 삭제 대상의 크기만큼 더해짐.
        // 최종적으로 TotalBytesWritten + BytesWrittenOnFile + info.totalBytesWritten은 항상 작업이 진행된 바이트 수만을 나타낸다.
        private static long TotalBytes;                             // 작성해야하는 총 바이트 수
        private static long TotalBytesWritten;                      // 현재까지 확정적으로 작성된 바이트 수 (BytesWrittenOnFile 제외)
        private static long BytesWrittenOnFile;                     // 현재 작업중인 드라이브에 작성된 바이트 수
        public static RemoveDisk? CurrentDrive;                     // 현재 작업중인 드라이브
        private static ProgressDialogController? progressDialog;
        #endregion

        public DriveInfo DriveInformation { get; }
        public string DriveLabel { get; }
        public long Size { get; }
        public DiskRemoveMethod DiskRemoveMethod { get; }
        public RemoveAlgorithm Mode { get; set; }
        public bool IsWipeMFT { get; set; }
        public bool IsWipeClusterTip { get; set; }
        public string ProgressStatus { get; set; }
        public bool IsComplete { get; set; }
        private static List<RemoveDisk>? instance;
        public static List<RemoveDisk> GetInstance()
        {
            if (instance == null)
                instance = new List<RemoveDisk>();

            return instance;
        }

        public static int GetStandbyTasks()
        {
            return GetInstance().Where(x => x.IsComplete == false).Count();
        }

        public RemoveDisk(string driveLabel, DiskRemoveMethod diskRemoveMethod, RemoveAlgorithm mode, bool wipeMFT, bool wipeClusterTip)
        {
            DriveInformation = new DriveInfo(driveLabel);
            DriveLabel = driveLabel;
            if (diskRemoveMethod == DiskRemoveMethod.FreeSpace)
            {
                Size = DriveInformation.TotalFreeSpace;
            }
            else // EntirePartiton
            {
                Size = DriveInformation.TotalSize;
            }

            DiskRemoveMethod = diskRemoveMethod;
            Mode = mode;
            IsWipeMFT = wipeMFT;
            IsWipeClusterTip = wipeClusterTip;
            ProgressStatus = "대기중";
            IsComplete = false;
        }

        public static bool IsExistsDrive(char driveLabel)
        {
            foreach (var disk in GetInstance())
            {
                if (disk?.DriveLabel == driveLabel.ToString().ToUpper() && disk?.IsComplete == false)
                    return true;
            }

            return false;
        }

        public static bool IsSystemDrive(char driveLabel)
        {

            return false;
        }

        public static void RemoveClusterTipOnFiles(string path, RemoveAlgorithm mode)
        {
            try
            {
                foreach (string file in Directory.GetFiles(path))
                {
                    RemoveClusterTip(file, mode);
                }

                foreach (string directory in Directory.GetDirectories(path))
                {
                    RemoveClusterTipOnFiles(directory, mode);
                }
            }
            catch (UnauthorizedAccessException)
            {

            }
        }

        public static async Task StartRemoveDiskAsync(ProgressDialogController _progressDialog)
        {
            progressDialog = _progressDialog;
            try
            {
                Utility.InitializeTimer().Start();
                TotalBytes = 0;
                TotalBytesWritten = 0;

                await Task.Run(async () =>
                {
                    // 총 작업할 바이트 수를 구한다. 이를 예상 시간 계산에 사용함.
                    foreach (var item in GetInstance())
                    {
                        // 완료된 작업은 작업할 크기에 포함하지 않는다.
                        if (!item.IsComplete)
                        {
                            switch (item.Mode)
                            {
                                case RemoveAlgorithm.ThreePass:
                                    TotalBytes += item.Size * 3;
                                    break;
                                case RemoveAlgorithm.SevenPass:
                                    TotalBytes += item.Size * 7;
                                    break;
                                case RemoveAlgorithm.ThirtyFivePass:
                                    TotalBytes += item.Size * 35;
                                    break;
                                default:
                                    TotalBytes += item.Size;
                                    break;
                            }
                        }

                    }

                    foreach (var disk in GetInstance())
                    {
                        if (disk.IsComplete) continue;                      // 완료된 파일은 패스

                        CurrentDrive = disk;
                        BytesWrittenOnFile = 0;

                        try
                        {
                            if (disk.DiskRemoveMethod == DiskRemoveMethod.FreeSpace)
                            {
                                RemoveEmptyResult result = RemoveEmptySpace(disk.DriveInformation.Name, disk.Mode, EmptyUpdateCallback); // 빈공간 와이핑
                                if (disk.IsWipeClusterTip)
                                    RemoveClusterTipOnFiles(disk.DriveInformation.Name, disk.Mode);             // 기존 파일들의 ClusterTip 덮어쓰기
                                if (disk.IsWipeMFT)
                                    WipeMFT(disk.DriveInformation.Name);

                                switch (result)
                                {
                                    case RemoveEmptyResult.SUCCESS:
                                        disk.ProgressStatus = "삭제 성공";
                                        break;
                                    case RemoveEmptyResult.INVALID_HANDLE:
                                        disk.ProgressStatus = "실패: 올바르지 않은 디스크 핸들";
                                        break;
                                    case RemoveEmptyResult.SEEK_FAILED:
                                        disk.ProgressStatus = "실패: 디스크 탐색 위치 이동 실패";
                                        break;
                                    case RemoveEmptyResult.WRITE_FAILED:
                                        disk.ProgressStatus = "실패: 디스크 쓰기 작업 실패";
                                        break;
                                    case RemoveEmptyResult.RETRIEVE_EMPTY_SPACE_FAILED:
                                        disk.ProgressStatus = "실패: 빈공간 크기 확인 실패";
                                        break;
                                    case RemoveEmptyResult.EXCEPTION:
                                        // TODO: ex.what() 메세지를 작성할 수 있도록 한다.
                                        disk.ProgressStatus = $"실패: 예외 발생";
                                        break;
                                    default:
                                        disk.ProgressStatus = "실패: 알 수 없는 에러";
                                        break;
                                }
                            }
                            else
                            {
                                PartitionRemoveResult result = SecureRemovePartition(disk.DriveInformation.Name, disk.Mode, EmptyUpdateCallback); // 디스크 전체 덮어쓰기

                                switch (result)
                                {
                                    case PartitionRemoveResult.SUCCESS:
                                        disk.ProgressStatus = "삭제 성공";
                                        break;
                                    case PartitionRemoveResult.INVALID_HANDLE:
                                        disk.ProgressStatus = "실패: 올바르지 않은 디스크 핸들";
                                        break;
                                    case PartitionRemoveResult.RETRIEVE_DISK_INFO_FAILED:
                                        disk.ProgressStatus = "실패: 디스크 정보 획득 실패";
                                        break;
                                    case PartitionRemoveResult.LOCK_FAILED:
                                        disk.ProgressStatus = "실패: 디스크 잠금 실패";
                                        break;
                                    case PartitionRemoveResult.DISMOUNT_FAILED:
                                        disk.ProgressStatus = "실패: 디스크 마운트 해제 실패";
                                        break;
                                    case PartitionRemoveResult.SEEK_FAILED:
                                        disk.ProgressStatus = "실패: 디스크 탐색 위치 이동 실패";
                                        break;
                                    case PartitionRemoveResult.WRITE_FAILED:
                                        disk.ProgressStatus = "실패: 디스크 쓰기 작업 실패";
                                        break;
                                    case PartitionRemoveResult.EXCEPTION:
                                        // TODO: ex.what() 메세지를 작성할 수 있도록 한다.
                                        disk.ProgressStatus = $"실패: 예외 발생";
                                        break;
                                    default:
                                        disk.ProgressStatus = "실패: 알 수 없는 에러";
                                        break;
                                }
                            }
                            
                        }
                        catch (Exception ex)
                        {
                            disk.ProgressStatus = $"실패: 예외 발생 [{ex.Message}]";
                        }
                        finally
                        {
                            disk.IsComplete = true;
                            switch (disk.Mode)
                            {
                                case RemoveAlgorithm.ThreePass:
                                    TotalBytesWritten += disk.Size * 3;
                                    break;
                                case RemoveAlgorithm.SevenPass:
                                    TotalBytesWritten += disk.Size * 7;
                                    break;
                                case RemoveAlgorithm.ThirtyFivePass:
                                    TotalBytesWritten += disk.Size * 35;
                                    break;
                                default:
                                    TotalBytesWritten += disk.Size;
                                    break;
                            }
                        }
                    }
                });
            }
            finally
            {
                Utility.InitializeTimer().Stop();
                Utility.InitializeTimer().Reset();
                CurrentDrive = null;
            }
        }
    }
}
