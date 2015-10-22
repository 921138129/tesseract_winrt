/**********************************************************************
* File:        BaseApiWinRT.cpp
* Description: BaseApi class wrapper for interop with WinRT/WinPhone Apps
* Author:      Yoisel Melis Santana
* Created:     Oct 14 2015
*
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** http://www.apache.org/licenses/LICENSE-2.0
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*
**********************************************************************/

// Leptonica header
#include "allheaders.h"

// Tesseract headers
#include "baseapi.h"
#include "strngs.h"
#include "genericvector.h"

// ATL headers for Multibyte <=> Widestring conversions, ComPtr support and IStream support
#include "atlbase.h"
#include "atlconv.h"

// WIC header files
#include <wincodec.h>
#include <wincodecsdk.h>

// Needed for IRandomAccessStream to IStream interop
#include <ShCore.h>

#include <ppltasks.h>

#include "BaseApiWinRT.h"


namespace Tesseract {

	template<class T>
	HRESULT AssignToOutputPointer(T** pp, const CComPtr<T> &p)
	{
		assert(pp);
		*pp = p;
		if (nullptr != (*pp))
		{
			(*pp)->AddRef();
		}

		return S_OK;
	}

	HRESULT GetWICFactory(IWICImagingFactory** factory)
	{
		static CComAutoCriticalSection m_pWICFactoryLock;

		// keep this variable "private"
		static CComPtr<IWICImagingFactory> m_pWICFactory;

		if (!factory)
			return E_INVALIDARG;

		// We may have async methods calling this function, so we will protect it
		// with a critical section object
		m_pWICFactoryLock.Lock();
		HRESULT hr = S_OK;

		if (nullptr == m_pWICFactory)
		{
			hr = CoCreateInstance(
				CLSID_WICImagingFactory, nullptr,
				CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICFactory));
		}

