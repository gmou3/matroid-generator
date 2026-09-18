#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

static inline int width_for(size_t line_len) {
    if (line_len <= 0xFF) return 1;
    if (line_len <= 0xFFFF) return 2;
    return 4;
}

struct ByteWriter {
    ofstream* f;
    vector<char> buf;
    size_t pos;
};

static const size_t BYTE_IO_BUF_SIZE = 1 << 16;  // 64 KiB

static inline void bw_init(ByteWriter* bw, ofstream* f) {
    bw->f = f;
    bw->buf.resize(BYTE_IO_BUF_SIZE);
    bw->pos = 0;
}

static inline void bw_flush(ByteWriter* bw) {
    if (bw->pos) bw->f->write(bw->buf.data(), static_cast<streamsize>(bw->pos));
    bw->pos = 0;
}

static inline void bw_write(ByteWriter* bw, uint32_t val, int width) {
    if (bw->pos + static_cast<size_t>(width) > bw->buf.size()) bw_flush(bw);
    memcpy(bw->buf.data() + bw->pos, &val, static_cast<size_t>(width));
    bw->pos += static_cast<size_t>(width);
}

struct ByteReader {
    ifstream* f;
    vector<char> buf;
    size_t pos;
    size_t len;
};

static inline void br_init(ByteReader* br, ifstream* f) {
    br->f = f;
    br->buf.resize(BYTE_IO_BUF_SIZE);
    br->pos = 0;
    br->len = 0;
}

static inline bool br_refill(ByteReader* br) {
    br->f->read(br->buf.data(), static_cast<streamsize>(br->buf.size()));
    br->len = static_cast<size_t>(br->f->gcount());
    br->pos = 0;
    return br->len > 0;
}

static inline int br_read(ByteReader* br, int width, uint32_t* out) {
    size_t w = static_cast<size_t>(width);
    *out = 0;  // width may be < 4 bytes; zero the rest of the value first
    if (br->pos + w > br->len) {
        // Slow path: the value straddles a refill boundary (or the buffer
        // is simply empty/exhausted).
        size_t have = br->len - br->pos;
        char tmp[4] = {0, 0, 0, 0};
        memcpy(tmp, br->buf.data() + br->pos, have);
        size_t need = w - have;
        if (!br_refill(br) || br->len < need) return -1;
        memcpy(tmp + have, br->buf.data(), need);
        br->pos = need;
        memcpy(out, tmp, w);
        return 0;
    }
    memcpy(out, br->buf.data() + br->pos, w);
    br->pos += w;
    return 0;
}

// Streaming compressor for bitstrings ('0'/'*' encoding 0/1)
class SZWriter {
   private:
    ofstream file;
    ByteWriter bw;
    size_t line_len;
    int B;
    uint64_t count;
    bool first_line;
    bool streaming;
    string prev;  // stored as original '0'/'*' chars for diff tracking

   public:
    SZWriter()
        : line_len(0), B(0), count(0), first_line(true), streaming(false) {}

    bool open(const string& filename, bool streaming = false) {
        this->streaming = streaming;
        if (filename == "-")
            file.open("/dev/stdout", ios::binary);
        else
            file.open(filename, ios::binary);
        if (!file.is_open()) return false;

        return true;
    }

