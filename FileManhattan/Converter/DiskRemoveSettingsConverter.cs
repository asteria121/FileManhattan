using System;
using System.Globalization;
using System.IO;
using System.Windows.Data;
using FileManhattan.Enums;

namespace FileManhattan.Converter
{
    public class DiskRemoveSettingsConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            DriveInfo driveInfo = (DriveInfo)values[0];
            DiskRemoveMethod diskRemoveMethod = (DiskRemoveMethod)values[1];
            bool wipeMFT = System.Convert.ToBoolean(values[2]);
            bool wipeClusterTip = System.Convert.ToBoolean(values[3]);

            string result;
            if (diskRemoveMethod == DiskRemoveMethod.FreeSpace)
            {
                result = "드라이브 빈공간 삭제";
                if (wipeMFT) result += " + MFT 빈공간 덮어쓰기";
                if (wipeClusterTip) result += " + 클러스터 팁 덮어쓰기";
            }
            else
            {
                result = "드라이브 전체 삭제";
            }

            result += $" ({driveInfo.Name})";

            return result;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
