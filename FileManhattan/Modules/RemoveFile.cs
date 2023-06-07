using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using FileManhattan.Enums;
using FileManhattan.Interfaces;
using MahApps.Metro.Controls.Dialogs;
using System.IO;

namespace FileManhattan.Modules
{
    public class RemoveFile : IRemoveFile
    {
        [DllImport("ManhattanNative.dll")]
        private static extern RemoveResult SecureRemoveFile(string inputFile, RemoveAlgorithm mode, RemoveCallback callback);

        #region UIUpdateCallback
        [StructLayout(LayoutKind.Sequential, Pack = 1)]  // 패딩 제거
        public struct RemoveProgressInfo
        {
            public double progressRate;
            public long totalBytesWritten;
            public string currentWork;
            public int isFinished;
        }

        public delegate void RemoveCallback(RemoveProgressInfo info);
        public static void RemoveUpdateCallback(RemoveProgressInfo info)
        {
            // 0을 나누는 오류가 발생하지 않도록 방지한다.
            if (TotalBytesWritten + info.totalBytesWritten == 0)
                return;

            if (info.isFinished != 0)
            {
                if (CurrentFile != null)
                    BytesWrittenOnFile += CurrentFile.FileSize;
                return;
            }

            double totalRate = (double)(TotalBytesWritten + BytesWrittenOnFile + info.totalBytesWritten) / TotalBytes;
            // 클러스터 크기 단위로 덮어씌우기 때문에 파일 전체 용량보다 쓰기 용량이 클 수 있음.
            // 1.0이 넘어가면 예외가 발생하므로 1.0을 넘으면 1.0으로 고정시킨다.
            if (totalRate > 1.0) totalRate = 1.0;

            long eta = (long)(Utility.InitializeTimer().ElapsedMilliseconds * (1 - totalRate) / totalRate);

            //string bytesPerSec = Utility.ConvertFileSize((TotalBytesWritten + BytesWrittenOnFile + info.totalBytesWritten) / Utility.InitializeTimer().ElapsedMilliseconds * 1000) + "/s";
            progressDialog?.SetProgress(totalRate);
            progressDialog?.SetMessage($"현재 작업중: {CurrentFile?.FileName}" +
                $"\r\n{Utility.ConvertFileSize(BytesWrittenOnFile + info.totalBytesWritten)} / {Utility.ConvertFileSize(CurrentFile?.FileSize)} ({(int)(info.progressRate * 100)}%)" +
                $"\r\n{info.currentWork}" +
                $"\r\n남은 시간: 약 {Utility.GetTimeFormat(eta)}" +
                $"\r\n남은 파일: {GetStandbyTasks()} 개");
        }

        // finally 문을 통해 빠져나올때 TotalBytes에 값이 더해짐.
        // BytesWrittenOnFile은 완료 콜백을 받았을 때 삭제 대상의 크기만큼 더해짐.
        // 최종적으로 TotalBytesWritten + BytesWrittenOnFile + info.totalBytesWritten은 항상 작업이 진행된 바이트 수만을 나타낸다.
        private static long TotalBytes;                             // 작성해야하는 총 바이트 수
        private static long TotalBytesWritten;                      // 현재까지 확정적으로 작성된 바이트 수 (BytesWrittenOnFile 제외)
        private static long BytesWrittenOnFile;                     // 현재 작업중인 파일에 작성된 바이트 수
        private static RemoveFile? CurrentFile;                     // 현재 작업중인 파일
        private static ProgressDialogController? progressDialog;
        #endregion

        public string FileName { get; set; }
        public long FileSize { get; set; }
        public RemoveAlgorithm Mode { get; set; }
        public string ProgressStatus { get; set; }
        public bool IsComplete { get; set; }

        public RemoveFile(string fileName, long fileSize, RemoveAlgorithm mode)
        {
            FileName = fileName;
            FileSize = fileSize;
            Mode = mode;
            ProgressStatus = "대기중";
        }

        private static List<RemoveFile>? instance;
        public static List<RemoveFile> GetInstance()
        {
            if (instance == null)
                instance = new List<RemoveFile>();

            return instance;
        }

        public static int GetStandbyTasks()
        {
            return GetInstance().Where(x => x.IsComplete == false).Count();
        }

