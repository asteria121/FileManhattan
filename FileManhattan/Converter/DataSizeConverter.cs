using System;
using System.Globalization;
using System.Windows.Data;

namespace FileManhattan.Converter
{
    public class DataSizeConverter : IValueConverter
    {
        private readonly string[] sizeSuffixes = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            double size = System.Convert.ToDouble(value);
            int suffixIndex = 0;

            while (size >= 1024 && suffixIndex < sizeSuffixes.Length - 1)
            {
                size /= 1024;
                suffixIndex++;
            }

            return string.Format("{0:0.#} {1}", size, sizeSuffixes[suffixIndex]);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}