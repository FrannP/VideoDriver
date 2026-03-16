#include "videoDriver.hpp"

int videoDriver::setFPS() {
    // struct v4l2_streamparm stream;
    // memset(&stream, 0, sizeof(stream));
    v4l2_streamparm stream = {};
    stream.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stream.parm.capture.timeperframe.numerator = 1;
    stream.parm.capture.timeperframe.denominator = fps;

    if (ioctl(fd, VIDIOC_S_PARM, &stream) == -1) {
        LOG_ERR("Could not set FPS: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int videoDriver::queryCapabilities() {
    struct v4l2_capability cap;

    if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        LOG_ERR("Query capabilites Error: %s", strerror(errno));
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOG_ERR("Device is no video capture device");
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
        fprintf(stderr, "Device does not support read i/o\n");
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOG_ERR("Device does not support streaming i/o");
        return -1;
    }
    return 0;
}

int videoDriver::setFormat() {
    // struct v4l2_format format = {0};
    v4l2_format format{};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        LOG_ERR("Could not set format Error: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int videoDriver::requestBuffers(int count) {
    // struct v4l2_requestbuffers req = {0};
    v4l2_requestbuffers req = {};
    req.count = count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        LOG_ERR("Requesting Buffer: %s", strerror(errno));
        return -1;
    }

    return req.count;
}

int videoDriver::queryBuffer(int index, unsigned char **buffer) {
    // struct v4l2_buffer buf = {0};
    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        LOG_ERR("Could not query buffer: %s", strerror(errno));
        return -1;
    }

    *buffer = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    return buf.length;
}

int videoDriver::queueBuffer(int index) {
    // struct v4l2_buffer bufd = {0};
    v4l2_buffer bufd = {};
    bufd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufd.memory = V4L2_MEMORY_MMAP;
    bufd.index = index;

    if (ioctl(fd, VIDIOC_QBUF, &bufd) == -1) {
        LOG_ERR("Queue Buffer: %s", strerror(errno));
        return -1;
    }

    // return bufd.bytesused;
    return 0;
}

int videoDriver::startStreaming() {
    unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        LOG_ERR("VIDIOC_STREAMON: %s", strerror(errno));
        return -1;
    }

    return 0;
}

std::span<uint8_t> videoDriver::dequeueBuffer() {
    // struct v4l2_buffer bufd = {0};
    v4l2_buffer bufd = {};
    bufd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufd.memory = V4L2_MEMORY_MMAP;
    bufd.index = 0;

    if (ioctl(fd, VIDIOC_DQBUF, &bufd) == -1) {
        LOG_ERR("DeQueue Buffer: %s", strerror(errno));
        return std::span<uint8_t>();
    }

    // return bufd.index;
    index = bufd.index;
    return std::span<uint8_t>(buffer[bufd.index], bufd.bytesused);
}

int videoDriver::stopStreaming() {
    unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        LOG_ERR("VIDIOC_STREAMON: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int videoDriver::bufferWrapper() {
    int nbufs = requestBuffers(NBUF);
    if (nbufs == -1)
        return -1;
    if (nbufs > NBUF) {
        LOG_ERR("Increase NBUF to at least %i", nbufs);
        return -1;
    }

    for (int i = 0; i < NBUF; i++) {
        size = queryBuffer(i, &buffer[i]);
        if (size == -1)
            return -1;
        if (queueBuffer(i) == -1)
            return -1;
    }
    return 0;
}

videoDriver::videoDriver(const char *fdPath, uint8_t fps, uint8_t duration, uint16_t width, uint16_t height) {
    this->fd = open(fdPath, O_RDWR);
    this->outputFile.open("output.raw", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);

    this->fps = fps;
    this->duration = duration;
    this->width = width;
    this->height = height;
}

int videoDriver::setup() {
    if (queryCapabilities() == -1)
        return -1;
    if (setFormat() == -1)
        return -1;
    if (setFPS() == -1)
        return -1;
    if (bufferWrapper() == -1)
        return -1;
    if (startStreaming() == -1)
        return -1;
    return 0;
}

int videoDriver::captureLoop() {
    for (int i = 0; i < (fps * duration); i++) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        // struct timeval tv = {0};
        timeval tv = {};
        tv.tv_sec = 2;
        int r = select(fd + 1, &fds, NULL, NULL, &tv);
        if (-1 == r) {
            LOG_ERR("Waiting for Frame: %s", strerror(errno));
            return -1;
        }

        // pass buffer view to the buffer
        std::span<uint8_t> passBuffer = dequeueBuffer();
        if (passBuffer.empty())
            return -1;
        // for (auto &x : passBuffer) {
        //     std::cout << x << std::endl;
        // }
        outputFile.write(reinterpret_cast<const char *>(passBuffer.data()), passBuffer.size_bytes());
        if (queueBuffer(index) == -1)
            return -1;
    }
    return 0;
}

void videoDriver::cleanup() {
    stopStreaming();
    for (int i = 0; i < NBUF; i++) {
        if (buffer[i] != nullptr)
            munmap(buffer[i], size);
    }
    if (fd != -1)
        close(fd);
}

videoDriver::~videoDriver() { cleanup(); }