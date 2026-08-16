#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>
#include "PorterStemmer.hpp"

// Class to tokenize text, removing stopwords and applying stemming
class TextTokenizer {
public:
    // Constructor: Initializes the tokenizer with a set of stopwords
    // @param stopwords : Set of words to be excluded from tokenization
    TextTokenizer(const std::unordered_set<std::string>& stopwords) : stopwords(stopwords) {}

    // Tokenizes input text into words, removes stopwords, applies stemming
    // @param text : The input text to tokenize
    // @return Vector : of stemmed tokens
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        // Regex to match alphabetic words only
        std::regex matcher(R"(\b[a-zA-Z]+\b)");

        // Iterate over all matches in the text
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), matcher);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::string token = i->str();
            // Convert token to lowercase 
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);

            // Skip if token is a stopword
            if (stopwords.find(token) == stopwords.end()) {
                // Apply Porter stemming and add to tokens
                tokens.push_back(PorterStemmer::stem(token));
            }
        }
        return tokens;
    }

private:
    std::unordered_set<std::string> stopwords; // Set of stopwords for filtering
};

// Class to maintain a word index with unique IDs
class WordIndex {
public:
    // Adds a word to the index and assigns a unique ID
    // @param word : The word to add
    void add(const std::string& word) {
        // Assign a new ID if the word is not already in the map
        if (word_map.find(word) == word_map.end()) {
            word_map[word] = id_counter++;
        }
    }

    // Prints the word index to an output file
    // @param writer The output file stream
    void print(std::ofstream& writer) {
        // Write each word and its ID to the file
        for (const auto& pair : word_map) {
            writer << pair.first << " " << pair.second << "\n";
        }
    }

private:
    std::unordered_map<std::string, int> word_map; // Maps words to unique IDs
    int id_counter = 1; // Counter for assigning unique IDs
};

// Class to maintain a document index with unique IDs
class DocumentIndex {
public:
    // Adds a document name to the index and assigns a unique ID
    // @param doc_name : The document name to add
    void add(const std::string& doc_name) {
        // Assign a new ID if the document is not already in the map
        if (doc_map.find(doc_name) == doc_map.end()) {
            doc_map[doc_name] = id_counter++;
        }
    }

    // Prints the document index to an output file
    // @param writer : The output file stream
    void print(std::ofstream& writer) {
        // Write each document name and its ID to the file
        for (const auto& pair : doc_map) {
            writer << pair.first << " " << pair.second << "\n";
        }
    }

private:
    std::unordered_map<std::string, int> doc_map; // Maps document names to unique IDs
    int id_counter = 1; // Counter for assigning unique IDs
};

// Class to parse files and update word and document indices
class Parser {
public:
    // Constructor: Initializes the parser with tokenizer and indices
    // The tokenizer to process text
    // The word index to update
    // The document index to update
    Parser(TextTokenizer& tokenizer, WordIndex& word_index, DocumentIndex& doc_index)
        : tokenizer(tokenizer), word_index(word_index), doc_index(doc_index) {}

    // Parses a file, extracts documents, and updates indices
    // file_path The path to the input file
    void parse(const std::string& file_path) {
        // Open the input file
        std::ifstream reader(file_path);
        if (!reader.is_open()) {
            throw std::ios_base::failure("Error opening file: " + file_path);
        }

        std::string line, doc_id, doc_content;
        bool is_in_doc = false; // Tracks if currently inside a <DOC> block
        // Regex to extract document ID from <DOCNO> tags
        std::regex docno_regex(R"(<DOCNO>(.*?)</DOCNO>)");

        // Read the file line by line
        while (std::getline(reader, line)) {
            if (line.empty()) continue; // Skip empty lines

            // Extract document ID from <DOCNO> tags
            if (line.find("<DOCNO>") != std::string::npos) {
                std::smatch match;
                if (std::regex_search(line, match, docno_regex) && match.size() > 1) {
                    doc_id = match[1].str();
                    doc_index.add(doc_id); // Add document ID to index
                }
            }
            // Start of a document block
            else if (line.find("<DOC>") != std::string::npos) {
                is_in_doc = true;
                doc_content.clear(); // Clear content for new document
            }
            // End of a document block
            else if (line.find("</DOC>") != std::string::npos) {
                is_in_doc = false;
                // Tokenize document content and add tokens to word index
                auto tokens = tokenizer.tokenize(doc_content);
                for (const auto& token : tokens) {
                    word_index.add(token);
                }
            }
            else if (is_in_doc) {
                doc_content += line + " ";
            }
        }
    }

private:
    TextTokenizer& tokenizer; // Reference to the tokenizer
    WordIndex& word_index;   // Reference to the word index
    DocumentIndex& doc_index; // Reference to the document index
};

// Loads stopwords from a file into a set
std::unordered_set<std::string> load_stopwords(const std::string& filepath) {
    std::unordered_set<std::string> stopwords;
    std::ifstream reader(filepath);
    if (!reader.is_open()) {
        throw std::ios_base::failure("Error opening stopword list: " + filepath);
    }

    std::string line;
    // Read each line and add to stopwords set
    while (std::getline(reader, line)) {
        stopwords.insert(line);
    }
    return stopwords;
}

// Retrieves all regular files from a directory
// @param directory : The directory path
std::vector<std::string> get_files(const std::string& directory) {
    std::vector<std::string> files;
    // Iterate over directory entries
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string()); // Add file path to vector
        }
    }
    return files;
}

// Main function to process files and generate indices
int main() {
    try {
        WordIndex word_index;         // Initialize word index
        DocumentIndex doc_index;      // Initialize document index

        // Load stopwords from file
        auto stopwords = load_stopwords("stopwordlist.txt");
        TextTokenizer tokenizer(stopwords); // Initialize tokenizer with stopwords

        // Initialize parser with tokenizer and indices
        Parser parser(tokenizer, word_index, doc_index);
        // Get all files in the ft911 directory
        std::vector<std::string> files = get_files("ft911");

        // Check if No files were found
        if (files.empty()) {
            std::cerr << "No files found in the ft911 directory." << std::endl;
            return 1;
        }

        // Parse each file and update indices
        for (const auto& file : files) {
            std::cout << "Parsing file: " << file << std::endl;
            parser.parse(file);
        }

        // Open output file for writing indices
        std::ofstream writer("parser_output.txt");
        if (!writer.is_open()) {
            std::cerr << "Failed to open parser_output.txt for writing." << std::endl;
            return 1;
        }

        // Write vocabulary and document indices to output file
        writer << "Vocabulary Index:\n";
        word_index.print(writer);
        writer << "\nDocument Index:\n";
        doc_index.print(writer);

        std::cout << "Results stored in file parser_output.txt." << std::endl;
    } catch (const std::ios_base::failure& e) {
        // Handle I/O errors (e.g., file not found)
        std::cerr << "I/O Error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Handle other standard exceptions
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}