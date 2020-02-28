#pragma once

#ifdef __linux__

#include "i-camera.h"

#include <vector>
#include <string>
#include <stdint.h>

namespace USBCam {
    class LinuxCamera : public ICamera {
    public:
        LinuxCamera(uint32_t portNumber);
        virtual ~LinuxCamera();

        virtual std::vector<ICamera::Capabilities> GetCapabilities() const override;
        virtual void SetFormat(uint32_t id) override;
        virtual void TakeAndSavePicture() const override;
    };

}

#endif // __linux__
