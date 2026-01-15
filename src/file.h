#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>  // for std::setw and std::setfill
#include <filesystem>
namespace fs = std::filesystem;

#include "matroid.h"

// Generate filename based on n, r, and thread number
inline std::stringstream generate_filename(int n, int r, int thread_num) {
    std::stringstream filename;
    filename << "output" << "/"
             << "n" << std::setw(2) << std::setfill('0') << n
             << "r" << std::setw(2) << std::setfill('0') << r
             << "-thread" << std::setw(2) << std::setfill('0') << thread_num;

    return filename;
}

// Open files (one for each thread) and return file names
inline std::vector<std::string> open_files(int n, int r, int threads) {
    if (!fs::exists("output")) {
        if (!fs::create_directory("output")) {
            std::cerr << "Error: Could not create output directory" << std::endl;
            return {}; // Return empty vector if directory creation fails
        }
    }

    std::vector<std::string> filenames;
    for (int i = 0; i < threads; ++i) {
        std::stringstream filename = generate_filename(n, r, i);
        filenames.push_back(filename.str());
    }
    return filenames;
}

// Write a matroid's revlex to a file
inline void write_revlex_to_file(const std::string& filename, const Matroid& matroid) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (file.is_open()) {
        file << matroid.revlex << "\n";
    } else {
        std::cerr << "Error opening file: " << filename << std::endl;
    }
}

// Merge the files and delete them
inline void merge_and_delete(const std::vector<std::string>& filenames) {
    // Compute output filename based on the first input file
    std::string first_filename = filenames.front();
    size_t pos = first_filename.find('-');
    std::string output_filename = first_filename.substr(0, pos);  // Get substring up to the first '-'
    std::ofstream out(output_filename, std::ios::binary);
    if (!out) {
        std::cerr << "Error opening output file for writing" << std::endl;
        return;
    }

    for (const auto& filename : filenames) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) {
            std::cerr << "Error opening file: " << filename << std::endl;
            continue;  // Skip this file and continue
        }

        out << in.rdbuf();
        in.close();
    }

    out.close();

    // Cleanup
    for (const auto& filename : filenames) {
        if (!fs::remove(filename)) {
            std::cerr << "Failed to delete " << filename << std::endl;
        }
    }
}
