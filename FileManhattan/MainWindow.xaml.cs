using System;
using System.Data;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using MahApps.Metro.Controls;
using MahApps.Metro.Controls.Dialogs;
using FileManhattan.Enums;
using FileManhattan.Modules;

namespace FileManhattan
{
    public partial class MainWindow : MetroWindow
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private async void MetroWindow_Loaded(object sender, RoutedEventArgs e)
        {

            removeAlgorithmHelpTextBox.Text =
                "암호학적으로 안전한 의사 난수 생성기(Cryptographic Secure Pseudo Random Number Generator)를 이용하여 생성된 난수로 파일을 1회 덮어씌웁니다. " +
                "데이터 복원 프로그램으로 복구가 불가능해집니다." +
                        "\r\n\r\n일반적으로 복구 불능";

            if (!Utility.IsAdministrator())
            {
                await this.ShowMessageAsync("권한 부족", "본 프로그램은 관리자권한으로 실행해야합니다.");
                Application.Current.Shutdown();
            }

            await RefreshDiskList();

            // 볼륨 섀도우 자동 삭제
            if (Utility.IsAdministrator() && Vss.GetVssCount() > 0)
            {
                var result = await this.ShowMessageAsync("스냅샷 발견", "윈도우 스냅샷이 발견되었습니다. 스냅샷을 생성 시점의 파일 복구가 가능할 수 있습니다.\r\n모든 스냅샷을 삭제 하시겠습니까?" +
                    "\r\n백신 프로그램이 작업 중 프로그램을 종료시킬 수 있으나 정상적인 과정으로 잠시 실시간 감지를 꺼야할 수 있습니다.", MessageDialogStyle.AffirmativeAndNegative);
                if (result == MessageDialogResult.Affirmative)
                {
                    Vss.DeleteVolumeShadowCopy();
                }
            }

            // 빈공간 삭제를 진행할지 확인한다
            string firstTmpFile = AppDomain.CurrentDomain.BaseDirectory + "\\FirstExecution";
            if (!File.Exists(firstTmpFile))
            {
                File.Create(firstTmpFile);
                var list = await PhysicalDiskInfo.GetPhysicalDiskInformationAsync();
                if (list.Where(x => x.IsSSD).Any())
                {
                    var result = await this.ShowMessageAsync("환영합니다",
                        "하드디스크 드라이브가 발견되었습니다.\r\n" +
                        "기존에 삭제한 데이터 복원 방지를 위해 빈공간 덮어쓰기를 수행할 수 있습니다.\r\n" +
                        "예를 누르면 작업이 자동으로 추가되며 수동으로 작업 시작을 클릭해야 합니다.\r\n" +
                        "계속 하시겠습니까?", MessageDialogStyle.AffirmativeAndNegative);
                    if (result == MessageDialogResult.Affirmative)
                    {
                        mainTabControl.SelectedIndex = 1;

                        foreach (PhysicalDiskInfo disk in list)
                        {
                            try
                            {
                                if (disk.IsSSD || RemoveDisk.IsExistsDrive(disk.DiskLabel))
                                    continue;

                                // NTFS 에서만 MFT 크기를 구하기 때문에 NTFS 시스템만 MFT wipe를 실시함.
                                RemoveDisk removeDisk = new RemoveDisk(disk.DiskLabel.ToString(), DiskRemoveMethod.FreeSpace, RemoveAlgorithm.OnePass, true, false);
                                if (!string.Equals(removeDisk.DriveInformation.DriveFormat, "NTFS") && removeDisk.IsWipeMFT)
                                    removeDisk.IsWipeMFT = false;
                                RemoveDisk.GetInstance().Add(removeDisk);
                            }
                            catch
                            {

                            }
                            finally
                            {
                                diskDataGrid.ItemsSource ??= RemoveDisk.GetInstance();
                                diskDataGrid.Items.Refresh();
                            }
                        }
                    }
                }
            }
        }

        private void informationButton_Click(object sender, RoutedEventArgs e)
        {
            MetroWindow wnd = new InformationWindow();
            wnd.ShowDialog();
        }

