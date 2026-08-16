#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cctype>

using namespace std;

// Global data structures used to store term ids, document ids, indexes, relevance sets, and stopwords
unordered_map<string, int> word_index;
unordered_map<int, string> doc_index;
unordered_map<int, unordered_map<int, int>> inverted_index;
unordered_map<int, unordered_map<int, int>> forward_index;
unordered_map<int, unordered_set<string>> relevance;
unordered_set<string> stopwords;

// Porter Stemmer for reducing words to root form
class PorterStemmer {
public:
    string stem(const string& word) {
        string s = word;
        for (char& c : s) c = tolower(c); // convert to lowercase
        s = clean(s); // remove non alphanumeric
        if (s.length() <= 2) return s;
        s = strip_prefixes(s); // handle prefixes
        if (!s.empty()) {
            s = step1(s);
            s = step2(s);
            s = step3(s);
            s = step4(s);
            s = step5(s);
        }
        return s;
    }

private:
    string clean(const string& s) {
        string res;
        for (char c : s) if (isalnum(c)) res += c; // keep only alphanumeric
        return res;
    }

    bool ends_with(const string& s, const string& suf) {
        if (s.length() < suf.length()) return false;
        return s.substr(s.length() - suf.length()) == suf;
    }

    string replace_suffix(const string& s, const string& suf, const string& rep) {
        if (ends_with(s, suf)) return s.substr(0, s.length() - suf.length()) + rep;
        return s;
    }

    int measure(const string& s) {
        int m = 0, i = 0;
        while (i < s.length()) {
            while (i < s.length() && !is_vowel(s[i], i == 0 ? ' ' : s[i-1])) i++;
            while (i < s.length() && is_vowel(s[i], i == 0 ? ' ' : s[i-1])) i++;
            if (i < s.length()) { m++; i++; }
        }
        return m;
    }

    bool is_vowel(char c, char prev) {
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') return true;
        if (c == 'y') return prev != 'a' && prev != 'e' && prev != 'i' && prev != 'o' && prev != 'u';
        return false;
    }

    bool contains_vowel(const string& s) {
        for (size_t i = 0; i < s.length(); ++i)
            if (is_vowel(s[i], i == 0 ? ' ' : s[i-1])) return true;
        return false;
    }

    bool cvc(const string& s) {
        if (s.length() < 3) return false;
        char a = s[s.length()-3], b = s[s.length()-2], c = s[s.length()-1];
        return !is_vowel(c, b) && c != 'w' && c != 'x' && c != 'y' && is_vowel(b, a);
    }

    string step1(string s) {
        if (ends_with(s, "sses")) return s.substr(0, s.length()-2); // stemming rule
        if (ends_with(s, "ies"))  return s.substr(0, s.length()-2);
        if (s.back() == 's' && s.length() > 1 && s[s.length()-2] != 's') s.pop_back();

        if (ends_with(s, "eed") && measure(s.substr(0, s.length()-3)) > 0) return s.substr(0, s.length()-1);
        if ((ends_with(s, "ed") || ends_with(s, "ing")) && contains_vowel(s.substr(0, s.length()-2))) {
            s = s.substr(0, s.length() - (ends_with(s, "ed") ? 2 : 3));
            if (ends_with(s, "at") || ends_with(s, "bl") || ends_with(s, "iz")) s += "e";
            else if (s.length() > 1 && s.back() == s[s.length()-2] && !string("lsz").find(s.back()))
                s.pop_back();
            else if (measure(s) == 1 && cvc(s)) s += "e";
        }
        if (ends_with(s, "y") && contains_vowel(s.substr(0, s.length()-1)))
            return s.substr(0, s.length()-1) + "i";
        return s;
    }

    string step2(const string& s) {
        static const vector<pair<string, string>> rules = {
            {"ational","ate"}, {"tional","tion"}, {"enci","ence"}, {"anci","ance"},
            {"izer","ize"}, {"abli","able"}, {"alli","al"}, {"entli","ent"},
            {"eli","e"}, {"ousli","ous"}, {"ization","ize"}, {"ation","ate"},
            {"ator","ate"}, {"alism","al"}, {"iveness","ive"}, {"fulness","ful"},
            {"ousness","ous"}, {"aliti","al"}, {"iviti","ive"}, {"biliti","ble"}
        };
        for (const auto& p : rules)
            if (ends_with(s, p.first) && measure(s.substr(0, s.length()-p.first.length())) > 0)
                return s.substr(0, s.length()-p.first.length()) + p.second;
        return s;
    }

    string step3(const string& s) {
        static const vector<pair<string, string>> rules = {
            {"icate","ic"}, {"ative",""}, {"alize","al"}, {"iciti","ic"},
            {"ical","ic"}, {"ful",""}, {"ness",""}
        };
        for (const auto& p : rules)
            if (ends_with(s, p.first) && measure(s.substr(0, s.length()-p.first.length())) > 0)
                return s.substr(0, s.length()-p.first.length()) + p.second;
        return s;
    }

