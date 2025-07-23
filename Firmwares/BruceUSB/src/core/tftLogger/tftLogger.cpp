#include <tftLogger.h>
#include <base64.h>
/*
AUXILIARY FUNCTIONS TO CREATE THE JSONS
*/

/* TFT LOGGER FUNCTIONS */

tft_logger::tft_logger(int16_t w, int16_t h) : BRUCE_TFT_DRIVER(w, h) {
    framebuffer = new uint16_t[w * h];                   // allocate framebuffer memory
    memset(framebuffer, 0, w * h * sizeof(uint16_t));    // initialize framebuffer to zero
}

tft_logger::~tft_logger() {
    clearLog();
    if (framebuffer) {
        delete[] framebuffer;    // free allocated framebuffer memory
        framebuffer = nullptr;
    }
}
void tft_logger::clearLog() {
    memset(log, 0, sizeof(log));
    memset(images, 0, sizeof(images));
    logWriteIndex = 0;
}

void tft_logger::addLogEntry(const uint8_t *buffer, uint8_t size) {
    memcpy(log[logWriteIndex].data, buffer, size);
    logWriteIndex = (logWriteIndex + 1) % MAX_LOG_ENTRIES;
}

void tft_logger::logWriteHeader(uint8_t *buffer, uint8_t &pos, tftFuncs fn) {
    buffer[pos++] = LOG_PACKET_HEADER;
    buffer[pos++] = 0; // placeholder size
    buffer[pos++] = fn;
}

void tft_logger::writeUint16(uint8_t *buffer, uint8_t &pos, uint16_t value) {
    buffer[pos++] = (value >> 8) & 0xFF;
    buffer[pos++] = value & 0xFF;
}

void tft_logger::setLogging(bool _log) {
    logging = _logging = _log;
    logWriteIndex = 0;
    memset(log, 0, sizeof(log));
};

void tft_logger::getBinLog(uint8_t *outBuffer, size_t &outSize) {
    outSize = 0;
    // add Screen Info at the beginning of the Bin packet
    uint8_t buffer[16];
    uint8_t pos = 0;
    logWriteHeader(buffer, pos, SCREEN_INFO);
    writeUint16(buffer, pos, width());
    writeUint16(buffer, pos, height());
    buffer[pos++] = rotation;
    buffer[1] = pos;

    memcpy(outBuffer + outSize, buffer, pos);
    outSize += pos;

    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        if (log[i].data[0] != LOG_PACKET_HEADER) continue;
        uint8_t *entry = log[i].data;
        uint8_t fn = entry[2];

        if (fn == DRAWIMAGE) {
            uint8_t imageSlot = entry[12]; // AA SS FN XX XX YY YY Ce Ce Ms Ms FS SLOT
                                           // 0  1  2  3  4  5  6  7  8  9  10 11 12
            const char *imgPath = images[imageSlot];
            size_t baseLen = 12; // AA SS FN XX XX YY YY Ce Ce Ms Ms FS + PATH
            size_t imgLen = strlen(imgPath);
            if (outSize + baseLen + imgLen > MAX_LOG_SIZE * MAX_LOG_ENTRIES) continue;

            memcpy(outBuffer + outSize, entry, baseLen);
            outSize += baseLen;
            memcpy(outBuffer + outSize, imgPath, imgLen);
            outSize += imgLen;

            outBuffer[outSize - imgLen - baseLen + 1] = baseLen + imgLen; // update packet size
        } else {
            uint8_t size = entry[1];
            if (outSize + size > MAX_LOG_SIZE * MAX_LOG_ENTRIES) continue;
            memcpy(outBuffer + outSize, entry, size);
            outSize += size;
        }
    }
}

void tft_logger::restoreLogger() {
    if (_logging) logging = true;
}

bool tft_logger::isLogEqual(const tftLog &a, const tftLog &b) {
    uint8_t sizeA = a.data[1];
    uint8_t sizeB = b.data[1];
    if (sizeA != sizeB) return false;
    return memcmp(a.data, b.data, sizeA) == 0;
}

void tft_logger::pushLogIfUnique(const tftLog &l) {
    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        if (isLogEqual(log[i], l)) {
            return; // Entry already exists
        }
    }
    memcpy(log[logWriteIndex].data, l.data, l.data[1]);
    logWriteIndex = (logWriteIndex + 1) % MAX_LOG_ENTRIES;
}

