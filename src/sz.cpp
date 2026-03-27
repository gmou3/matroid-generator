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
        } else {
            inpath = argv[i];
        }
    }

    if (!inpath) {
        cerr
            << "SZ: Simple compressor of fixed-length strings of '0's and '*'s "
               "with few incremental differences\n"
            << "usage: sz <file> [options]\n"
            << "  Compresses <file> to <file>.sz, or decompresses <file>.sz "
               "to <file> (with -d option)\n"
            << "options:\n"
            << "  -d            decompress mode\n"
            << "  -o <outfile>  write output to <outfile> (use - for stdout)\n";
        return 1;
    }

    string outpath_str;
    if (!outpath) {
        if (decompress) {
            outpath_str = string(inpath, strlen(inpath) - 3);
        } else {
            outpath_str = string(inpath) + ".sz";
        }
        outpath = outpath_str.c_str();
    }

    if (decompress) {
        SZReader reader;
        if (!reader.open(inpath)) {
            cerr << "sz: Failed to open " << inpath << '\n';
            return 1;
        }

        if (get_info) {
            cout << reader.getinfo() << '\n';
            return 0;
        }

        ofstream fout;
        if (strcmp(outpath, "-") == 0) {
            fout.open("/dev/stdout");
        } else {
            fout.open(outpath);
        }
        if (!fout) {
            cerr << "sz: Failed to open " << outpath << '\n';
            return 1;
        }

        string line;
        while (reader.getline(line)) {
            fout << line << '\n';
        }
    } else {
        SZWriter writer;
        if (!writer.open(outpath)) {
            cerr << "sz: Failed to open " << outpath << '\n';
            return 1;
        }

        ifstream fin(inpath);
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
