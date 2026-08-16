#ifndef INDEXER_FINAL_H
#define INDEXER_FINAL_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <cctype>
#include <cstring>
#include <chrono>
#include "stemmer.h"

namespace fs = std::filesystem;

/*
 * In this header, I define the Indexer class used for Phase 2.
 * The goal of this class is to read through the TREC document collection,
 * build both forward and inverted indices, and then provide an option
 * to test those indices using a small test file.
 */
class Indexer {
private:
    std::unordered_map<std::string,int> wordDictionary;    // word → wordID
    std::unordered_map<std::string,int> fileDictionary;    // docName → docID
    std::vector<std::string> idToWord;                     // reverse map (wordID → word)
    std::vector<std::string> idToDoc;                      // reverse map (docID → doc name)
    std::unordered_set<std::string> stopWords;             // stopword list
    std::unordered_map<int, std::unordered_map<int,int>> forwardIndex;   // docID → (wordID → freq)
    std::unordered_map<int, std::unordered_map<int,int>> invertedIndex;  // wordID → (docID → freq)
    int nextWordID = 1;
    int nextDocID  = 1;

    // Lowercase conversion utility
    static inline void to_lower_inplace(std::string &s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }

    // Extract numeric suffix from doc name for sorting
    static inline int doc_numeric_suffix(const std::string& name) {
        int i = (int)name.size() - 1;
        while (i >= 0 && !std::isdigit((unsigned char)name[i])) --i;
        if (i < 0) return 0;
        int end = i;
        while (i >= 0 && std::isdigit((unsigned char)name[i])) --i;
        std::string num = name.substr((size_t)(i + 1), (size_t)(end - i));
        try { return std::stoi(num); } catch (...) { return 0; }
    }