bool tft_logger::removeLogEntriesInsideRect(int rx, int ry, int rw, int rh) {
    bool r = false;
    int rx1 = rx;
    int ry1 = ry;
    int rx2 = rx + rw;
    int ry2 = ry + rh;

    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        uint8_t *data = log[i].data;
        if (data[0] != LOG_PACKET_HEADER) continue;
        int px = (data[3] << 8) | data[4];
        int py = (data[5] << 8) | data[6];
        if (px >= rx1 && px < rx2 && py >= ry1 && py < ry2) {
            data[0] = 0; // Mark as deleted
            r = true;
        }
    }
    return r;
}

void tft_logger::removeOverlappedImages(int x, int y, int center, int ms) {
    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        uint8_t *data = log[i].data;
        if (data[0] != LOG_PACKET_HEADER) continue;
        uint8_t fn = data[2];
        if (fn != DRAWIMAGE) continue;
        int px = (data[3] << 8) | data[4];
        int py = (data[5] << 8) | data[6];
        int pcenter = (data[7] << 8) | data[8];
        int pms = (data[9] << 8) | data[10];
        if (px == x && py == y && pcenter == center && pms == ms) {
            data[0] = 0; // Mark as deleted
        }
    }
}

void tft_logger::checkAndLog(tftFuncs f, std::initializer_list<int32_t> values) {
    if (!logging) return;

    uint8_t buffer[MAX_LOG_SIZE];
    uint8_t pos = 0;
    logWriteHeader(buffer, pos, f);

    for (auto val : values) { writeUint16(buffer, pos, val); }
    buffer[1] = pos;

    tftLog l;
    memcpy(l.data, buffer, pos);
    pushLogIfUnique(l);
    logging = false;
}

void tft_logger::fillScreen(int32_t color) {
    if (logging) clearLog();
    Serial.printf("[LCD] fillScreen: %d\n", color);
    BRUCE_TFT_DRIVER::fillScreen(color);
    sendBitmapBlock(0, 0, width(), height());

    restoreLogger();
}

void tft_logger::imageToBin(uint8_t fs, String file, int x, int y, bool center, int Ms) {
    if (!logging) return;

    removeOverlappedImages(x, y, center, Ms);

    // Try to find or store in images[MAX_LOG_IMAGES][MAX_LOG_IMG_PATH];
    uint8_t imageSlot = 0xFF;
    for (int i = 0; i < MAX_LOG_IMG_PATH; ++i) {
        if (strcmp(images[i], file.c_str()) == 0) {
            imageSlot = i;
            break;
        }
    }
    if (imageSlot == 0xFF) {
        for (int i = 0; i < MAX_LOG_IMG_PATH; ++i) {
            if (images[i][0] == 0) {
                strncpy(images[i], file.c_str(), sizeof(images[i]) - 1);
                images[i][sizeof(images[i]) - 1] = 0;
                imageSlot = i;
                break;
            }
        }
    }

    // Use image path as identifier in log.data
    uint8_t buffer[MAX_LOG_SIZE];
    uint8_t pos = 0;
    logWriteHeader(buffer, pos, DRAWIMAGE);

    writeUint16(buffer, pos, x);
    writeUint16(buffer, pos, y);
    writeUint16(buffer, pos, center);
    writeUint16(buffer, pos, Ms);
    buffer[pos++] = fs; // 0=SD and 2=LittleFS
    buffer[pos++] = imageSlot;

    // Store the file path string in the remainder of the buffer
    size_t fileLen = strlen(images[imageSlot]);
    size_t maxLen = MAX_LOG_SIZE - pos;
    if (fileLen > maxLen) fileLen = maxLen;
    memcpy(buffer + pos, images[imageSlot], fileLen);
    pos += fileLen;

    buffer[1] = pos; // update size

    tftLog l;
    memcpy(l.data, buffer, pos);
    pushLogIfUnique(l);

    restoreLogger();
}

 // Optional serial debug print
    //Serial.printf("[LCD] drawImage: %s at (%d, %d)\n", images[imageSlot], x, y);

    // 👉 Attempt to mirror the screen image block (fallback size for now)
    // NOTE: You must estimate or hardcode width/height if your image format doesn't provide it.
    //sendBitmapBlock(x, y, 64, 64); // <-- Adjust width/height to actual image size if known
    //sendBitmapBlock(0, 0, width(), height());