        // ********************************************************
        // *                파일소거 관련 코드                     *
        // ********************************************************
        #region removefile
        private int ssdFileCount = 0;
        private async void removeStartButton_Click(object sender, RoutedEventArgs e)
        {
            // 완료되지 않은 작업이 0개인 경우
            if (RemoveFile.GetStandbyTasks() == 0)
            {
                await this.ShowMessageAsync("Error", "현재 대기중인 작업이 없습니다.");
                return;
            }

            if (await this.ShowMessageAsync("확실한가요?", $"이 작업은 절대 되돌릴 수 없습니다. 정말 {RemoveFile.GetStandbyTasks()}개의 파일을 정말 삭제하시겠습니까?",
                MessageDialogStyle.AffirmativeAndNegative) != MessageDialogResult.Affirmative)
                return;

            var progressDialog = await this.ShowProgressAsync("Please wait", "파일들의 크기를 계산중입니다. 잠시만 기다려주세요.");
            progressDialog.SetIndeterminate();
            try
            {
                await RemoveFile.StartRemoveAsync(progressDialog);
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.Message);
            }
            finally
            {
                removeDataGrid.Items.Refresh();
                if (progressDialog != null)
                    await progressDialog.CloseAsync();
            }

            await this.ShowMessageAsync("완료", "파일 소거 작업이 완료되었습니다.\r\n자세한 내용은 작업란의 상태를 확인해주세요.");
            // TODO: Dll 예외 발생시 System.Runtime.InteropServices.SEHException: 'External component has thrown an exception.'
        }

        private void removeAlgorithmComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (removeAlgorithmComboBox.SelectedIndex == 1)
            {
                removeAlgorithmHelpTextBox.Text =
                    "미국 국방부 표준 삭제 알고리즘입니다. 본 소프트웨어를 사용하는 대부분의 경우에 이 알고리즘 사용을 추천합니다." +
                    "\r\n\r\n복구 가능성 거의 없음";
            }
            else if (removeAlgorithmComboBox.SelectedIndex == 2)
            {
                removeAlgorithmHelpTextBox.Text =
                    "미국 국방부 표준 3회 + 난수로 1회 + 3회 반복 순으로 진행하는 알고리즘입니다. 시간이 더 소요되지만 3회 덮어쓰기보다 조금 더 안전합니다." +
                    "\r\n\r\n복구 가능성 사실상 없음";
            }
            else if (removeAlgorithmComboBox.SelectedIndex == 3)
            {
                removeAlgorithmHelpTextBox.Text =
                    "시간이 매우 오래걸리며 고용량 파일 혹은 디스크 전체를 삭제할 경우 하드디스크 수명에 악영향을 줄 수 있습니다." +
                    "\r\n\r\n확실한 데이터 소거를 보장합니다.";
            }
            else
            {
                if (removeAlgorithmHelpTextBox != null)
                    removeAlgorithmHelpTextBox.Text =
                        "암호학적으로 안전한 의사 난수 생성기(Cryptographic Secure Pseudo Random Number Generator)를 이용하여 생성된 난수로 파일을 1회 덮어씌웁니다. 데이터 복원 프로그램으로 복구가 불가능해집니다." +
                        "\r\n\r\n일반적으로 복구 불능";
            }
        }

        private async void DisplayAddItemResult()
        {
            if (ssdFileCount > 0)
            {
                await this.ShowMessageAsync("경고", $"SSD에 존재하는 파일은 추가하실 수 없습니다.\r\n{ssdFileCount}개의 파일 추가에 실패했습니다.");
                ssdFileCount = 0;
            }
        }

        private void AddRemoveItem(string path)
        {
            FileInfo fi = new FileInfo(path);
            DirectoryInfo di = new DirectoryInfo(path);

            // 이미 있는 파일 혹은 존재하지 않는 파일은 추가하지 않는다.
            if (RemoveFile.GetInstance().Where(x => x.FileName == fi.FullName).Count() > 0 || (!fi.Exists && !di.Exists))
                return;

            // SSD에 있는 파일은 추가하지 않는다
            if (PhysicalDiskInfo.IsSolidStateDriveByLabel(path.ElementAt(0)))
            {
                ssdFileCount++;
                return;
            }

            RemoveAlgorithm Mode = RemoveAlgorithm.OnePass;
            switch (removeAlgorithmComboBox.SelectedIndex)
            {
                case 0:
                    Mode = RemoveAlgorithm.OnePass;
                    break;
                case 1:
                    Mode = RemoveAlgorithm.ThreePass;
                    break;
                case 2:
                    Mode = RemoveAlgorithm.SevenPass;
                    break;
                case 3:
                    Mode = RemoveAlgorithm.ThirtyFivePass;
                    break;
                default:
                    Mode = RemoveAlgorithm.OnePass;
                    break;
            }

            if (fi.Exists)
                RemoveFile.GetInstance().Add(new RemoveFile(path, fi.Length, Mode));
            else
            {
                RemoveFile.GetInstance().Add(new RemoveFile(path, 0, Mode));
                foreach (var file in Directory.GetFiles(path, "*.*", SearchOption.AllDirectories))
                    AddRemoveItem(file);
            }

            return;
        }

        private void removeDataGrid_DragEnter(object sender, DragEventArgs e)
        {
            string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

            if (files != null)
                e.Effects = DragDropEffects.Copy;
        }

        private async void removeDataGrid_Drop(object sender, DragEventArgs e)
        {
            try
            {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

                if (files != null)
                {
                    foreach (string path in files)
                    {
                        if (File.Exists(path))              // 단일 파일이 존재하는 경우
                        {
                            AddRemoveItem(path);
                        }
                        else if (Directory.Exists(path))    // 폴더인 경우
                        {
                            // 모든 디렉토리 및 하위 디렉토리를 검색하여 각각 파일을 추가한다.
                            foreach (var file in Directory.GetFiles(path, "*.*", SearchOption.AllDirectories))
                                AddRemoveItem(file);
                        }
                        // 그 외의 경우는 파일이 없는 경우라 조건문이 필요 없음.
                    }
                }
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.ToString());
            }
            finally
            {
                removeDataGrid.ItemsSource ??= RemoveFile.GetInstance();
                removeDataGrid.Items.Refresh();
                DisplayAddItemResult();
            }
        }

        private void removeAddFileButton_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog dialog = new System.Windows.Forms.OpenFileDialog();
            dialog.Title = "삭제할 파일 선택";
            dialog.Multiselect = true;
            dialog.Filter = "모든 파일 (*.*) | *.*";

            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                foreach (var file in dialog.FileNames)
                    AddRemoveItem(file);

                removeDataGrid.ItemsSource ??= RemoveFile.GetInstance();
                removeDataGrid.Items.Refresh();
                DisplayAddItemResult();
            }
        }

        private void removeAddFolderButton_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.FolderBrowserDialog dialog = new System.Windows.Forms.FolderBrowserDialog();

            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                DirectoryInfo di = new DirectoryInfo(dialog.SelectedPath);
                if (di.Exists)
                {
                    AddRemoveItem(dialog.SelectedPath);

                    removeDataGrid.ItemsSource ??= RemoveFile.GetInstance();
                    removeDataGrid.Items.Refresh();
                    DisplayAddItemResult();
                }
            }
        }

        private void removeSettingsButton_Click(object sender, RoutedEventArgs e)
        {
            foreach (RemoveFile file in removeDataGrid.SelectedItems)
            {
                switch (removeAlgorithmComboBox.SelectedIndex)
                {
                    case 1:
                        file.Mode = RemoveAlgorithm.ThreePass;
                        break;
                    case 2:
                        file.Mode = RemoveAlgorithm.SevenPass;
                        break;
                    case 3:
                        file.Mode = RemoveAlgorithm.ThirtyFivePass;
                        break;
                    default:
                        file.Mode = RemoveAlgorithm.OnePass;
                        break;
                }
            }

            removeDataGrid.ItemsSource ??= RemoveFile.GetInstance();
            removeDataGrid.Items.Refresh();
        }

        private void removeRemoveButton_Click(object sender, RoutedEventArgs e)
        {
            foreach (RemoveFile file in removeDataGrid.SelectedItems)
                RemoveFile.GetInstance().Remove(file);

            removeDataGrid.ItemsSource ??= RemoveFile.GetInstance();
            removeDataGrid.Items.Refresh();
        }

        private void removeRemoveCompleteButton_Click(object sender, RoutedEventArgs e)
        {
            RemoveFile.GetInstance().RemoveAll(x => x.IsComplete);

            removeDataGrid.ItemsSource ??= RemoveFile.GetInstance();
            removeDataGrid.Items.Refresh();
        }
        #endregion

        // ********************************************************
        // *                파티션 관련 코드                       *
        // ********************************************************
        #region partition
        private async void emptyDiskHelpButton_Click(object sender, RoutedEventArgs e)
        {
            await this.ShowMessageAsync("디스크 빈공간 삭제",
                "윈도우 탐색기상에서 삭제되었지만 디스크 내에 삭제된 파일의 데이터가 존재할 수 있습니다.\r\n" +
                "빈공간 전체를 덮어씌워 해당 내용을 복구할 수 없도록 합니다.\r\n\r\n" +
                "기존 파일은 삭제되지 않습니다.");
        }

        private async void partitionHelpButton_Click(object sender, RoutedEventArgs e)
        {
            await this.ShowMessageAsync("파티션 전체 삭제",
                "디스크 빈공간 삭제보다 더 강력한 소거 방식입니다.\r\n" +
                "디스크 전체를 덮어씌우며 디스크의 내용이 전부 초기화됩니다.\r\n" +
                "이 작업은 절대 되돌릴 수 없으며 현재 사용중인 디스크는 선택할 수 없습니다.");
        }

        private async Task RefreshDiskList()
        {
            try
            {
                var list = await PhysicalDiskInfo.GetPhysicalDiskInformationAsync();
                if (list != null && list.Count > 0)
                {
                    DataTable dataTable = new DataTable();
                    dataTable.Columns.Add("VALUE", typeof(PhysicalDiskInfo));
                    dataTable.Columns.Add("NAME", typeof(string));

                    foreach (var hdd in list)
                    {
                        string isSSD = "HDD";
                        if (hdd.IsSSD)
                            isSSD = "SSD";

                        dataTable.Rows.Add(hdd, $"{hdd.DiskLabel}:\\ ({isSSD})");
                    }

                    emptyDiskComboBox.ItemsSource = dataTable.DefaultView;
                    emptyDiskComboBox.DisplayMemberPath = "NAME";
                    emptyDiskComboBox.SelectedValuePath = "VALUE";

                    partitionComboBox.ItemsSource = dataTable.DefaultView;
                    partitionComboBox.DisplayMemberPath = "NAME";
                    partitionComboBox.SelectedValuePath = "VALUE";

                    emptyDiskComboBox.SelectedIndex = 0;
                    partitionComboBox.SelectedIndex = 0;
                }
                else
                {
                    await this.ShowMessageAsync("Error", "디스크 목록을 불러오는데 실패했습니다.");
                }
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("디스크 목록 조회 실패", ex.ToString());
            }
        }

        private async void emptyDiskAddButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                PhysicalDiskInfo disk = (PhysicalDiskInfo)emptyDiskComboBox.SelectedValue;
                if (disk.IsSSD)
                {
                    await this.ShowMessageAsync("Error", "SSD에는 해당 작업을 수행할 수 없습니다.");
                    return;
                }

                if (RemoveDisk.IsExistsDrive(disk.DiskLabel))
                {
                    await this.ShowMessageAsync("Error", "이미 추가된 드라이브입니다.");
                    return;
                }

                RemoveAlgorithm mode = RemoveAlgorithm.ThreePass;
                switch (diskRemoveAlgorithmComboBox.SelectedIndex)
                {
                    case 0:
                        mode = RemoveAlgorithm.OnePass;
                        break;
                    case 1:
                        mode = RemoveAlgorithm.ThreePass;
                        break;
                    case 2:
                        mode = RemoveAlgorithm.SevenPass;
                        break;
                    case 3:
                        mode = RemoveAlgorithm.ThirtyFivePass;
                        break;
                    default:
                        mode = RemoveAlgorithm.OnePass;
                        break;
                }

                // NTFS 에서만 MFT 크기를 구하기 때문에 NTFS 시스템만 MFT wipe를 실시함.
                RemoveDisk removeDisk = new RemoveDisk(disk.DiskLabel.ToString(), DiskRemoveMethod.FreeSpace, mode, wipeMFTSwitch.IsOn, wipeClusterTipSwitch.IsOn);
                removeDisk.IsWipeMFT = string.Equals(removeDisk.DriveInformation.DriveFormat, "NTFS") & removeDisk.IsWipeMFT;
                RemoveDisk.GetInstance().Add(removeDisk);
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.ToString());
            }
            finally
            {
                diskDataGrid.ItemsSource ??= RemoveDisk.GetInstance();
                diskDataGrid.Items.Refresh();
            }
        }

        private async void partitionAddButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                PhysicalDiskInfo disk = (PhysicalDiskInfo)partitionComboBox.SelectedValue;
                if (disk.IsSSD)
                {
                    await this.ShowMessageAsync("Error", "SSD에는 해당 작업을 수행할 수 없습니다.");
                    return;
                }

                if (RemoveDisk.IsExistsDrive(disk.DiskLabel))
                {
                    await this.ShowMessageAsync("Error", "이미 추가된 드라이브입니다.");
                    return;
                }

                RemoveAlgorithm mode;
                switch (diskRemoveAlgorithmComboBox.SelectedIndex)
                {
                    case 0:
                        mode = RemoveAlgorithm.OnePass;
                        break;
                    case 1:
                        mode = RemoveAlgorithm.ThreePass;
                        break;
                    case 2:
                        mode = RemoveAlgorithm.SevenPass;
                        break;
                    case 3:
                        mode = RemoveAlgorithm.ThirtyFivePass;
                        break;
                    default:
                        mode = RemoveAlgorithm.OnePass;
                        break;
                }

                RemoveDisk.GetInstance().Add(new RemoveDisk(disk.DiskLabel.ToString(), DiskRemoveMethod.EntirePartition, mode, false, false));
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.ToString());
            }
            finally
            {
                diskDataGrid.ItemsSource ??= RemoveDisk.GetInstance();
                diskDataGrid.Items.Refresh();
            }
        }

        private void removeDiskSettingsButton_Click(object sender, RoutedEventArgs e)
        {
            foreach (RemoveDisk disk in diskDataGrid.SelectedItems)
            {
                if (disk.DiskRemoveMethod == DiskRemoveMethod.FreeSpace)
                {
                    // NTFS 에서만 MFT 크기를 구하기 때문에 NTFS 시스템만 MFT wipe를 실시함.
                    disk.IsWipeMFT = string.Equals(disk.DriveInformation.DriveFormat, "NTFS") & wipeMFTSwitch.IsOn;
                    disk.IsWipeClusterTip = wipeClusterTipSwitch.IsOn;
                }

                RemoveAlgorithm mode;
                switch (diskRemoveAlgorithmComboBox.SelectedIndex)
                {
                    case 0:
                        mode = RemoveAlgorithm.OnePass;
                        break;
                    case 1:
                        mode = RemoveAlgorithm.ThreePass;
                        break;
                    case 2:
                        mode = RemoveAlgorithm.SevenPass;
                        break;
                    case 3:
                        mode = RemoveAlgorithm.ThirtyFivePass;
                        break;
                    default:
                        mode = RemoveAlgorithm.OnePass;
                        break;
                }
                disk.Mode = mode;
            }

            diskDataGrid.ItemsSource ??= RemoveDisk.GetInstance();
            diskDataGrid.Items.Refresh();
        }
        
        private void removeDiskRemoveButton_Click(object sender, RoutedEventArgs e)
        {
            foreach (RemoveDisk disk in diskDataGrid.SelectedItems)
                RemoveDisk.GetInstance().Remove(disk);

            diskDataGrid.ItemsSource ??= RemoveDisk.GetInstance();
            diskDataGrid.Items.Refresh();
        }

        private void removeDiskRemoveCompleteButton_Click(object sender, RoutedEventArgs e)
        {
            RemoveDisk.GetInstance().RemoveAll(x => x.IsComplete);

            diskDataGrid.ItemsSource ??= RemoveDisk.GetInstance();
            diskDataGrid.Items.Refresh();
        }

        private async void removeDiskStartButton_Click(object sender, RoutedEventArgs e)
        {
            // 완료되지 않은 작업이 0개인 경우
            if (RemoveDisk.GetStandbyTasks() == 0)
            {
                await this.ShowMessageAsync("Error", "현재 대기중인 작업이 없습니다.");
                return;
            }

            // 진행하려는 작업 크기를 계산한다.
            long totalSize = 0;
            foreach (var disk in RemoveDisk.GetInstance().Where(x => x.IsComplete == false))
            {
                if (disk.Mode == RemoveAlgorithm.ThreePass)
                    totalSize += disk.Size * 3;
                else if (disk.Mode == RemoveAlgorithm.SevenPass)
                    totalSize += disk.Size * 7;
                else if (disk.Mode == RemoveAlgorithm.ThirtyFivePass)
                    totalSize += disk.Size * 35;
                else 
                    totalSize += disk.Size;
            }

            // 진행 전 충분한 경고 메세지를 출력한다.
            StringBuilder sb = new StringBuilder();
            sb.AppendLine("- 진행하려는 작업은 상당한 시간이 소요됩니다.");
            if (RemoveDisk.GetInstance().Where(x => x.IsComplete == false && x.DiskRemoveMethod == DiskRemoveMethod.FreeSpace).Count() > 0)
            {
                sb.AppendLine("- 빈공간 덮어쓰기는 존재하는 파일은 제거하지 않습니다.");
                if (RemoveDisk.GetInstance().Where(x => x.IsComplete == false && x.DiskRemoveMethod == DiskRemoveMethod.FreeSpace && x.IsWipeClusterTip).Count() > 0)
                {
                    sb.AppendLine("- 클러스터 팁 덮어쓰기는 실행중인 다른 프로그램을 종료하는게 좋습니다.");
                }
            }
            if (RemoveDisk.GetInstance().Where(x => x.IsComplete == false && x.DiskRemoveMethod == DiskRemoveMethod.EntirePartition).Count() > 0)
            {
                sb.AppendLine("- 디스크에서 사용중인 파일이 있을 경우 해당 디스크의 전체 삭제는 실행되지 않습니다.");
                sb.AppendLine("- 디스크 전체가 포맷됩니다. 중요한 파일은 백업하시기 바랍니다.");
            }
            if (RemoveDisk.GetInstance().Where(x => x.IsComplete == false && (x.Mode == RemoveAlgorithm.SevenPass) || x.Mode == RemoveAlgorithm.ThirtyFivePass).Count() > 0)
            {
                sb.AppendLine("- 진행하려는 작업에 7번 이상 덮어쓰기는 하드디스크 수명에 악영향을 줄 수 있습니다.");
            }
            
            sb.AppendLine($"\r\n이 작업은 절대 되돌릴 수 없습니다. 정말 {Utility.ConvertFileSize(totalSize)}개의 작업을 시작하시겠습니까?");
            if (await this.ShowMessageAsync("확실한가요?", sb.ToString(), MessageDialogStyle.AffirmativeAndNegative) != MessageDialogResult.Affirmative)
                return;

                var progressDialog = await this.ShowProgressAsync("Please wait", "파일들의 크기를 계산중입니다. 잠시만 기다려주세요.");
            progressDialog.SetIndeterminate();
            try
            {
                await RemoveDisk.StartRemoveDiskAsync(progressDialog);
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.Message);
            }
            finally
            {
                diskDataGrid.Items.Refresh();
                if (progressDialog != null)
                    await progressDialog.CloseAsync();
                await RefreshDiskList();
            }

            await this.ShowMessageAsync("완료", "디스크 소거 작업이 완료되었습니다.\r\n자세한 내용은 작업란의 상태를 확인해주세요.");
        }
        #endregion

        // ********************************************************
        // *                암호화 관련 코드                       *
        // ********************************************************
        #region crypto
        private void cryptoDataGrid_DragEnter(object sender, DragEventArgs e)
        {
            string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

            if (files != null)
                e.Effects = DragDropEffects.Copy;
        }

        private async void cryptoDataGrid_Drop(object sender, DragEventArgs e)
        {
            try
            {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

                if (files != null)
                {
                    foreach (string path in files)
                    {
                        if (File.Exists(path))              // 단일 파일이 존재하는 경우
                        {
                            AddCryptoItem(path);
                        }
                        else if (Directory.Exists(path))    // 폴더인 경우
                        {
                            // 모든 디렉토리 및 하위 디렉토리를 검색하여 각각 파일을 추가한다.
                            foreach (var file in Directory.GetFiles(path, "*.*", SearchOption.AllDirectories))
                                AddCryptoItem(file);
                        }
                        // 그 외의 경우는 파일이 없는 경우라 조건문이 필요 없음.
                    }
                }
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.ToString());
            }
            finally
            {
                cryptoDataGrid.ItemsSource ??= CryptoFile.GetInstance();
                cryptoDataGrid.Items.Refresh();
            }
        }

        private void cryptoAddFileButton_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog dialog = new System.Windows.Forms.OpenFileDialog();
            dialog.Title = "암복호화 파일 선택";
            dialog.Multiselect = true;
            dialog.Filter = "모든 파일 (*.*) | *.*";

            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                foreach (var file in dialog.FileNames)
                    AddCryptoItem(file);

                cryptoDataGrid.ItemsSource ??= CryptoFile.GetInstance();
                cryptoDataGrid.Items.Refresh();
            }
        }

        private void AddCryptoItem(string file)
        {
            FileInfo fi = new FileInfo(file);

            // 이미 있는 파일 혹은 존재하지 않는 파일은 추가하지 않는다.
            if (CryptoFile.GetInstance().Where(x => x.FileName == fi.FullName).Count() > 0 || !fi.Exists)
                return;

            bool IsEncrypt = false;
            bool IsRemoveAfter = false;

            // .fme 파일은 자동으로 복호화로 추가하도록 한다.
            if (fi.Extension == ".fme" || cryptoIsEncryptComboBox.SelectedIndex == 1)
                IsEncrypt = false;
            else if (cryptoIsEncryptComboBox.SelectedIndex == 0)
                IsEncrypt = true;

            RemoveAlgorithm Mode = RemoveAlgorithm.OnePass;
            // 암호화를 선택했고 파일 삭제 옵션을 선택했을 경우
            if (IsEncrypt && cryptoRemoveAfterSwitch.IsOn)
            {
                IsRemoveAfter = true;
                if (PhysicalDiskInfo.IsSolidStateDriveByLabel(file.ElementAt(0)))
                {
                    Mode = RemoveAlgorithm.NormalDelete;
                }
                else
                {
                    switch (cryptoRemoveComboBox.SelectedIndex)
                    {
                        case 0:
                            Mode = RemoveAlgorithm.NormalDelete;
                            break;
                        case 1:
                            Mode = RemoveAlgorithm.OnePass;
                            break;
                        case 2:
                            Mode = RemoveAlgorithm.ThreePass;
                            break;
                        case 3:
                            Mode = RemoveAlgorithm.SevenPass;
                            break;
                        case 4:
                            Mode = RemoveAlgorithm.ThirtyFivePass;
                            break;
                        default:
                            Mode = RemoveAlgorithm.OnePass;
                            break;
                    }
                }
            }
            CryptoFile.GetInstance().Add(new CryptoFile(file, fi.Length, IsEncrypt, IsRemoveAfter, Mode));
        }

        private void passwordBox_PasswordChanged(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(passwordBox.Password) && string.IsNullOrEmpty(confirmPasswordBox.Password))
            {
                passwordAlertLabel.Content = "";
            }
            else
            {
                if (string.Equals(passwordBox.Password, confirmPasswordBox.Password))
                {
                    passwordAlertLabel.Content = "비밀번호가 일치합니다.";
                }
                else
                {
                    passwordAlertLabel.Content = "비밀번호가 일치하지 않습니다.";
                }
            }
        }

        private void cryptoIsEncryptComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cryptoIsEncryptComboBox.SelectedIndex == 1)
            {
                // 복호화 모드 선택중에는 원본파일 삭제 및 옵션을 선택할 수 없도록 한다.
                cryptoRemoveAfterSwitch.IsEnabled = false;
                cryptoRemoveComboBox.IsEnabled = false;
            }
            else
            {
                // 암호화 모드에서만 원본파일 삭제 및 옵션을 선택할 수 있도록 한다.
                if (cryptoRemoveAfterSwitch != null)
                    cryptoRemoveAfterSwitch.IsEnabled = true;
                if (cryptoRemoveComboBox != null)
                    cryptoRemoveComboBox.IsEnabled = true;
            }
        }

        private void cryptoSettingsButton_Click(object sender, RoutedEventArgs e)
        {
            foreach (CryptoFile file in cryptoDataGrid.SelectedItems)
            {
                if (cryptoIsEncryptComboBox.SelectedIndex == 0)
                    file.IsEncrypt = true;
                else
                    file.IsEncrypt = false;

                file.IsRemoveAfter = file.IsEncrypt & cryptoRemoveAfterSwitch.IsOn;

                if (file.IsRemoveAfter)
                {
                    if (PhysicalDiskInfo.IsSolidStateDriveByLabel(file.FileName.ElementAt(0)))
                    {
                        file.Mode = RemoveAlgorithm.NormalDelete;
                    }
                    else
                    {
                        switch (cryptoRemoveComboBox.SelectedIndex)
                        {
                            case 0:
                                file.Mode = RemoveAlgorithm.NormalDelete;
                                break;
                            case 1:
                                file.Mode = RemoveAlgorithm.OnePass;
                                break;
                            case 2:
                                file.Mode = RemoveAlgorithm.ThreePass;
                                break;
                            case 3:
                                file.Mode = RemoveAlgorithm.SevenPass;
                                break;
                            case 4:
                                file.Mode = RemoveAlgorithm.ThirtyFivePass;
                                break;
                            default:
                                file.Mode = RemoveAlgorithm.OnePass;
                                break;
                        }
                    }
                }
            }

            cryptoDataGrid.ItemsSource ??= CryptoFile.GetInstance();
            cryptoDataGrid.Items.Refresh();
        }

        private void cryptoRemoveCompletedButton_Click(object sender, RoutedEventArgs e)
        {
            CryptoFile.GetInstance().RemoveAll(x => x.IsComplete);

            cryptoDataGrid.ItemsSource ??= CryptoFile.GetInstance();
            cryptoDataGrid.Items.Refresh();
        }

        private void cryptoRemoveButton_Click(object sender, RoutedEventArgs e)
        {
            foreach (CryptoFile file in cryptoDataGrid.SelectedItems)
                CryptoFile.GetInstance().Remove(file);

            cryptoDataGrid.ItemsSource ??= CryptoFile.GetInstance();
            cryptoDataGrid.Items.Refresh();
        }

        private async void cryptoStartButton_Click(object sender, RoutedEventArgs e)
        {
            // 완료되지 않은 작업이 0개인 경우
            if (CryptoFile.GetStandbyTasks() == 0)
            {
                await this.ShowMessageAsync("Error", "현재 대기중인 작업이 없습니다.");
                return;
            }

            // 비밀번호 확인란이 같지 않은 경우
            if (!string.Equals(passwordBox.Password, confirmPasswordBox.Password))
            {
                await this.ShowMessageAsync("Error", "비밀번호와 비밀번호 확인란이 일치하지 않습니다.");
                return;
            }

            // 비밀번호가 입력되지 않은 경우
            if (string.IsNullOrEmpty(passwordBox.Password))
            {
                await this.ShowMessageAsync("Error", "비밀번호를 입력해야합니다.");
                return;
            }

            StringBuilder sb = new StringBuilder();
            if (passwordBox.Password.Length < 10)
            {
                sb.AppendLine("안전하지 않은 비밀번호를 입력하셨습니다.");
                sb.AppendLine("숫자, 대소문자, 특수문자가 조합된 10자 이상의 강력한 비밀번호를 추천합니다.");
            }

            int removeCount = CryptoFile.GetInstance().Where(x => x.IsEncrypt && x.IsRemoveAfter && !x.IsComplete).Count();
            if (removeCount > 0)
            {
                sb.AppendLine($"암호화 후 {removeCount}개의 파일 삭제 작업이 자동으로 진행됩니다.");
                if (CryptoFile.GetStandbyTasks() > removeCount)
                    sb.AppendLine("현재 파일 소거 탭에 추가된 다른 소거 작업도 자동으로 진행됩니다.");
            }

            sb.AppendLine($"\r\n이 작업은 절대 되돌릴 수 없습니다. 정말 {CryptoFile.GetStandbyTasks()}개의 작업을 시작하시겠습니까?");
            // 사용자의 확인 후 작업을 진행함
            if (await this.ShowMessageAsync("확실한가요?", sb.ToString(), MessageDialogStyle.AffirmativeAndNegative) != MessageDialogResult.Affirmative)
                return;

            var progressDialog = await this.ShowProgressAsync("Please wait", "파일들의 크기를 계산중입니다. 잠시만 기다려주세요.");
            progressDialog.SetIndeterminate();

            // 암호화 작업 진행시 입력한 패스워드를 초기화한다.
            string key = passwordBox.Password;
            passwordBox.Clear();
            confirmPasswordBox.Clear();
            try
            {
                await CryptoFile.StartCryptoAsync(key, progressDialog);
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.Message);
            }
            finally
            {
                cryptoDataGrid.Items.Refresh();
                if (progressDialog != null)
                    await progressDialog.CloseAsync();
            }

            // 암호화 후 삭제할 파일이 있을 경우 파일 삭제 작업을 시작한다.
            if (CryptoFile.GetInstance().Where(x => x.IsRemoveAfter && x.IsEncrypt).Any())
            {
                removeDataGrid.ItemsSource ??= RemoveFile.GetInstance();
                removeDataGrid.Items.Refresh();
                mainTabControl.SelectedIndex = 0;

                progressDialog = await this.ShowProgressAsync("Please wait", "파일들의 크기를 계산중입니다. 잠시만 기다려주세요.");
                progressDialog.SetIndeterminate();
                try
                {
                    await RemoveFile.StartRemoveAsync(progressDialog);
                }
                catch (Exception ex)
                {
                    await this.ShowMessageAsync("Error", ex.Message);
                }
                finally
                {
                    removeDataGrid.Items.Refresh();
                    if (progressDialog != null)
                        await progressDialog.CloseAsync();
                }
            }

            mainTabControl.SelectedIndex = 2;
            await this.ShowMessageAsync("완료", "암복호화 작업이 완료되었습니다.\r\n자세한 내용은 작업란의 상태를 확인해주세요.");
        }

        #endregion
    }
}
