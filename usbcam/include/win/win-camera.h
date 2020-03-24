#pragma once

#ifdef _WIN32

#include "i-camera.h"

#include <vector>
#include <string>
#include <stdint.h>

#include <Windows.h>
#include <conio.h>
#include <Mferror.h>
#include <winrt\base.h>
#include <winrt\Windows.Foundation.h>
#include <winrt\Windows.Foundation.Collections.h>
#include <winrt\Windows.Media.Capture.h>
#include <winrt\Windows.Media.Core.h>
#include <winrt\Windows.Media.Devices.h>
#include <winrt\Windows.Media.MediaProperties.h>
#include <winrt\Windows.Media.Capture.Frames.h>
#include <winrt\Windows.Devices.Enumeration.h>
#include <winrt\Windows.Graphics.Imaging.h>
#include <winrt\Windows.Storage.h>
#include <winrt\Windows.Storage.Streams.h>

using namespace winrt;
using namespace Windows::Media::Capture;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture::Frames;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Storage;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Core;

namespace USBCam {

    class WinCamera : public ICamera {
    public:
        WinCamera(uint32_t portNumber);
        virtual ~WinCamera();

        // Getters and setters

        virtual std::vector<Capabilities> GetCapabilities() const override;
        virtual void SetFormat(const Capabilities& cap) override;
        virtual Capabilities GetFormat() override;
        
        virtual void SetSetting(CameraSetting setting, unsigned int value) override;
        virtual unsigned int GetSetting(CameraSetting setting) override;

        // Control methods

        virtual void TakeAndSavePicture() override;
        
    private:
        FrameEncoding SubTypeToFrameEncoding(const std::string& subType) const;

        void StartPreview();

    private:
        IVectorView<MediaFrameSourceGroup> m_sourceGroups; // The sourceinfos are holding weak references to this.
        MediaFrameSourceInfo m_sourceInfo;
        MediaCapture m_capture;
        MediaFrameReader m_reader;
        uint32_t m_portNumber;
    };
}

#endif // _WIN32