void tft_logger::drawLine(int32_t x, int32_t y, int32_t x1, int32_t y1, int32_t color) {
    if (logging) checkAndLog(DRAWLINE, {x, y, x1, y1, color});
    Serial.printf("[LCD] drawLine %d %d %d %d %d\n", x, y, x1, y1, color);
    BRUCE_TFT_DRIVER::drawLine(x, y, x1, y1, color);
    restoreLogger();
}

void tft_logger::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
    if (logging) checkAndLog(DRAWRECT, {x, y, w, h, color});
    Serial.printf("[LCD] drawRect %d %d %d %d %d\n", x, y, w, h, color);
    BRUCE_TFT_DRIVER::drawRect(x, y, w, h, color);
    restoreLogger();
}

void tft_logger::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
    if (logging) {
        if (w > 4 && h > 4) removeLogEntriesInsideRect(x, y, w, h);
        checkAndLog(FILLRECT, {x, y, w, h, color});
    }
    Serial.printf("[LCD] fillRect %d %d %d %d %d\n", x, y, w, h, color);
    BRUCE_TFT_DRIVER::fillRect(x, y, w, h, color);
    restoreLogger();
}

void tft_logger::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t color) {
    if (logging) checkAndLog(DRAWROUNDRECT, {x, y, w, h, r, color});
    Serial.printf("[LCD] drawRoundRect %d %d %d %d %d %d\n", x, y, w, h, r, color);
    BRUCE_TFT_DRIVER::drawRoundRect(x, y, w, h, r, color);
    restoreLogger();
}

void tft_logger::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t color) {
    if (logging) {
        removeLogEntriesInsideRect(x, y, w, h);
        checkAndLog(FILLROUNDRECT, {x, y, w, h, r, color});
    }
    Serial.printf("[LCD] fillRoundRect %d %d %d %d %d %d\n", x, y, w, h, r, color);
    BRUCE_TFT_DRIVER::fillRoundRect(x, y, w, h, r, color);
    restoreLogger();
}

void tft_logger::drawCircle(int32_t x, int32_t y, int32_t r, int32_t color) {
    if (logging) checkAndLog(DRAWCIRCLE, {x, y, r, color});
    Serial.printf("[LCD] drawCircle %d %d %d %d\n", x, y, r, color);
    BRUCE_TFT_DRIVER::drawCircle(x, y, r, color);
    restoreLogger();
}

void tft_logger::fillCircle(int32_t x, int32_t y, int32_t r, int32_t color) {
    if (logging) checkAndLog(FILLCIRCLE, {x, y, r, color});
    Serial.printf("[LCD] fillCircle %d %d %d %d\n", x, y, r, color);
    BRUCE_TFT_DRIVER::fillCircle(x, y, r, color);
    restoreLogger();
}

void tft_logger::drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {
    if (logging) checkAndLog(DRAWELIPSE, {x, y, rx, ry, color});
    Serial.printf("[LCD] drawEllipse %d %d %d %d %d\n", x, y, rx, ry, color);
    BRUCE_TFT_DRIVER::drawEllipse(x, y, rx, ry, color);
    restoreLogger();
}

void tft_logger::fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {
    if (logging) checkAndLog(FILLELIPSE, {x, y, rx, ry, color});
    Serial.printf("[LCD] fillEllipse %d %d %d %d %d\n", x, y, rx, ry, color);
    BRUCE_TFT_DRIVER::fillEllipse(x, y, rx, ry, color);
    restoreLogger();
}

