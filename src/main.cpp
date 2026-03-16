#include "../include/videoDriver.hpp"

int main() {
    videoDriver myVideoDriver("/dev/video0", 15, 3, 320, 240);

    myVideoDriver.setup();

    myVideoDriver.captureLoop();

    return 0;
}