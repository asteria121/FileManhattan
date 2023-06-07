using System;
using System.Globalization;
using System.Windows.Data;
using FileManhattan.Enums;

namespace FileManhattan.Converter
{
    public class RemoveSettingsConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            RemoveAlgorithm mode = (RemoveAlgorithm)value;

            string result;
            switch (mode)
            {
                case RemoveAlgorithm.NormalDelete:
                    result = "일반 삭제 (SSD)";
                    break;
                case RemoveAlgorithm.ThreePass:
                    result = "3회 덮어쓰기 - DoD 5220.22-M (E)";
                    break;
                case RemoveAlgorithm.SevenPass:
                    result = "7회 덮어쓰기 - DoD 5220.22-M (ECE)";
                    break;
                case RemoveAlgorithm.ThirtyFivePass:
                    result = "35회 덮어쓰기 - 피터 거트만 알고리즘";
                    break;
                default:
                    result = "1회 덮어쓰기 - 랜덤 데이터";
                    break;
            }

            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