void tft_logger::drawTriangle(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t color
) {
    if (logging) checkAndLog(DRAWTRIAGLE, {x1, y1, x2, y2, x3, y3, color});
    Serial.printf("[LCD] drawTriangle %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3, y3, color);
    BRUCE_TFT_DRIVER::drawTriangle(x1, y1, x2, y2, x3, y3, color);
    restoreLogger();
}

void tft_logger::fillTriangle(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t color
) {
    if (logging) checkAndLog(FILLTRIANGLE, {x1, y1, x2, y2, x3, y3, color});
    Serial.printf("[LCD] fillTriangle %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3, y3, color);
    BRUCE_TFT_DRIVER::fillTriangle(x1, y1, x2, y2, x3, y3, color);
    restoreLogger();
}
void tft_logger::drawArc(
    int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color,
    uint32_t bg_color, bool smoothArc
) {
    if (logging)
        checkAndLog(
            DRAWARC,
            {x, y, r, ir, (int32_t)startAngle, (int32_t)endAngle, (int32_t)fg_color, (int32_t)bg_color}
        );
    Serial.printf("[LCD] drawArc %d %d %d %d %u %u %u %u %d\n", x, y, r, ir, startAngle, endAngle, fg_color, bg_color, smoothArc);
    BRUCE_TFT_DRIVER::drawArc(x, y, r, ir, startAngle, endAngle, fg_color, bg_color, smoothArc);
    restoreLogger();
}

void tft_logger::drawWideLine(float ax, float ay, float bx, float by, float wd, int32_t fg, int32_t bg) {
    if (logging)
        checkAndLog(
            DRAWWIDELINE, {(uint16_t)ax, (uint16_t)ay, (uint16_t)bx, (uint16_t)by, (uint16_t)wd, fg, bg}
        );
    Serial.printf("[LCD] drawWideLine %.2f %.2f %.2f %.2f %.2f %d %d\n", ax, ay, bx, by, wd, fg, bg);
    BRUCE_TFT_DRIVER::drawWideLine(ax, ay, bx, by, wd, fg, bg);
    restoreLogger();
}

void tft_logger::drawFastVLine(int32_t x, int32_t y, int32_t h, int32_t fg) {
    if (logging) checkAndLog(DRAWFASTVLINE, {x, y, h, fg});
    Serial.printf("[LCD] drawFastVLine %d %d %d %d\n", x, y, h, fg);
    BRUCE_TFT_DRIVER::drawFastVLine(x, y, h, fg);
    restoreLogger();
}

void tft_logger::drawFastHLine(int32_t x, int32_t y, int32_t w, int32_t fg) {
    if (logging) checkAndLog(DRAWFASTHLINE, {x, y, w, fg});
    Serial.printf("[LCD] drawFastHLine %d %d %d %d\n", x, y, w, fg);
    BRUCE_TFT_DRIVER::drawFastHLine(x, y, w, fg);
    restoreLogger();
}

void tft_logger::log_drawString(String s, tftFuncs fn, int32_t x, int32_t y) {
    if (!logging) return;
    if (removeLogEntriesInsideRect(x, y, s.length() * LW * textsize, s.length() * LH * textsize)) {
    Serial.printf("[LCD] Removed overlapping log entries before: %s\n", s.c_str());
        // debug purpose
        // Serial.printf("Something was removed while processing: %s\n", s.c_str());
    }
    Serial.printf("[LCD] log_drawString(%d, %d): %s\n", x, y, s.c_str());  // <-- ADD THIS

    uint8_t buffer[MAX_LOG_SIZE];
    uint8_t pos = 0;
    logWriteHeader(buffer, pos, fn);

    writeUint16(buffer, pos, x);
    writeUint16(buffer, pos, y);
    writeUint16(buffer, pos, textsize);
    writeUint16(buffer, pos, textcolor);
    writeUint16(buffer, pos, textbgcolor);

    size_t maxLen = MAX_LOG_SIZE - pos - 1;
    size_t len = s.length();
    if (len > maxLen) len = maxLen;
    memcpy(buffer + pos, s.c_str(), len);
    pos += len;

    buffer[1] = pos;

    tftLog l;
    memcpy(l.data, buffer, pos);
    pushLogIfUnique(l);
    logging = false;
}

int16_t tft_logger::drawString(const String &string, int32_t x, int32_t y, uint8_t font) {
    log_drawString(string, DRAWSTRING, x, y);
    Serial.printf("[LCD] drawString(%d, %d): %s\n", x, y, string.c_str());
    int16_t r = BRUCE_TFT_DRIVER::drawString(string, x, y, font);
    restoreLogger();
    return r;
}

int16_t tft_logger::drawCentreString(const String &string, int32_t x, int32_t y, uint8_t font) {
    log_drawString(string, DRAWCENTRESTRING, x, y);
    Serial.printf("[LCD] drawString(%d, %d): %s\n", x, y, string.c_str());
    int16_t r = BRUCE_TFT_DRIVER::drawCentreString(string, x, y, font);
    restoreLogger();
    return r;
}

int16_t tft_logger::drawRightString(const String &string, int32_t x, int32_t y, uint8_t font) {
    log_drawString(string, DRAWRIGHTSTRING, x, y);
    Serial.printf("[LCD] drawString(%d, %d): %s\n", x, y, string.c_str());
    int16_t r = BRUCE_TFT_DRIVER::drawRightString(string, x, y, font);
    restoreLogger();
    return r;
}

void tft_logger::log_print(String s) {
    if (!logging) return;
    Serial.printf("[LCD] log_print(%d, %d): %s\n", cursor_x, cursor_y, s.c_str());

    removeLogEntriesInsideRect(
        cursor_x - 1, cursor_y - 1, s.length() * LW * textsize + 2, s.length() * LH * textsize + 2
    );

    uint8_t buffer[MAX_LOG_SIZE];
    uint8_t pos = 0;
    logWriteHeader(buffer, pos, PRINT);

    writeUint16(buffer, pos, cursor_x);
    writeUint16(buffer, pos, cursor_y);
    writeUint16(buffer, pos, textsize);
    writeUint16(buffer, pos, textcolor);
    writeUint16(buffer, pos, textbgcolor);

    size_t maxLen = MAX_LOG_SIZE - pos - 1;
    size_t len = s.length();
    if (len > maxLen) len = maxLen;
    memcpy(buffer + pos, s.c_str(), len);
    pos += len;

    buffer[1] = pos;

    tftLog l;
    memcpy(l.data, buffer, pos);
    pushLogIfUnique(l);
}

size_t tft_logger::print(const String &s) {
    size_t totalPrinted = 0;
    int remaining = s.length();
    int offset = 0;

    const int maxChunkSize = MAX_LOG_SIZE - 13; // 13 bytes reserved to header + metadata

    while (remaining > 0) {
        int chunkSize = (remaining > maxChunkSize) ? maxChunkSize : remaining;
        String chunk = s.substring(offset, offset + chunkSize);

        log_print(chunk);
        Serial.printf("[LCD] print(%d, %d): %s\n", cursor_x, cursor_y, chunk.c_str());
        totalPrinted += BRUCE_TFT_DRIVER::print(chunk);

        offset += chunkSize;
        remaining -= chunkSize;
    }

    return totalPrinted;
}

void tft_logger::sendBitmapBlock(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (w <= 0 || h <= 0) return;

    // Make sure framebuffer is allocated and large enough
    if (!framebuffer || (w * h) > (width() * height())) {
        Serial.println("[LCD] sendBitmapBlock error: framebuffer not allocated or size too small.");
        return;
    }

    // Read pixel data into pre-allocated framebuffer buffer
    BRUCE_TFT_DRIVER::readRect(x, y, w, h, framebuffer);

    // Base64 encode raw RGB565 pixel data
    String encoded = base64::encode((uint8_t *)framebuffer, w * h * 2);  // 2 bytes per pixel

    // Send over serial
    Serial.printf("[LCD] BITMAPDATA %d %d %d %d %s\n", x, y, w, h, encoded.c_str());
}

size_t tft_logger::println(void) { return print("\n"); }

size_t tft_logger::println(const String &s) { return print(s + "\n"); }

size_t tft_logger::println(char c) { return print(String(c) + "\n"); }

size_t tft_logger::println(unsigned char b, int base) { return print(String(b, base) + "\n"); }

size_t tft_logger::println(int n, int base) { return print(String(n, base) + "\n"); }

size_t tft_logger::println(unsigned int n, int base) { return print(String(n, base) + "\n"); }

size_t tft_logger::println(long n, int base) { return print(String(n, base) + "\n"); }

size_t tft_logger::println(unsigned long n, int base) { return print(String(n, base) + "\n"); }

size_t tft_logger::println(long long n, int base) { return print(String(n, base) + "\n"); }

size_t tft_logger::println(unsigned long long n, int base) { return print(String(n, base) + "\n"); }

size_t tft_logger::println(double n, int digits) { return print(String(n, digits) + "\n"); }

size_t tft_logger::print(char c) { return print(String(c)); }

size_t tft_logger::print(unsigned char b, int base) { return print(String(b, base)); }

size_t tft_logger::print(int n, int base) { return print(String(n, base)); }

size_t tft_logger::print(unsigned int n, int base) { return print(String(n, base)); }

size_t tft_logger::print(long n, int base) { return print(String(n, base)); }

size_t tft_logger::print(unsigned long n, int base) { return print(String(n, base)); }

size_t tft_logger::print(long long n, int base) { return print(String(n, base)); }

size_t tft_logger::print(unsigned long long n, int base) { return print(String(n, base)); }

size_t tft_logger::print(double n, int digits) { return print(String(n, digits)); }

size_t tft_logger::printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return print(String(buf));
}
