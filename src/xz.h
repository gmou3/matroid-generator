#include <lzma.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

const size_t COMPRESS_BUFFER_SIZE = 65536;

class XZWriter {
   private:
    lzma_stream stream;
    ofstream file;
    vector<uint8_t> out_buffer;
    bool initialized;

   public:
    XZWriter() : out_buffer(COMPRESS_BUFFER_SIZE), initialized(false) {
        stream = LZMA_STREAM_INIT;
    }

    bool open(const string& filename, uint32_t threads = 1) {
        file.open(filename, ios::binary);
        if (!file.is_open()) return false;

        lzma_ret ret;

        if (threads > 1) {
            lzma_mt mt_options = {};
            mt_options.threads = threads;
            mt_options.preset = 9 | LZMA_PRESET_EXTREME;
            mt_options.check = LZMA_CHECK_CRC64;
            ret = lzma_stream_encoder_mt(&stream, &mt_options);
        } else {
            ret = lzma_easy_encoder(&stream, 9 | LZMA_PRESET_EXTREME,
                                    LZMA_CHECK_CRC64);
        }

        if (ret != LZMA_OK) {
            file.close();
            return false;
        }

        initialized = true;
        return true;
    }

    void write(const string& data) {
        if (!initialized) return;

        stream.next_in = reinterpret_cast<const uint8_t*>(data.c_str());
        stream.avail_in = data.size();

        while (stream.avail_in > 0) {
            stream.next_out = out_buffer.data();
            stream.avail_out = out_buffer.size();
            lzma_ret ret = lzma_code(&stream, LZMA_RUN);
            (void)ret;  // supress unused warning

            size_t write_size = out_buffer.size() - stream.avail_out;
            if (write_size > 0) {
                file.write(reinterpret_cast<char*>(out_buffer.data()),
                           write_size);
            }
        }
    }

    void close() {
        if (!initialized) return;

        lzma_ret ret;
        do {
            stream.next_out = out_buffer.data();
            stream.avail_out = out_buffer.size();
            ret = lzma_code(&stream, LZMA_FINISH);

            size_t write_size = out_buffer.size() - stream.avail_out;
            if (write_size > 0) {
                file.write(reinterpret_cast<char*>(out_buffer.data()),
                           write_size);
            }
        } while (ret == LZMA_OK);

        lzma_end(&stream);
        file.close();
        initialized = false;
    }

    ~XZWriter() {
        if (initialized) close();
    }
};

class XZReader {
   private:
    lzma_stream stream;
    ifstream file;
    vector<uint8_t> in_buffer;
    vector<uint8_t> out_buffer;
    bool initialized;
    bool eof;
    string line_buffer;
    size_t buffer_pos;

   public:
    XZReader()
        : in_buffer(COMPRESS_BUFFER_SIZE),
          out_buffer(COMPRESS_BUFFER_SIZE),
          initialized(false),
          eof(false),
          buffer_pos(0) {
        stream = LZMA_STREAM_INIT;
    }

    bool open(const string& filename) {
        file.open(filename, ios::binary);
        if (!file.is_open()) return false;

        if (lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED) !=
            LZMA_OK) {
            file.close();
            return false;
        }

        initialized = true;
        stream.avail_in = 0;
        return true;
    }

    bool getline(string& line) {
        if (!initialized) return false;
        line.clear();

        while (true) {
            // Check for complete line in buffer
            size_t newline_pos = line_buffer.find('\n', buffer_pos);
            if (newline_pos != string::npos) {
                line = line_buffer.substr(buffer_pos, newline_pos - buffer_pos);
                buffer_pos = newline_pos + 1;
                return true;
            }

            // Compact buffer
            if (buffer_pos > 0) {
                line_buffer = line_buffer.substr(buffer_pos);
                buffer_pos = 0;
            }

            // Check if we're done
            if (eof && stream.avail_in == 0) {
                if (!line_buffer.empty()) {
                    line = line_buffer;
                    line_buffer.clear();
                    return !line.empty();
                }
                return false;
            }

            // Read more compressed data
            if (stream.avail_in == 0 && !file.eof()) {
                file.read(reinterpret_cast<char*>(in_buffer.data()),
                          in_buffer.size());
                stream.avail_in = file.gcount();
                stream.next_in = in_buffer.data();
            }

            // Decompress
            stream.next_out = out_buffer.data();
            stream.avail_out = out_buffer.size();
            lzma_ret ret = lzma_code(
                &stream,
                (file.eof() && stream.avail_in == 0) ? LZMA_FINISH : LZMA_RUN);

            size_t decompressed = out_buffer.size() - stream.avail_out;
            if (decompressed > 0) {
                line_buffer.append(reinterpret_cast<char*>(out_buffer.data()),
                                   decompressed);
            }

            if (ret == LZMA_STREAM_END) {
                eof = true;
            } else if (ret != LZMA_OK) {
                return false;
            }
        }
    }

    void close() {
        if (initialized) {
            lzma_end(&stream);
            file.close();
            initialized = false;
        }
    }

    ~XZReader() {
        if (initialized) close();
    }
};
