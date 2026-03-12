#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

static inline int bits_for(size_t max_val) {
    if (max_val == 0) return 1;
    int b = 0;
    while (max_val > 0) {
        b++;
        max_val >>= 1;
    }
    return b;
}

struct BitWriter {
    ofstream* f;
    uint8_t buf;
    int bit_pos;
};

static inline void bw_init(BitWriter* bw, ofstream* f) {
    bw->f = f;
    bw->buf = 0;
    bw->bit_pos = 7;
}

static inline void bw_write_bit(BitWriter* bw, int bit) {
    if (bit) bw->buf |= (1u << bw->bit_pos);
    if (bw->bit_pos-- == 0) {
        bw->f->write(reinterpret_cast<char*>(&bw->buf), 1);
        bw->buf = 0;
        bw->bit_pos = 7;
    }
}

static inline void bw_write_bits(BitWriter* bw, uint32_t val, int n) {
    for (int i = n - 1; i >= 0; i--) bw_write_bit(bw, (val >> i) & 1);
}

static inline void bw_flush(BitWriter* bw) {
    if (bw->bit_pos < 7) bw->f->write(reinterpret_cast<char*>(&bw->buf), 1);
}

struct BitReader {
    ifstream* f;
    uint8_t buf;
    int bit_pos;
};

static inline void br_init(BitReader* br, ifstream* f) {
    br->f = f;
    br->bit_pos = -1;
}

static inline int br_read_bit(BitReader* br) {
    if (br->bit_pos < 0) {
        int c = br->f->get();
        if (c == EOF) return -1;
        br->buf = (uint8_t)c;
        br->bit_pos = 7;
    }
    return (br->buf >> br->bit_pos--) & 1;
}

static inline int br_read_bits(BitReader* br, int n, uint32_t* out) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
        int b = br_read_bit(br);
        if (b < 0) return -1;
        v = (v << 1) | b;
    }
    *out = v;
    return 0;
}

// Streaming compressor for bitstrings ('0'/'*' encoding 0/1)
class SZWriter {
   private:
    ofstream file;
    BitWriter bw;
    size_t line_len;
    int B;
    uint64_t count;
    bool first_line;
    string prev;  // stored as original '0'/'*' chars for diff tracking

   public:
    SZWriter() : line_len(0), B(0), count(0), first_line(true) {}

    bool open(const string& filename) {
        file.open(filename, ios::binary);
        if (!file.is_open()) return false;
        // Placeholder header: line length (u32), count (u64)
        uint32_t L32 = 0;
        uint64_t cnt = UINT64_MAX;  // sentinel meaning "in progress"
        file.write(reinterpret_cast<const char*>(&L32), sizeof(L32));
        file.write(reinterpret_cast<const char*>(&cnt), sizeof(cnt));
        return true;
    }

    void write(const string& data) {
        if (first_line) {
            line_len = data.size();
            // Update the header with the true line length
            uint32_t L32 = static_cast<uint32_t>(line_len);
            file.seekp(0, ios::beg);
            file.write(reinterpret_cast<const char*>(&L32), sizeof(L32));
            file.seekp(0, ios::end);

            // Write the first line verbatim
            file.write(data.data(), data.size());
            prev = data;
            count = 1;
            first_line = false;

            // Calculate block size
            B = bits_for(line_len);

            // Initialize bit writer for later lines
            bw_init(&bw, &file);
            return;
        }

        // Collect differing positions
        vector<size_t> diffs;
        diffs.reserve(line_len);
        for (size_t i = 0; i < line_len; i++)
            if (data[i] != prev[i]) diffs.push_back(i);

        // Encode each flipped position followed by a seperator value
        for (size_t pos : diffs) bw_write_bits(&bw, (uint32_t)pos, B);
        bw_write_bits(&bw, (uint32_t)line_len, B);  // sentinel

        prev = data;
        count++;
    }

    void close() {
        if (!first_line) {
            bw_flush(&bw);
        }

        // Update the line count in the header
        file.seekp(sizeof(uint32_t), ios::beg);
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        file.close();
    }

    ~SZWriter() { close(); }
};

// Streaming decompressor matching SZWriter
class SZReader {
   private:
    ifstream file;
    BitReader br;
    size_t line_len;
    int B;
    string prev;  // stored as '0'/'*' chars
    bool have_first_line;
    size_t remaining = 0;

   public:
    SZReader() : line_len(0), B(0), have_first_line(false) {}

    bool open(const string& filename) {
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
        remaining = count;

        // Calculate block size
        B = bits_for(line_len);

        // Read first line verbatim
        prev.resize(line_len);
        if (!file.read(prev.data(), line_len)) return false;

        have_first_line = true;

        // Set up bit reader for the remaining data
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

        string cur = prev;
        while (true) {
            uint32_t pos;
            if (br_read_bits(&br, B, &pos) != 0) return false;
            if (pos == line_len) break;                // sentinel
            cur[pos] = (cur[pos] == '*') ? '0' : '*';  // flip
        }

        line = cur;
        prev = cur;
        remaining--;
        return true;
    }

    void close() { file.close(); }
    ~SZReader() { close(); }
};
