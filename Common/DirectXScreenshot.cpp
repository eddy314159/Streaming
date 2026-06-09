#include "DirectXScreenshot.h"
#include <dxgi.h>

DirectXScreenShot::~DirectXScreenShot()
{
    stop();
}

bool DirectXScreenShot::start()
{
    if (started)
        return false;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &d3dDevice,
        nullptr,
        &d3dContext);

    if (FAILED(hr))
        return false;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = d3dDevice.As(&dxgiDevice);
    if (FAILED(hr))
        return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr))
        return false;

    Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    if (FAILED(hr))
        return false;

    Microsoft::WRL::ComPtr<IDXGIOutput1> dxgiOutput1;
    hr = dxgiOutput.As(&dxgiOutput1);
    if (FAILED(hr))
        return false;

    DXGI_OUTPUT_DESC outputDesc;
    hr = dxgiOutput->GetDesc(&outputDesc);
    if (FAILED(hr))
        return false;

    width = static_cast<unsigned int>(outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left);
    height = static_cast<unsigned int>(outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top);

    hr = dxgiOutput1->DuplicateOutput(d3dDevice.Get(), &dxgiOutputDuplication);
    if (FAILED(hr))
    {
        return false;
    }

    started = true;
    return true;
}

static bool EnsureStaging(Microsoft::WRL::ComPtr<ID3D11Device>& device,
                          Microsoft::WRL::ComPtr<ID3D11Texture2D>& staging,
                          const D3D11_TEXTURE2D_DESC& srcDesc)
{
    if (!staging)
    {
        D3D11_TEXTURE2D_DESC desc = srcDesc;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        HRESULT hr = device->CreateTexture2D(&desc, nullptr, &staging);
        return SUCCEEDED(hr);
    }
    D3D11_TEXTURE2D_DESC curDesc;
    staging->GetDesc(&curDesc);
    return curDesc.Width == srcDesc.Width && curDesc.Height == srcDesc.Height && curDesc.Format == srcDesc.Format;
}

bool DirectXScreenShot::screen(std::vector<char>& screenshot)
{
    if (!started || !dxgiOutputDuplication)
        return false;

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    Microsoft::WRL::ComPtr<IDXGIResource> desktopResource;

    HRESULT hr = dxgiOutputDuplication->AcquireNextFrame(10, &frameInfo, &desktopResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        return false;
    if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_DEVICE_REMOVED)
    {
        stop();
        return false;
    }
    if (FAILED(hr))
        return false;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> desktopTexture;
    hr = desktopResource.As(&desktopTexture);
    if (FAILED(hr))
    {
        dxgiOutputDuplication->ReleaseFrame();
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    desktopTexture->GetDesc(&desc);

    if (!EnsureStaging(d3dDevice, stagingTexture, desc))
    {
        stagingTexture.Reset();
        if (!EnsureStaging(d3dDevice, stagingTexture, desc))
        {
            dxgiOutputDuplication->ReleaseFrame();
            return false;
        }
    }

    d3dContext->CopyResource(stagingTexture.Get(), desktopTexture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = d3dContext->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr))
    {
        dxgiOutputDuplication->ReleaseFrame();
        return false;
    }

    const size_t requiredSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
    if (cachedBuffer.size() < requiredSize)
        cachedBuffer.resize(requiredSize);

    const unsigned char* srcRow = reinterpret_cast<const unsigned char*>(mapped.pData);
    unsigned char* dst = reinterpret_cast<unsigned char*>(cachedBuffer.data());
    const unsigned int rowPitch = static_cast<unsigned int>(mapped.RowPitch);

    for (unsigned int y = 0; y < height; ++y)
    {
        const unsigned char* src = srcRow;
        unsigned char* out = dst;
        for (unsigned int x = 0; x < width; ++x)
        {
            out[0] = src[0];
            out[1] = src[1];
            out[2] = src[2];
            src += 4;
            out += 3;
        }
        srcRow += rowPitch;
        dst += static_cast<size_t>(width) * 3;
    }

    d3dContext->Unmap(stagingTexture.Get(), 0);
    dxgiOutputDuplication->ReleaseFrame();

    screenshot.swap(cachedBuffer);
    cachedBuffer.clear();

    return true;
}

void DirectXScreenShot::stop()
{
    if (!started)
        return;

    started = false;
    width = 0;
    height = 0;

    dxgiOutputDuplication.Reset();
    d3dContext.Reset();
    d3dDevice.Reset();
    stagingTexture.Reset();
    cachedBuffer.clear();
}

unsigned int DirectXScreenShot::getHeight() const
{
    return height;
}

unsigned int DirectXScreenShot::getWidth() const
{
    return width;
}