    void write(const string& data) {
        if (first_line) {
            // Write header: line length and placeholder count
            line_len = data.size();
            uint32_t L32 = static_cast<uint32_t>(line_len);
            uint64_t cnt = streaming ? 0ULL : UINT64_MAX;
            file.write(reinterpret_cast<const char*>(&L32), sizeof(L32));
            file.write(reinterpret_cast<const char*>(&cnt), sizeof(cnt));

            // Write the first line verbatim
            file.write(data.data(), data.size());
            prev = data;
            count = 1;
            first_line = false;

            // Calculate block size
            B = width_for(line_len);

            // Initialize byte writer for later lines
            bw_init(&bw, &file);
            return;
        }

        // Find differing positions 8 bytes at a time and emit directly.
        const char* d = data.data();
        const char* p = prev.data();
        size_t i = 0;
        for (; i + 8 <= line_len; i += 8) {
            uint64_t a, b;
            memcpy(&a, d + i, 8);
            memcpy(&b, p + i, 8);
            uint64_t diff = a ^ b;
            while (diff) {
                // lowest differing byte within this word (little-endian)
                size_t j = (size_t)(__builtin_ctzll(diff) >> 3);
                bw_write(&bw, (uint32_t)(i + j), B);
                diff &= ~(0xFFULL << (j << 3));  // clear the whole byte
            }
        }
        for (; i < line_len; i++)
            if (d[i] != p[i]) bw_write(&bw, (uint32_t)i, B);

        bw_write(&bw, (uint32_t)line_len, B);  // sentinel

        prev = data;
        count++;
    }

    void close() {
        if (!first_line) {
            bw_flush(&bw);
        }
        if (!streaming) {
            // Update the line count in the header
            file.seekp(sizeof(uint32_t), ios::beg);
            file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        }
        file.close();
    }

    ~SZWriter() { close(); }
};

// Streaming decompressor matching SZWriter
class SZReader {
   private:
    ifstream file;
    ByteReader br;
    size_t line_len;
    int B;
    string prev;  // stored as '0'/'*' chars
    bool have_first_line;
    size_t cnt;
    size_t remaining = 0;
    bool streaming = false;  // header count 0
    bool corrupt = false;    // header count UINT64_MAX

   public:
    SZReader() : line_len(0), B(0), have_first_line(false) {}

    bool open(const string& filename) {
        if (filename == "-")
            file.open("/dev/stdin", ios::binary);
        else
            file.open(filename, ios::binary);
        if (!file.is_open()) return false;

        uint32_t L32;
        uint64_t count;
        if (!file.read(reinterpret_cast<char*>(&L32), sizeof(L32)))
            return false;
        if (!file.read(reinterpret_cast<char*>(&count), sizeof(count)))
            return false;

        line_len = static_cast<size_t>(L32);
        if (line_len == 0) return false;

        cnt = static_cast<size_t>(count);
        streaming = (count == 0);
        corrupt = (count == UINT64_MAX);
        remaining = streaming ? UINT64_MAX : cnt;

        // Calculate block size
        B = width_for(line_len);

        // Read first line verbatim
        prev.resize(line_len);
        if (!file.read(prev.data(), line_len)) return false;

        have_first_line = true;

        // Set up byte reader for the remaining data
        br_init(&br, &file);
        return true;
    }

    bool getline(string& line) {
        if (remaining == 0) return false;

        if (have_first_line) {
            line = prev;
            have_first_line = false;
            remaining--;
            return true;
        }

        line = prev;
        while (true) {
            uint32_t pos;
            if (br_read(&br, B, &pos) != 0) return false;
            if (pos == line_len) break;                  // sentinel
            line[pos] = (line[pos] == '*') ? '0' : '*';  // flip
        }

        prev = line;
        remaining--;
        return true;
    }

    bool is_complete() {
        // Check that stream ends after all lines are read
        // Trying to read another byte should fail (EOF)
        int c = file.peek();
        return c == EOF;
    }

    size_t get_remaining() const { return remaining; }

    size_t get_expected_count() const { return cnt; }

    bool is_streaming() const { return streaming; }

    bool is_corrupt() const { return corrupt; }

    size_t scan_line_count() {
        size_t n = 0;
        string line;
        while (getline(line)) n++;
        return n;
    }

    string getinfo() {
        uint64_t effective_cnt = cnt;
        if (streaming) effective_cnt = scan_line_count();
        string noun = (effective_cnt == 1) ? "string" : "strings";
        return to_string(effective_cnt) + " " + noun + " of length " +
               to_string(line_len);
    }

    void close() { file.close(); }
    ~SZReader() { close(); }
};
