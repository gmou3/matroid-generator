#include "sz.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv) {
    const char* inpath = nullptr;
    const char* outpath = nullptr;
    bool decompress = false;
    bool get_info = false;
    bool streaming = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                cerr << "sz: -o requires argument\n";
                return 1;
            }
            outpath = argv[i];
        } else if (strcmp(argv[i], "-d") == 0) {
            decompress = true;
        } else if (strcmp(argv[i], "-i") == 0) {
            get_info = true;
            decompress = true;
        } else if (strcmp(argv[i], "-s") == 0) {
            streaming = true;
        } else if (strcmp(argv[i], "-h") == 0 or
                   strcmp(argv[i], "--help") == 0) {
            cerr << "SZ: Simple compressor of fixed-length strings of '0's and "
                    "'*'s with few incremental differences\n"
                 << "usage: sz [<file>] [options]\n"
                 << "  Compresses <file> to <file>.sz, or decompresses "
                    "<file>.sz to <file> (with -d option)\n"
                 << "options:\n"
                 << "  -d            decompress mode\n"
                 << "  -o <outfile>  write output to <outfile>\n"
                 << "  -i            print info about a .sz file\n"
                 << "  -s            streaming mode: no seeks, no count in "
                    "header\n"
                 << "                (required when used in `sort "
                    "--compress-program`)\n";
            return 1;
        } else {
            inpath = argv[i];
        }
    }

    // Streaming defaults to stdin/stdout; normal mode requires a file
    string outpath_str;
    if (streaming) {
        if (!inpath) inpath = "-";
        if (!outpath) outpath = "-";
    } else {
        if (!inpath) {
            cerr << "sz: no input file specified\n";
            return 1;
        }
        if (!outpath) {
            if (decompress) {
                size_t len = strlen(inpath);
                outpath_str = (len > 3 && strcmp(inpath + len - 3, ".sz") == 0)
                                  ? string(inpath, len - 3)
                                  : string(inpath) + ".out";
            } else {
                outpath_str = string(inpath) + ".sz";
            }
            outpath = outpath_str.c_str();
        }
    }

    if (decompress) {
        SZReader reader;
        if (!reader.open(inpath)) {
            cerr << "sz: Failed to open " << inpath << '\n';
            return 1;
        }

        if (get_info) {
            cout << reader.getinfo() << '\n';
            if (!streaming && reader.get_expected_count() == UINT64_MAX) {
                cerr << "sz: file header is invalid (incomplete write)\n";
                return 1;
            }
            return 0;
        }

        ofstream fout;
        if (strcmp(outpath, "-") == 0)
            fout.open("/dev/stdout");
        else
            fout.open(outpath);
        if (!fout) {
            cerr << "sz: Failed to open " << outpath << '\n';
            return 1;
        }

        string line;
        while (reader.getline(line)) {
            fout << line << '\n';
        }
        fout.flush();

        if (!streaming) {
            if (reader.get_remaining() > 0) {
                cerr << "sz: only read "
                     << (reader.get_expected_count() - reader.get_remaining())
                     << " out of " << reader.get_expected_count()
                     << " expected strings\n";
                return 1;
            }
            if (!reader.is_complete()) {
                cerr << "sz: failed to reach EOF\n";
                return 1;
            }
        }
    } else {
        SZWriter writer;
        if (!writer.open(outpath, streaming)) {
            cerr << "sz: Failed to open " << outpath << '\n';
            return 1;
        }

        ifstream fin;
        if (strcmp(inpath, "-") == 0)
            fin.open("/dev/stdin");
        else
            fin.open(inpath);
        if (!fin) {
            cerr << "sz: Failed to open " << inpath << '\n';
            return 1;
        }

        string line;
        while (getline(fin, line)) {
            writer.write(line);
        }
    }

    return 0;
}