    string step4(const string& s) {
        static const vector<string> sufs = {"al","ance","ence","er","ic","able","ible","ant",
                                           "ement","ment","ent","ou","ism","ate","iti","ous","ive","ize"};
        for (const auto& suf : sufs)
            if (ends_with(s, suf) && measure(s.substr(0, s.length()-suf.length())) > 1)
                return s.substr(0, s.length()-suf.length());
        return s;
    }

    string step5(const string& s) {
        string res = s;
        if (res.back() == 'e') {
            string stem = res.substr(0, res.length()-1);
            if (measure(res) > 1 || (measure(res) == 1 && !cvc(stem)))
                res = stem;
        }
        if (res.length() > 1 && res.substr(res.length()-2) == "ll" && measure(res) > 1)
            res.pop_back();
        return res;
    }

    string strip_prefixes(string s) {
        static const vector<string> prefs = {"kilo","micro","milli","intra","ultra","mega","nano","pico","pseudo"};
        for (const auto& p : prefs)
            if (s.rfind(p, 0) == 0)
                return s.substr(p.length());
        return s;
    }
};

// Create a global stemmer
PorterStemmer stemmer;

// Load word to id mapping
void load_term_dictionary(const string& path) {
    ifstream f(path);
    string word; int id;
    while (f >> word >> id) word_index[word] = id;
}

// Load document id to document number mapping
void load_document_dictionary(const string& path) {
    ifstream f(path);
    string docno; int id;
    while (f >> docno >> id) doc_index[id] = docno;
}

// Load per document list of term frequencies
void load_forward_index(const string& path) {
    ifstream f(path);
    string line;
    while (getline(f, line)) {
        stringstream ss(line);
        string doc_str; getline(ss, doc_str, ':');
        int doc_id = stoi(doc_str);
        string token;
        while (getline(ss, token, ';')) {
            if (token.empty()) continue;
            size_t pos = token.find(':');
            if (pos == string::npos) continue;
            int tid = stoi(token.substr(0, pos));
            int tf = stoi(token.substr(pos+1));
            forward_index[doc_id][tid] = tf;
        }
    }
}

// Load inverted index mapping each term id to documents containing it
void load_inverted_index(const string& path) {
    ifstream f(path);
    string line;
    while (getline(f, line)) {
        stringstream ss(line);
        string term_str; getline(ss, term_str, ':');
        int tid = stoi(term_str);
        string token;
        while (getline(ss, token, ';')) {
            if (token.empty()) continue;
            size_t pos = token.find(':');
            if (pos == string::npos) continue;
            int doc_id = stoi(token.substr(0, pos));
            int tf = stoi(token.substr(pos+1));
            inverted_index[tid][doc_id] = tf;
        }
    }
}

// Load relevance judgments for evaluation
void load_relevance(const string& path) {
    ifstream f(path);
    string line;
    while (getline(f, line)) {
        stringstream ss(line);
        int qid; string skip, docno; int rel;
        ss >> qid >> skip >> docno >> rel;
        if (rel > 0) relevance[qid].insert(docno);
    }
}

// Load stopwords
void load_stopwords(const string& path) {
    ifstream f(path);
    string w;
    while (f >> w) stopwords.insert(w);
}

// Compute idf values for all terms
unordered_map<int, double> compute_idf(int N) {
    unordered_map<int, double> idf;
    for (const auto& p : inverted_index) {
        double df = p.second.size();
        idf[p.first] = log((N + 1.0) / (df + 1.0)) + 1.0;
    }
    return idf;
}

// Build a document vector using tf-idf weights
unordered_map<int, double> doc_vector(int doc_id, const unordered_map<int, double>& idf) {
    unordered_map<int, double> vec;
    for (const auto& p : forward_index[doc_id]) {
        double weight = (1.0 + log(p.second)) * idf.at(p.first);
        vec[p.first] = weight;
    }
    return vec;
}

// Build a query vector using tf-idf weights
unordered_map<int, double> query_vector(const vector<string>& terms, const unordered_map<int, double>& idf) {
    unordered_map<int, int> tf;
    for (const auto& term : terms) {
        auto it = word_index.find(term);
        if (it != word_index.end()) tf[it->second]++;
    }
    unordered_map<int, double> vec;
    for (const auto& p : tf) {
        double weight = (1.0 + log(p.second)) * idf.at(p.first);
        vec[p.first] = weight;
    }
    return vec;
}

