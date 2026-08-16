#include "indexer.h"
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // I take command-line arguments in the same order used in Phase 1.
    std::string trec = (argc > 1 ? argv[1] : "ft911");
    std::string stop = (argc > 2 ? argv[2] : "stopwordlist.txt");
    std::string outd = (argc > 3 ? argv[3] : "output");
    std::string testFile = (argc > 4 ? argv[4] : "testdata.txt");

    Indexer indexer(stop);
    indexer.ensureOutputDir(outd);

    bool existing = fs::exists(outd + "/forward_index.txt") &&
                    fs::exists(outd + "/inverted_index.txt") &&
                    fs::exists(outd + "/term_dictionary.txt") &&
                    fs::exists(outd + "/document_dictionary.txt");

    auto start = std::chrono::high_resolution_clock::now();

    if (existing) {
        std::cout << "Existing index found in '" << outd << "'. Skipping indexing.\n";
    } else {
        std::cout << "Indexing from: " << trec << "\n";
        indexer.processDocuments(trec);

        auto parseEnd = std::chrono::high_resolution_clock::now();
        double parseTime = std::chrono::duration<double>(parseEnd - start).count();

        indexer.writeForwardIndex(outd + "/forward_index.txt");
        indexer.writeInvertedIndex(outd + "/inverted_index.txt");
        indexer.writeTermDictionary(outd + "/term_dictionary.txt");
        indexer.writeDocumentDictionary(outd + "/document_dictionary.txt");

        auto writeEnd = std::chrono::high_resolution_clock::now();
        double writeTime = std::chrono::duration<double>(writeEnd - parseEnd).count();

        std::cout << "\nSummary after indexing:\n";
        indexer.printSummary();

        std::cout << "\nTiming:\n";
        std::cout << "  Parse + Index time: " << parseTime << " seconds\n";
        std::cout << "  File writing time:  " << writeTime << " seconds\n";

        std::cout << "\nFile sizes (bytes):\n";
        std::cout << "  forward_index.txt      " << fs::file_size(outd + "/forward_index.txt") << "\n";
        std::cout << "  inverted_index.txt     " << fs::file_size(outd + "/inverted_index.txt") << "\n";
        std::cout << "  term_dictionary.txt    " << fs::file_size(outd + "/term_dictionary.txt") << "\n";
        std::cout << "  document_dictionary.txt " << fs::file_size(outd + "/document_dictionary.txt") << "\n";

        double totalSize = (fs::file_size(outd + "/forward_index.txt") +
                            fs::file_size(outd + "/inverted_index.txt") +
                            fs::file_size(outd + "/term_dictionary.txt") +
                            fs::file_size(outd + "/document_dictionary.txt")) / (1024.0 * 1024.0);
        std::cout << "\nTotal index size: " << totalSize << " MB (approx.)\n";
    }

    // Always print summary — even if the index already existed (safe check for me)
    std::cout << "\nSummary:\n";
    indexer.printSummary();

    std::cout << "\nForward and Inverted Indices Ready.\n";
    std::cout << "Now I can test the index using the query interface.\n";

    // Ask if the user wants to test automatically using a file
    std::cout << "Do you want to use a test query file (y/n)? ";
    std::string ans;
    std::getline(std::cin, ans);

    if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
        std::cout << "Enter the test file path (or press Enter for default 'testdata.txt'): ";
        std::string fileInput;
        std::getline(std::cin, fileInput);
        if (fileInput.empty()) fileInput = testFile;

        std::cout << "\nRunning automated test queries from " << fileInput << "...\n\n";
        indexer.queryFromFile(fileInput);
    } else {
        indexer.queryLoop();
    }

    return 0;
}
