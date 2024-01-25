#ifndef __wav_file_reader_h__
#define __wav_file_reader_h__

#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "SampleSource.h"
#include "esp_log.h"


class File {
public:
    File() : fd(nullptr) {}
    ~File() {}

    // Mở tệp
    bool open(const char *path, const char *mode) {
        fd = fopen(path, mode);
        return fd != nullptr;
    }

    // Đóng tệp
    void close() {
        if (fd != nullptr) {
            fclose(fd);
            fd = nullptr;
        }
    }

    // Đọc dữ liệu từ tệp
    size_t read(uint8_t *buf, size_t size) {
        return fread(buf, 1, size, fd);
    }

    // Ghi dữ liệu vào tệp
    size_t write(const uint8_t *buf, size_t size) {
        return fwrite(buf, 1, size, fd);
    }

    // Trả về kích thước của tệp
    size_t size() {
        fseek(fd, 0, SEEK_END);
        size_t fileSize = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        return fileSize;
    }
    bool seek(uint32_t pos)
    {
        fseek(fd, pos, SEEK_SET);
    }
    // Kiểm tra xem tệp có sẵn để đọc hay không
    bool available() {
        return fd != nullptr;
    }

private:
    FILE *fd;
};
class WAVFileReader : public SampleSource
{
private:
    int m_num_channels;
    bool m_repeat;
    File m_file;

public:
    WAVFileReader(const char *file_name, bool repeat = false);
    ~WAVFileReader();
    int getFrames(Frame_t *frames, int number_frames);
    bool available();
    void reset();
};

#endif