using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.Storage.Pickers;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Media.Imaging;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace tesseract_sample_app_uwp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        async private void button_Click(object sender, RoutedEventArgs e)
        {
            FileOpenPicker openPicker = new FileOpenPicker();
            openPicker.ViewMode = PickerViewMode.Thumbnail;
            openPicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
            openPicker.FileTypeFilter.Add(".jpg");
            openPicker.FileTypeFilter.Add(".jpeg");
            openPicker.FileTypeFilter.Add(".png");
            openPicker.FileTypeFilter.Add(".bmp");

            StorageFile file = await openPicker.PickSingleFileAsync();
            if (file != null)
            {
                try
                {
                    BitmapImage bitmapImage = new BitmapImage();

                    IRandomAccessStream stream = await file.OpenReadAsync();
                    await bitmapImage.SetSourceAsync(stream);
                    image.Source = bitmapImage;

                    Tesseract.BaseApiWinRT tessBaseApi = new Tesseract.BaseApiWinRT();

                    StorageFolder tessdataFolderobj = await Windows.ApplicationModel.Package.Current.InstalledLocation.GetFolderAsync("tessdata");
                    await tessBaseApi.InitAsync(tessdataFolderobj.Path, "eng");

                    Rect winrtRect = new Rect(0, 0, 10000, 10000);
                    string result = await tessBaseApi.TesseractRectAsync(stream, winrtRect);

                    textBlock.Text = result;
                }
                catch (Exception ex)
                {
                    textBlock.Text = "ERROR!!! - " + ex.Message;
                }
            }
        }
    }
}