		if (SUCCEEDED(hr))
		{
			hr = AssignToOutputPointer(factory, m_pWICFactory);
		}
		m_pWICFactoryLock.Unlock();
		return hr;
	}

	BaseApiWinRT::BaseApiWinRT()
	{
		baseAPI = new tesseract::TessBaseAPI();
	}

	BaseApiWinRT::~BaseApiWinRT()
	{
		delete baseAPI;
	}


	int BaseApiWinRT::Init(Platform::String^ datapath, Platform::String^ language, OcrEngineMode mode,
		const Platform::Array<Platform::String^> ^ configs,
		const Platform::Array<Platform::String^> ^ vars_vec,
		const Platform::Array<Platform::String^> ^ vars_values,
		bool set_only_non_debug_params)
	{
		USES_CONVERSION;
		InitData initData; // InitData destructor will take care of deallocating whatever we use here

		if (language == nullptr || datapath == nullptr)
			return -1; // nothing to do

		if (configs && configs->Length > 0)
		{
			initData._configs_size = configs->Length;
			initData._configs = new LPSTR[initData._configs_size];

			for (int i = 0; i < initData._configs_size; i++)
			{
				char * _configStr = W2A_CP(configs[i]->Data(), CP_UTF8);
				initData._configs[i] = new char[configs[i]->Length() + 1];
				strcpy_s(initData._configs[i], configs[i]->Length() + 1, _configStr);
			}

		}

		if (vars_vec && vars_vec->Length > 0)
		{
			initData._vars_vec = new GenericVector<STRING>();

			for (int i = 0; i < vars_vec->Length; i++)
			{
				char * auxStr = W2A_CP(vars_vec[i]->Data(), CP_UTF8);
				initData._vars_vec->push_back(STRING(auxStr));
			}
		}

		if (vars_values && vars_values->Length > 0)
		{
			initData._vars_values = new GenericVector<STRING>();

			for (int i = 0; i < vars_values->Length; i++)
			{
				char * auxStr = W2A_CP(vars_values[i]->Data(), CP_UTF8);
				initData._vars_values->push_back(STRING(auxStr));
			}
		}

		return baseAPI->Init(W2A_CP(datapath->Data(), CP_UTF8), W2A_CP(language->Data(), CP_UTF8), (tesseract::OcrEngineMode)mode,
			initData._configs, initData._configs_size, initData._vars_vec, initData._vars_values, set_only_non_debug_params);
	}

	Windows::Foundation::IAsyncOperation<Platform::String^>^ BaseApiWinRT::TesseractRectAsync(Windows::Storage::Streams::IRandomAccessStream ^ inputImage, Windows::Foundation::Rect rect)
	{
		return Concurrency::create_async([=]() -> Platform::String^
		{
			try
			{
				USES_CONVERSION;

				unsigned char * imageData;
				int bytes_per_pixel; int bytes_per_line;
				int left; int top; int width; int height;

				CComPtr<IStream> stream;
				CComPtr<IWICImagingFactory> m_pWICFactory;
				IfFailedThrowHR(GetWICFactory(&m_pWICFactory));

				IfFailedThrowHR(CreateStreamOverRandomAccessStream(inputImage, IID_PPV_ARGS(&stream)));

				CComPtr<IWICBitmapDecoder> m_pBitmapDecoder;
				IfFailedThrowHR(m_pWICFactory->CreateDecoderFromStream(stream, NULL, WICDecodeOptions::WICDecodeMetadataCacheOnLoad, &m_pBitmapDecoder));

				CComPtr<IWICBitmapFrameDecode> m_pFrameDecode;
				CComPtr<IWICFormatConverter> m_pConvertedFrame;
				CComPtr<IWICBitmap> m_pBitmap;
				CComPtr<IWICBitmapLock> m_pBitmapLock;

				IfFailedThrowHR(m_pBitmapDecoder->GetFrame(0, &m_pFrameDecode));

				IfFailedThrowHR(m_pWICFactory->CreateBitmapFromSource(
					m_pFrameDecode,          // Create a bitmap from the image frame
					WICBitmapCacheOnDemand,  // Cache metadata when needed
					&m_pBitmap));

				UINT imageWidth; UINT imageHeight;
				m_pBitmap->GetSize(&imageWidth, &imageHeight);

				WICRect rcLock = { 0, 0, imageWidth, imageHeight };

				// The bitmap will get unlocked once m_pBitmapLock gets out of scope
				IfFailedThrowHR( m_pBitmap->Lock(&rcLock, WICBitmapLockRead, &m_pBitmapLock) );

				WICInProcPointer pixelData;
				UINT bufferSize;
				m_pBitmapLock->GetDataPointer( &bufferSize, &pixelData);
				imageData = (unsigned char *)pixelData;

				UINT stride;
				m_pBitmapLock->GetStride(&stride);
				bytes_per_line = (int)stride;

				WICPixelFormatGUID bitmapFormat;
				m_pBitmapLock->GetPixelFormat(&bitmapFormat);

				if (bitmapFormat == GUID_WICPixelFormat1bppIndexed)
					bytes_per_pixel = 0;
				else if (bitmapFormat == GUID_WICPixelFormat16bppGray)
					bytes_per_pixel = 1;
				else if (bitmapFormat == GUID_WICPixelFormat24bppBGR)
					bytes_per_pixel = 3;
				else if (bitmapFormat == GUID_WICPixelFormat32bppBGRA)
					bytes_per_pixel = 4;
				else
				{
					throw ref new Platform::Exception(E_FAIL, "Pixel format not supported, supported formats are: mono, 8-bit grayscale, 24-bit RGB and 32-bit ARGB");
				}

				auto resultStr = baseAPI->TesseractRect(imageData, bytes_per_pixel, bytes_per_line, 0, 0, rcLock.Width, rcLock.Height); //(int)max(0, rect.Left), max(0,rect.Top), min(rect.Width, rcLock.Width), min(rect.Height, rcLock.Height) );
				Platform::String^ result = nullptr;
				if (resultStr)
				{
					result = resultStr ? ref new Platform::String(A2W_CP(resultStr, CP_UTF8)) : nullptr;
					delete resultStr;
				}

				return result;				
			}
			catch(HRESULT hr)
			{
				throw ref new Platform::Exception(hr, "TesseractRectAsync failed reading the input image");
			}
		});

	}

#if 0

	// To be used maybe with WritableBitmap
	byte* GetPointerToPixelData(IBuffer^ buffer)
	{
		// Cast to Object^, then to its underlying IInspectable interface.
		Object^ obj = buffer;
		ComPtr<IInspectable> insp(reinterpret_cast<IInspectable*>(obj));

		// Query the IBufferByteAccess interface.
		ComPtr<IBufferByteAccess> bufferByteAccess;
		ThrowIfFailed(insp.As(&bufferByteAccess));

		// Retrieve the buffer data.
		byte* pixels = nullptr;
		ThrowIfFailed(bufferByteAccess->Buffer(&pixels));
		return pixels;
	}
#endif
}