        public static async Task StartRemoveAsync(ProgressDialogController _progressDialog)
        {
            progressDialog = _progressDialog;

            try
            {
                Utility.InitializeTimer().Start();
                TotalBytes = 0;
                TotalBytesWritten = 0;

                await Task.Run(() =>
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
                                    TotalBytes += item.FileSize * 3;
                                    break;
                                case RemoveAlgorithm.SevenPass:
                                    TotalBytes += item.FileSize * 7;
                                    break;
                                case RemoveAlgorithm.ThirtyFivePass:
                                    TotalBytes += item.FileSize * 35;
                                    break;
                                default:
                                    TotalBytes += item.FileSize;
                                    break;
                            }
                        }
                    }

                    // 파일 삭제
                    foreach (var file in GetInstance())
                    {
                        if (file.IsComplete) continue;                      // 완료된 파일은 패스
                        if (Directory.Exists(file.FileName)) continue;      // 폴더는 파일을 모두 삭제 후 따로 진행

                        CurrentFile = file;
                        BytesWrittenOnFile = 0;

                        RemoveResult result;
                        try
                        {
                            CurrentFile = file;
                            if (file.Mode == RemoveAlgorithm.NormalDelete)
                            {
                                if (File.Exists(file.FileName))
                                    File.Delete(file.FileName);

                                result = RemoveResult.SUCCESS;
                            }
                            result = SecureRemoveFile(file.FileName, file.Mode, RemoveUpdateCallback);

                            switch (result)
                            {
                                case RemoveResult.SUCCESS:
                                    file.ProgressStatus = "삭제 성공";
                                    break;
                                case RemoveResult.INVALID_HANDLE:
                                    file.ProgressStatus = "실패: 올바르지 않은 파일 핸들";
                                    break;
                                case RemoveResult.SEEK_FAILED:
                                    file.ProgressStatus = "실패: 파일 탐색 위치 이동 실패";
                                    break;
                                case RemoveResult.WRITE_FAILED:
                                    file.ProgressStatus = "실패: 파일 쓰기 실패";
                                    break;
                                case RemoveResult.EXCEPTION:
                                    // TODO: ex.what() 메세지를 작성할 수 있도록 한다.
                                    file.ProgressStatus = $"실패: 예외 발생";
                                    break;
                                default:
                                    file.ProgressStatus = "실패: 알 수 없는 에러";
                                    break;
                            }
                        }
                        catch (Exception ex)
                        {
                            file.ProgressStatus = $"실패: 예외 발생 [{ex.Message}]";
                        }
                        finally
                        {
                            file.IsComplete = true;
                            switch (file.Mode)
                            {
                                case RemoveAlgorithm.ThreePass:
                                    TotalBytesWritten += file.FileSize * 3;
                                    break;
                                case RemoveAlgorithm.SevenPass:
                                    TotalBytesWritten += file.FileSize * 7;
                                    break;
                                case RemoveAlgorithm.ThirtyFivePass:
                                    TotalBytesWritten += file.FileSize * 35;
                                    break;
                                default:
                                    TotalBytesWritten += file.FileSize;
                                    break;
                            }

                        }
                    }

                    // 폴더만 삭제
                    foreach (var file in GetInstance())
                    {
                        if (file.IsComplete) continue;

                        try
                        {
                            if (Directory.Exists(file.FileName))
                            {
                                // 폴더 내에 폴더가 아닌 파일이 존재하는지 확인 후 존재하면 삭제하지 못했다는 경고 출력
                                int fileCount = 0;
                                foreach (var tmp in Directory.GetFiles(file.FileName, "*.*", SearchOption.AllDirectories))
                                {
                                    if (File.Exists(tmp))
                                        fileCount++;
                                }

                                if (fileCount == 0)
                                {
                                    Directory.Delete(file.FileName, true);
                                    file.ProgressStatus = "삭제 성공";
                                }
                                else
                                {
                                    file.ProgressStatus = "실패: 하위 폴더에 파일이 존재합니다";
                                }        
                            }
                        }
                        catch (Exception ex)
                        {
                            file.ProgressStatus = $"실패: 예외 발생 [{ex.Message}]";
                        }
                        finally
                        {
                            file.IsComplete = true;
                        }
                    }
                });
            }
            finally
            {
                Utility.InitializeTimer().Stop();
                Utility.InitializeTimer().Reset();
                CurrentFile = null;
            }
        }
    }
}
