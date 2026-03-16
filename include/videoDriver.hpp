#pragma once

#include <cstdio>

#ifdef DEBUG
#define LOG_ERR(fmt, ...) fprintf(stderr, "[ERROR] [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_ERR(fmt, ...)
#define LOG_INFO(fmt, ...)
#endif

#include <iostream>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <ios>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <span>
#include <vector>

static constexpr int NBUF = 3;

class videoDriver {

private:
    int fd;
    uint8_t fps;
    uint8_t duration;
    uint16_t height;
    uint16_t width;

    std::vector<unsigned char *> buffer{NBUF};
    int size;
    int index;
    int nbufs;

    std::fstream outputFile;

    int setFPS();
    int queryCapabilities();
    int setFormat();
    int requestBuffers(int count);
    int queryBuffer(int index, unsigned char **buffer);
    // int queueBuffer(int index);
    int queueBuffer(int index);

    int startStreaming();
    // int dequeueBuffer();
    std::span<uint8_t> dequeueBuffer();
    int stopStreaming();
    int bufferWrapper();

public:
    videoDriver(const char *fd, uint8_t fps, uint8_t duration, uint16_t width, uint16_t height);
    ~videoDriver();

    videoDriver(const videoDriver &) = default;
    videoDriver(videoDriver &&) = default;
    videoDriver &operator=(const videoDriver &) = default;
    videoDriver &operator=(videoDriver &&) = default;
    // virtual ~videoDriver() = default;

    int setup();
    int captureLoop();
    void cleanup();
};