    // Porter stemmer wrapper
    std::string porterStem(const std::string &word) {
        if (word.empty()) return "";
        char temp[256];
        std::strncpy(temp, word.c_str(), sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
        int end = stem(temp, 0, (int)std::strlen(temp) - 1);
        if (end < 0) return "";
        temp[end + 1] = '\0';
        return std::string(temp);
    }

public:
    explicit Indexer(const std::string &stopfile) { loadStopWords(stopfile); }

    // Load stopwords into memory
    void loadStopWords(const std::string &file) {
        std::ifstream in(file);
        if (!in.is_open()) {
            std::cerr << "Could not open stopword file: " << file << "\n";
            return;
        }
        std::string w;
        while (in >> w) {
            to_lower_inplace(w);
            stopWords.insert(w);
        }
    }

    // Parse all FT911 files recursively
    void processDocuments(const std::string &folderPath) {
        for (auto &entry : fs::recursive_directory_iterator(folderPath)) {
            if (entry.is_regular_file()) parseTRECFile(entry.path().string());
        }
    }

    // Parse one TREC-style file
    void parseTRECFile(const std::string &filePath) {
        std::ifstream in(filePath);
        if (!in.is_open()) {
            std::cerr << "Cannot open " << filePath << "\n";
            return;
        }

        std::string line, docName;
        bool insideDoc = false;
        std::stringstream buffer;

        while (std::getline(in, line)) {
            if (line.find("<DOCNO>") != std::string::npos) {
                size_t s = line.find("<DOCNO>") + 7;
                size_t e = line.find("</DOCNO>");
                docName = line.substr(s, e - s);
                docName.erase(std::remove_if(docName.begin(), docName.end(), ::isspace), docName.end());
                insideDoc = true;
                buffer.str("");
                buffer.clear();
            } else if (insideDoc && line.find("</DOC>") != std::string::npos) {
                parseDocument(docName, buffer.str());
                insideDoc = false;
            } else if (insideDoc) buffer << line << ' ';
        }
    }

    // Process content of a document and update indices
    void parseDocument(const std::string &docName, const std::string &content) {
        int docID;
        auto itDoc = fileDictionary.find(docName);
        if (itDoc == fileDictionary.end()) {
            docID = nextDocID++;
            fileDictionary[docName] = docID;
            if ((int)idToDoc.size() < docID) idToDoc.resize(docID);
            idToDoc[docID - 1] = docName;
        } else docID = itDoc->second;

        std::stringstream ss(content);
        std::string token;
        while (ss >> token) {
            to_lower_inplace(token);
            token.erase(std::remove_if(token.begin(), token.end(),
                        [](unsigned char c){ return !std::isalpha(c); }),
                        token.end());

            if (token.empty() || token.size() < 2) continue;
            if (stopWords.count(token)) continue;

            std::string stemmed = porterStem(token);
            if (stemmed.empty() || stemmed.size() < 2) continue;

            int wordID;
            auto itW = wordDictionary.find(stemmed);
            if (itW == wordDictionary.end()) {
                wordID = nextWordID++;
                wordDictionary[stemmed] = wordID;
                if ((int)idToWord.size() < wordID) idToWord.resize(wordID);
                idToWord[wordID - 1] = stemmed;
            } else wordID = itW->second;

            forwardIndex[docID][wordID]++;
            invertedIndex[wordID][docID]++;
        }
    }

    void ensureOutputDir(const std::string &outDir) {
        if (!fs::exists(outDir)) fs::create_directories(outDir);
    }

    // Write the forward index to file
    void writeForwardIndex(const std::string &fileName) {
        std::ofstream out(fileName);
        if (!out.is_open()) { std::cerr << "Cannot write " << fileName << "\n"; return; }

        std::vector<int> docIDs;
        for (auto &d : forwardIndex) docIDs.push_back(d.first);
        std::sort(docIDs.begin(), docIDs.end());

        for (int dID : docIDs) {
            out << dID << ": ";
            std::vector<std::pair<int,int>> pairs(forwardIndex[dID].begin(), forwardIndex[dID].end());
            std::sort(pairs.begin(), pairs.end());
            for (size_t i = 0; i < pairs.size(); ++i) {
                if (i) out << "; ";
                out << pairs[i].first << ": " << pairs[i].second;
            }
            out << ";\n";
        }
    }

    // Write inverted index
    void writeInvertedIndex(const std::string &fileName) {
        std::ofstream out(fileName);
        if (!out.is_open()) { std::cerr << "Cannot write " << fileName << "\n"; return; }

        std::vector<int> wordIDs;
        for (auto &w : invertedIndex) wordIDs.push_back(w.first);
        std::sort(wordIDs.begin(), wordIDs.end());

        for (int wID : wordIDs) {
            out << wID << ": ";
            std::vector<std::pair<int,int>> pairs(invertedIndex[wID].begin(), invertedIndex[wID].end());
            std::sort(pairs.begin(), pairs.end());
            for (size_t i = 0; i < pairs.size(); ++i) {
                if (i) out << "; ";
                out << pairs[i].first << ": " << pairs[i].second;
            }
            out << ";\n";
        }
    }

    void writeTermDictionary(const std::string &fileName) {
        std::ofstream out(fileName);
        for (size_t i = 0; i < idToWord.size(); ++i)
            if (!idToWord[i].empty())
                out << std::left << std::setw(25) << idToWord[i] << '\t' << (i + 1) << '\n';
    }

    void writeDocumentDictionary(const std::string &fileName) {
        std::ofstream out(fileName);
        for (size_t i = 0; i < idToDoc.size(); ++i)
            if (!idToDoc[i].empty())
                out << std::left << std::setw(25) << idToDoc[i] << '\t' << (i + 1) << '\n';
    }

    // Handle queries from testdata file
    void queryFromFile(const std::string &fileName) {
        std::ifstream in(fileName);
        if (!in.is_open()) {
            std::cerr << "Cannot open test file: " << fileName << "\n";
            return;
        }

        std::string term;
        while (in >> term) {
            to_lower_inplace(term);

            // Skip XML-like tags such as <DOC>, <TEXT>, etc.
            if (!term.empty() && term.front() == '<' && term.back() == '>') continue;

            term.erase(std::remove_if(term.begin(), term.end(),
                        [](unsigned char c){ return !std::isalpha(c); }),
                        term.end());

            if (term.empty() || term.size() < 2) continue;
            if (stopWords.count(term)) {
                std::cout << "'" << term << "' is a stopword. Skipped.\n";
                continue;
            }

            std::string s = porterStem(term);
            auto it = wordDictionary.find(s);
            if (it == wordDictionary.end()) {
                std::cout << "The term '" << term << "' (stem '" << s << "') was not found.\n";
            } else {
                int wid = it->second;
                auto pit = invertedIndex.find(wid);
                if (pit == invertedIndex.end()) {
                    std::cout << "The term '" << term << "' has no postings.\n";
                } else {
                    std::cout << "Inverted postings for '" << term << "' (stem '" << s << "'):\n";
                    std::vector<std::pair<int,int>> postings(pit->second.begin(), pit->second.end());
                    std::sort(postings.begin(), postings.end());
                    std::cout << "  wordID" << wid << ": ";
                    for (auto &kv : postings)
                        std::cout << "docID" << kv.first << ": " << kv.second << "; ";
                    std::cout << "\n";
                }
            }
        }
        std::cout << "\nFinished reading all terms from test file.\n";
    }

    // Print summary
    void printSummary() const {
        size_t postings = 0;
        for (auto &w : invertedIndex) postings += w.second.size();
        std::cout << "\nSummary:\n";
        std::cout << "  Unique terms: " << wordDictionary.size() << "\n";
        std::cout << "  Documents indexed: " << fileDictionary.size() << "\n";
        std::cout << "  Total postings: " << postings << "\n";
    }
        // Manual interactive query loop
    // I kept this for when I want to test individual queries myself.
    void queryLoop() {
        std::string q;
        std::cout << "\nEnter a term to search (or 'exit'): ";

        while (std::getline(std::cin, q)) {
            q.erase(std::remove_if(q.begin(), q.end(),
                    [](unsigned char c){ return std::isspace(c); }), q.end());
            to_lower_inplace(q);

            if (q == "exit" || q == "quit") {
                std::cout << "Exiting search.\n";
                break;
            }

            // Clean the input term (letters only)
            q.erase(std::remove_if(q.begin(), q.end(),
                    [](unsigned char c){ return !std::isalpha(c); }), q.end());

            if (q.empty() || q.size() < 2) {
                std::cout << "Invalid or too short term. Try again.\n";
                std::cout << "\nEnter a term to search (or 'exit'): ";
                continue;
            }

            // Skip stopwords
            if (stopWords.count(q)) {
                std::cout << "'" << q << "' is a stopword. Skipped.\n";
                std::cout << "\nEnter a term to search (or 'exit'): ";
                continue;
            }

            std::string s = porterStem(q);
            auto it = wordDictionary.find(s);
            if (it == wordDictionary.end()) {
                std::cout << "The term '" << q << "' (stem '" << s << "') was not found.\n";
            } else {
                int wid = it->second;
                auto pit = invertedIndex.find(wid);
                if (pit == invertedIndex.end()) {
                    std::cout << "The term '" << q << "' has no postings.\n";
                } else {
                    std::cout << "Inverted postings for '" << q << "' (stem '" << s << "'):\n";
                    std::vector<std::pair<int,int>> postings(pit->second.begin(), pit->second.end());
                    std::sort(postings.begin(), postings.end());
                    std::cout << "  wordID" << wid << ": ";
                    for (auto &kv : postings)
                        std::cout << "docID" << kv.first << ": " << kv.second << "; ";
                    std::cout << "\n";
                }
            }
            std::cout << "\nEnter a term to search (or 'exit'): ";
        }
    }

};

#endif
