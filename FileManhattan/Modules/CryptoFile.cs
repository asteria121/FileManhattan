using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using FileManhattan.Enums;
using FileManhattan.Interfaces;
using MahApps.Metro.Controls.Dialogs;

namespace FileManhattan.Modules
{
    public class CryptoFile : ICryptoFile
    {
        [DllImport("ManhattanNative.dll")]
        private static extern CryptoResult Encrypt(string inputPath, string outputPath, string originalKey, CryptoCallback callback);
        [DllImport("ManhattanNative.dll")]
        private static extern CryptoResult Decrypt(string inputPath, string outputPath, string originalKey, CryptoCallback callback);


        #region UIUpdateCallback
        [StructLayout(LayoutKind.Sequential, Pack = 1)]  // 패딩 제거
        public struct ProgressInfo
        {
            public double progressRate;
            public long totalBytesWritten;
        }

        delegate void CryptoCallback(ProgressInfo info);
        static void CryptoUpdateCallback(ProgressInfo info)
        {
            // 0을 나누는 오류가 발생하지 않도록 방지한다.
            if (TotalBytesWritten + info.totalBytesWritten == 0)
                return;

            double totalRate = (double)(TotalBytesWritten + info.totalBytesWritten) / TotalBytes;
            // 클러스터 크기 단위로 덮어씌우기 때문에 파일 전체 용량보다 쓰기 용량이 클 수 있음.
            // 1.0이 넘어가면 예외가 발생하므로 1.0을 넘으면 1.0으로 고정시킨다.
            if (totalRate > 1.0) totalRate = 1.0;

            long eta = (long)(Utility.InitializeTimer().ElapsedMilliseconds * (1 - totalRate) / totalRate);

            progressDialog?.SetProgress(totalRate);
            progressDialog?.SetMessage($"현재 작업중: {CurrentFile?.FileName} ({(int)(info.progressRate * 100)}%)" +
                $"\r\n남은 시간: 약 {Utility.GetTimeFormat(eta)}" +
                $"\r\n남은 파일: {GetStandbyTasks()} 개");
        }
       
        private static long TotalBytes;
        private static long TotalBytesWritten;
        private static ProgressDialogController? progressDialog;
        private static CryptoFile? CurrentFile;
#endregion

        public string FileName { get; set; }
        public long FileSize { get; set; }
        public bool IsEncrypt { get; set; }
        public bool IsRemoveAfter { get; set; }
        public RemoveAlgorithm Mode { get; set; }
        public string ProgressStatus { get; set; }
        public bool IsComplete { get; set; }

        private static List<CryptoFile>? instance;
        public static List<CryptoFile> GetInstance()
        {
            if (instance == null)
                instance = new List<CryptoFile>();

            return instance;
        }

        public static int GetStandbyTasks()
        {
            return GetInstance().Where(x => x.IsComplete == false).Count();
        }

        public CryptoFile(string fileName, long fileSize, bool isEncrypted, bool isRemoveAfter, RemoveAlgorithm mode)
        {
            FileName = fileName;
            FileSize = fileSize;
            IsEncrypt = isEncrypted;
            IsRemoveAfter = isRemoveAfter;
            Mode = mode;
            ProgressStatus = "대기중";
            IsComplete = false;
        }

        public static async Task StartCryptoAsync(string password, ProgressDialogController _progressDialog)
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
                            TotalBytes += item.FileSize;
                    }

                    foreach (var file in GetInstance())
                    {
                        if (file.IsComplete) continue;
                        CurrentFile = file;

                        CryptoResult result;
                        try
                        {
                            if (file.IsEncrypt)
                                result = Encrypt(file.FileName, file.FileName + ".fme", password, CryptoUpdateCallback);
                            else
                                result = Decrypt(file.FileName, file.FileName.Substring(0, file.FileName.Length - 4), password, CryptoUpdateCallback);

                            switch (result)
                            {
                                case CryptoResult.SUCCESS:
                                    file.ProgressStatus = file.IsEncrypt ? "암호화 성공" : "복호화 성공";
                                    if (file.IsRemoveAfter && file.IsEncrypt)
                                        RemoveFile.GetInstance().Add(new RemoveFile(file.FileName, file.FileSize, file.Mode));
                                    
                                    break;
                                case CryptoResult.WRONG_FILE:
                                    file.ProgressStatus = "실패: FileManhattan으로 암호화된 파일이 아닙니다";
                                    break;
                                case CryptoResult.WRONG_PASSWORD:
                                    file.ProgressStatus = "실패: 비밀번호가 틀립니다";
                                    break;
                                case CryptoResult.INPUT_NOT_EXISTS:
                                    file.ProgressStatus = "실패: 존재하지 않는 파일입니다";
                                    break;
                                case CryptoResult.OUTPUT_NOT_CREATABLE:
                                    file.ProgressStatus = "실패: 복호화 파일을 생성할 수 없습니다";
                                    break;
                                case CryptoResult.EXCEPTION:
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
                            TotalBytesWritten += file.FileSize;
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
