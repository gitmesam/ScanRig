#include "linux/linux-camera.h"

#ifdef __linux__

#include "capture-manager.h"

#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <linux/uinput.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

namespace USBCam {

    std::vector<Port> GetDevicesList() {
        std::vector<Port> ports;

        for (unsigned int i = 0; i < 60; i++) {
            const auto path = std::string("/dev/video") + std::to_string(i);
            const int fd = open(path.c_str(), O_RDONLY);

            if (fd != -1) {
                Port port;
                port.number = i;
                
                v4l2_capability cap;
                if (ioctl(fd, VIDIOC_QUERYCAP, &cap) != -1) {
                    port.name = std::string(reinterpret_cast<char*>(cap.card));
                }

                ports.push_back(port);
            } else {
                break;
            }
        }

        return ports;
    }

    LinuxCamera::LinuxCamera(uint32_t portNumber) : m_fd(-1), m_id(portNumber), m_frameCount(0) {
        const auto path = std::string("/dev/video") + std::to_string(portNumber);
        m_fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
        if (m_fd == -1) {
            throw std::invalid_argument("Camera does not exist at this port : " + std::to_string(portNumber) + " : " + std::string(strerror(errno)));
        }

        // Set Default capture format
        auto format = GetFormat();
        format.encoding = ICamera::FrameEncoding::MJPG;
        SetFormat(format);

        // Start stream
        m_buffers = new MMapBuffers(m_fd, 1);
        StartStreaming();

        // Remove first frame as it is often corrupted
        Wait();
        m_buffers->Dequeue();
        m_buffers->Queue();

        // Create capture hierarchy
        mkdir("./capture", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        const std::string savePath = "./capture/cam" + std::to_string(m_id);
        mkdir(savePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    LinuxCamera::~LinuxCamera() {
        StopStreaming();
        delete m_buffers;
        close(m_fd);
    }

    std::vector<ICamera::Capabilities> LinuxCamera::GetCapabilities() const {
        std::vector<ICamera::Capabilities> capabilities;
        
        // Get supported frame encodings
        std::vector<unsigned int> encodings;
        v4l2_fmtdesc formatDesc;
        CLEAR(formatDesc);
        formatDesc.index = 0;
        formatDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while (ioctl(m_fd, VIDIOC_ENUM_FMT, &formatDesc) == 0) {
            switch (formatDesc.pixelformat) {
            case V4L2_PIX_FMT_MJPEG: encodings.push_back(V4L2_PIX_FMT_MJPEG); break;
            case V4L2_PIX_FMT_UYVY: encodings.push_back(V4L2_PIX_FMT_UYVY); break;
            default: break;
            }
            formatDesc.index++;
        }

        // Get supported frame sizes
        v4l2_frmsizeenum frameDesc;
        CLEAR(frameDesc);
        frameDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        
        for (const auto encoding : encodings) {
            frameDesc.index = 0;
            frameDesc.pixel_format = encoding;
            
            while (ioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &frameDesc) == 0) {
                ICamera::Capabilities cap;
                cap.id = frameDesc.index;
                cap.height = frameDesc.discrete.height;
                cap.width = frameDesc.discrete.width;
                cap.encoding = PixelFormatToFrameEncoding(frameDesc.pixel_format);
                cap.frameRate = 0;

                capabilities.push_back(cap);
                frameDesc.index++;
            }
        }

        // Get supported frame intervals
        v4l2_frmivalenum intervalDesc;
        CLEAR(intervalDesc);
        intervalDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        for (auto& cap : capabilities) {
            intervalDesc.width = cap.width;
            intervalDesc.height = cap.height;
            intervalDesc.pixel_format = FrameEncodingToPixelFormat(cap.encoding);

            if (ioctl(m_fd, VIDIOC_ENUM_FRAMEINTERVALS, &intervalDesc) == -1) {
                std::cerr << "Cannot get framerate : " << std::string(strerror(errno)) << std::endl;
                continue;
            }

            cap.frameRate = intervalDesc.stepwise.min.denominator;
        }

        return capabilities;
    }

    void LinuxCamera::SetFormat(const ICamera::Capabilities& cap) {
        v4l2_format format;
        CLEAR(format);
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.pixelformat = FrameEncodingToPixelFormat(cap.encoding);
        format.fmt.pix.width = cap.width;
        format.fmt.pix.height = cap.height;

        if (ioctl(m_fd, VIDIOC_S_FMT, &format) == -1) {
            throw std::runtime_error("Cannot set camera default capture format : " + std::string(strerror(errno)));
        }
    }

    ICamera::Capabilities LinuxCamera::GetFormat() {
        v4l2_format format;
        CLEAR(format);
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (ioctl(m_fd, VIDIOC_G_FMT, &format) == -1) {
            throw std::runtime_error("Cannot get camera capture format : " + std::string(strerror(errno)));
        }

        ICamera::Capabilities cap;
        cap.encoding = PixelFormatToFrameEncoding(format.fmt.pix.pixelformat);
        cap.width = format.fmt.pix.width;
        cap.height = format.fmt.pix.height;
        return cap;
    }

    void LinuxCamera::SetSetting(CameraSetting setting, unsigned int value) override {

    }

    unsigned int LinuxCamera::GetSetting(CameraSetting setting) override {
        return 0;
    }

    void LinuxCamera::TakeAndSavePicture() {
        Wait();
        m_buffers->Dequeue();

        const std::string filepath =  "./capture/cam" + std::to_string(m_id) + "/" + std::to_string(m_frameCount) + ".jpeg";
        int imgFile = open(filepath.c_str(), O_WRONLY | O_CREAT, 0660);
        if (imgFile == -1) {
            throw std::runtime_error("Cannot create image at : " + imgFile);
        }
        
        write(imgFile, m_buffers->GetStart(), m_buffers->GetLength());
        close(imgFile);

        m_buffers->Queue();
    }

    ////////////////////////////////////////////////////////////////////
    ////////////////////////// PRIVATE METHODS /////////////////////////
    ////////////////////////////////////////////////////////////////////

    void LinuxCamera::Wait() const {
        const auto poolFlags = POLLIN | POLLRDNORM | POLLERR;
        pollfd desc { m_fd, poolFlags, 0 };
        if (poll(&desc, 1, 5000) == -1) {
            throw std::runtime_error("Pool error : " + std::string(strerror(errno)));
        }
    }

    ICamera::FrameEncoding LinuxCamera::PixelFormatToFrameEncoding(unsigned int pixelFormat) const {
        switch (pixelFormat) {
            case V4L2_PIX_FMT_MJPEG: return FrameEncoding::MJPG;
            case V4L2_PIX_FMT_UYVY: return FrameEncoding::UYVY;
            default: return FrameEncoding::_UNKNOWN;
        }
    }

    unsigned int LinuxCamera::FrameEncodingToPixelFormat(FrameEncoding encoding) const {
        switch (encoding) {
            case FrameEncoding::MJPG: return V4L2_PIX_FMT_MJPEG;
            case FrameEncoding::UYVY: return V4L2_PIX_FMT_UYVY;
            default:
                std::cerr << "Unknown encoding !" << std::endl;
                return 0;
        }
    }

    CameraSetting LinuxCamera::ControlIdToCameraSetting(unsigned int controlId) const {
        switch (controlId) {
            case V4L2_CID_BRIGHTNESS: return CameraSetting::BRIGHTNESS;
            default:
                std::cerr << "Unknown control ID !" << std::endl;
                return 0;
        }
    }

    unsigned int LinuxCamera::CameraSettingToControlId(CameraSetting setting) const {
        switch (setting) {
            case CameraSetting::BRIGHTNESS: return V4L2_CID_BRIGHTNESS;
            default:
                std::cerr << "Unknown camera setting !" << std::endl;
                return 0;
        }
    }

    void LinuxCamera::StartStreaming() {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(m_fd, VIDIOC_STREAMON, &type) == -1) {
            throw std::runtime_error("Cannot start streaming : " + std::string(strerror(errno)));
        }
    }

    void LinuxCamera::StopStreaming() {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(m_fd, VIDIOC_STREAMOFF, &type) == -1) {
            throw std::runtime_error("Cannot stop streaming : " + std::string(strerror(errno)));
        }
    }
}

#endif // __linux__

