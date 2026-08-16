#ifndef PORTER_STEMMER_HPP
#define PORTER_STEMMER_HPP

#include <string>

// C++ Class implementing a simplified version of the Porter Stemming Algorithm
class PorterStemmer {
public:
    // Stems a word to its root form using a simplified version of the Porter Stemming Algorithm
    // @param word : The input word to stem (lowercase obtained from parser.cpp)
    // @return The stemmed word
    static std::string stem(const std::string& word) {
        // Return short words (length <= 2) unchanged
        if (word.length() <= 2) return word;

        std::string w = word; // Working copy of the input word

        // 1a: Handle plural forms and simple suffixes
        if (w.length() >= 4 && w.compare(w.length() - 4, 4, "sses") == 0) {
            w = w.substr(0, w.length() - 2); // Replace "sses" with "ss" (e.g., "caresses" -> "caress")
        } else if (w.length() >= 3 && w.compare(w.length() - 3, 3, "ies") == 0) {
            w = w.substr(0, w.length() - 2); // Replace "ies" with "i" (e.g., "ponies" -> "poni")
        } else if (w.length() >= 2 && w.compare(w.length() - 1, 1, "s") == 0 && w.compare(w.length() - 2, 2, "ss") != 0) {
            w = w.substr(0, w.length() - 1); // Remove single "s" unless it ends in "ss" (e.g., "cats" -> "cat")
        }

        // 1b: Handle past participles and verb forms
        if (w.length() >= 3 && w.compare(w.length() - 3, 3, "eed") == 0 && vowel_consonant_count(w.substr(0, w.length() - 3)) > 0) {
            w = w.substr(0, w.length() - 1); // Replace "eed" with "ee" if m > 0 (e.g., "agreed" -> "agree")
        } else if ((w.length() >= 2 && w.compare(w.length() - 2, 2, "ed") == 0 && has_vowel(w.substr(0, w.length() - 2))) ||
                   (w.length() >= 3 && w.compare(w.length() - 3, 3, "ing") == 0 && has_vowel(w.substr(0, w.length() - 3)))) {
            // Remove "ed" or "ing" if the stem contains a vowel (e.g., "walked" -> "walk", "walking" -> "walk")
            w = w.substr(0, w.length() - (w.compare(w.length() - 2, 2, "ed") == 0 ? 2 : 3));

            // Handle special cases after removing "ed" or "ing"
            if (w.length() >= 2 && (w.compare(w.length() - 2, 2, "at") == 0 || w.compare(w.length() - 2, 2, "bl") == 0 || w.compare(w.length() - 2, 2, "iz") == 0)) {
                w += "e"; // Append "e" for specific endings (e.g., "rat" -> "rate", "bl" -> "ble")
            } else if (ends_with_double_consonants(w) && w.back() != 'l' && w.back() != 's' && w.back() != 'z') {
                w = w.substr(0, w.length() - 1); // Remove duplicate consonant (e.g., "hopp" -> "hop")
            } else if (vowel_consonant_count(w) == 1 && ends_with_cvc(w)) {
                w += "e"; // Append "e" for consonant-vowel-consonant pattern (e.g., "lov" -> "love")
            }
        }

        // 1c: Change terminal "y" to "i" if preceded by a vowel
        if (w.length() >= 2 && w.back() == 'y' && has_vowel(w.substr(0, w.length() - 1))) {
            w.back() = 'i'; // Replace "y" with "i" (e.g., "happy" -> "happi")
        }

        return w; // Return the stemmed word
    }

private:
    // Determines if the character at the given index is a consonant
    // @param word : The input word
    // @param index : The index of the character to check
    // @return true if the character is a consonant, false otherwise
    static bool is_consonant(const std::string& word, size_t index) {
        char character = word[index];
        // Vowels (a, e, i, o, u) are not consonants
        if (character == 'a' || character == 'e' || character == 'i' || character == 'o' || character == 'u') {
            return false;
        }
        // 'y' is a consonant if at the start or preceded by a consonant
        return character != 'y' || (index == 0 || is_consonant(word, index - 1));
    }

    // Counts vowel-to-consonant transitions (measure m) in the word
    // @param word : The input word
    // @return The number of vowel-to-consonant transitions
    static int vowel_consonant_count(const std::string& word) {
        int count = 0;
        bool is_prev_consonant = true; // Assume previous character is a consonant initially
        for (size_t i = 0; i < word.length(); ++i) {
            bool current_is_consonant = is_consonant(word, i);
            if (!is_prev_consonant && current_is_consonant) {
                ++count; // Increment count on vowel-to-consonant transition
            }
            is_prev_consonant = current_is_consonant;
        }
        return count;
    }

    // Checks if the word contains at least one vowel
    // @param word : The input word
    // @return true if a vowel is present, false otherwise
    static bool has_vowel(const std::string& word) {
        for (char c : word) {
            if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y') {
                return true;
            }
        }
        return false;
    }

    // Checks if the word ends with a double consonant
    // @param word : The input word
    // @return true if the word ends with a double consonant, false otherwise
    static bool ends_with_double_consonants(const std::string& word) {
        return word.length() >= 2 && word[word.length() - 1] == word[word.length() - 2] &&
               is_consonant(word, word.length() - 1);
    }

    // Checks if the word ends with a consonant-vowel-consonant pattern (not ending with w, x, y)
    // @param word : The input word
    // @return true if the pattern is cvc and valid, false otherwise
    static bool ends_with_cvc(const std::string& word) {
        if (word.length() < 3) return false;
        return is_consonant(word, word.length() - 3) &&
               !is_consonant(word, word.length() - 2) &&
               is_consonant(word, word.length() - 1) &&
               word.back() != 'w' && word.back() != 'x' && word.back() != 'y';
    }
};

#endif 