// Compute cosine similarity between query vector and document vector
double cosine(const unordered_map<int, double>& qv, const unordered_map<int, double>& dv) {
    double dot = 0, qnorm = 0, dnorm = 0;
    for (const auto& p : qv) {
        qnorm += p.second * p.second;
        if (dv.count(p.first)) dot += p.second * dv.at(p.first);
    }
    for (const auto& p : dv) dnorm += p.second * p.second;
    if (qnorm == 0 || dnorm == 0) return 0.0;
    return dot / (sqrt(qnorm) * sqrt(dnorm));
}

// Tokenize and stem a query string
vector<string> tokenize_and_stem(const string& text) {
    vector<string> tokens;
    stringstream ss(text);
    string word;
    while (ss >> word) {
        for (char& c : word) c = tolower(c);
        string cleaned;
        for (char c : word) if (isalpha(c)) cleaned += c;
        if (cleaned.length() > 1 && stopwords.find(cleaned) == stopwords.end()) {
            string stemmed = stemmer.stem(cleaned);
            if (stemmed.length() > 1) tokens.push_back(stemmed);
        }
    }
    return tokens;
}

// Main program
int main() {
    load_term_dictionary("term_dictionary.txt");
    load_document_dictionary("document_dictionary.txt");
    load_forward_index("forward_index.txt");
    load_inverted_index("inverted_index.txt");
    load_relevance("main.qrels");
    load_stopwords("stopwordlist.txt");

    int N = doc_index.size();
    auto idf = compute_idf(N); // compute idf once

    ifstream topics("topics.txt");
    ofstream out_title("satish_title.txt");
    ofstream out_desc("satish_title_desc.txt");
    ofstream out_narr("satish_title_narr.txt");
    ofstream out_perf("satish_performance.txt");

    out_perf << "QueryID\tSetting\tPrecision\tRecall\n";

    string line;
    int qid = 0;
    string title, desc, narr;
    string current_field;

    while (getline(topics, line)) {
        if (line.find("<top>") != string::npos) {
            title = desc = narr = ""; // reset fields
        }
        else if (line.find("<num>") != string::npos) {
            size_t pos = line.find(":");
            if (pos != string::npos) qid = stoi(line.substr(pos+1)); // extract query id
        }
        else if (line.find("<title>") != string::npos) {
            title = line.substr(line.find(">") + 1);
            current_field = "title";
        }
        else if (line.find("<desc>") != string::npos) current_field = "desc";
        else if (line.find("<narr>") != string::npos) current_field = "narr";
        else if (line.find("</top>") != string::npos) {
            vector<tuple<string, string, ofstream*>> queries = {
                {"title", title, &out_title},
                {"title+desc", title + " " + desc, &out_desc},
                {"title+narr", title + " " + narr, &out_narr}
            };

            for (auto& q : queries) {
                string setting = get<0>(q);
                string qtext = get<1>(q);
                ofstream* outfile = get<2>(q);

                auto qterms = tokenize_and_stem(qtext);
                auto qvec = query_vector(qterms, idf);

                unordered_set<int> candidates;
                for (const auto& term : qterms) {
                    auto it = word_index.find(term);
                    if (it == word_index.end()) continue;
                    int tid = it->second;
                    auto inv_it = inverted_index.find(tid);
                    if (inv_it == inverted_index.end()) continue;
                    for (const auto& p : inv_it->second)
                        candidates.insert(p.first);
                }

                vector<pair<double, int>> results;
                for (int doc_id : candidates) {
                    auto dvec = doc_vector(doc_id, idf);
                    double score = cosine(qvec, dvec);
                    if (score > 0) results.emplace_back(score, doc_id);
                }

                sort(results.rbegin(), results.rend()); // sort descending
                int retrieved = 0;
                for (const auto& p : results) {
                    if (++retrieved > 1000) break;
                    string docno = doc_index[p.second];
                    *outfile << qid << "\t" << docno << "\t" << retrieved << "\t" << fixed << setprecision(6) << p.first << "\n";
                }

                int relevant_retrieved = 0;
                for (int i = 0; i < min(1000, (int)results.size()); ++i) {
                    string docno = doc_index[results[i].second];
                    if (relevance[qid].count(docno)) relevant_retrieved++;
                }

                double precision = results.empty() ? 0.0 : (double)relevant_retrieved / min(1000, (int)results.size());
                double recall = relevance[qid].empty() ? 0.0 : (double)relevant_retrieved / relevance[qid].size();

                out_perf << qid << "\t" << setting << "\t" << fixed << setprecision(4) << precision << "\t" << recall << "\n";

                cout << "Query " << qid << " " << setting << " P= " << precision << " R= " << recall << endl;
            }
        }
        else if (!line.empty() && current_field == "desc") desc += " " + line;
        else if (!line.empty() && current_field == "narr") narr += " " + line;
    }

    cout << "finished" << endl;
    return 0;
}
