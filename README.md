# Information Retrieval System in C++

A three-phase information retrieval project implementing a complete text-processing and retrieval pipeline over a TREC-style document collection. The project covers parsing and normalization, forward/inverted indexing, TF-IDF weighting, cosine-similarity retrieval, and precision/recall evaluation.

## Project Overview

This repository combines three progressively built phases:

### Phase 1 — Text Parser
- Parses TREC-style `<DOC>` / `<DOCNO>` documents.
- Tokenizes alphabetic terms.
- Converts tokens to lowercase.
- Removes stopwords.
- Applies Porter-style stemming.
- Builds term and document dictionaries.

### Phase 2 — Indexer
- Processes the document collection into searchable index structures.
- Builds a forward index mapping documents to term frequencies.
- Builds an inverted index mapping terms to matching documents and frequencies.
- Writes term and document dictionaries.
- Supports interactive and file-based term queries.

### Phase 3 — Retrieval & Evaluation
- Loads the generated dictionaries and indexes.
- Processes batch TREC-style topics.
- Builds TF-IDF document and query vectors.
- Ranks documents using cosine similarity.
- Compares three query formulations:
  - title
  - title + description
  - title + narrative
- Evaluates results using relevance judgments and reports precision and recall.

## Technologies

- C++17
- Standard Template Library (STL)
- `std::filesystem`
- Regular expressions
- Porter-style stemming
- TF-IDF
- Cosine similarity
- TREC-style topics and relevance judgments

## Repository Structure

```text
Information-Retrieval-System/
├── phase1-parser/
│   ├── parser.cpp
│   ├── PorterStemmer.hpp
│   ├── stopwordlist.txt
│   └── parser_output.txt
├── phase2-indexer/
│   ├── main.cpp
│   ├── indexer.h
│   ├── stemmer.h
│   ├── stopwordlist.txt
│   ├── testdata.txt
│   ├── testdata_output.txt
│   └── output/
├── phase3-retrieval/
│   ├── final.cpp
│   ├── topics.txt
│   ├── main.qrels
│   ├── document_dictionary.txt
│   ├── term_dictionary.txt
│   ├── forward_index.txt
│   ├── inverted_index.txt
│   ├── stopwordlist.txt
│   └── results/
└── docs/
    ├── phase2-report.docx
    └── phase3-report.pdf
```

## Build

All three phases were verified to compile with `g++` using C++17.

### Phase 1

```bash
cd phase1-parser
g++ -std=c++17 parser.cpp -o parser
./parser
```

The program expects the document collection to be available in a local `ft911/` directory.

### Phase 2

```bash
cd phase2-indexer
g++ main.cpp -o indexer -std=c++17 -O2 -Wno-write-strings
./indexer ft911 stopwordlist.txt output testdata.txt
```

### Phase 3

```bash
cd phase3-retrieval
g++ -std=c++17 final.cpp -o retrieval
./retrieval
```

## Retrieval Evaluation

Phase 3 evaluates multiple query representations using the supplied relevance judgments. Example results from the project include:

| Query | Setting | Precision | Recall |
|---|---|---:|---:|
| 352 | title | 0.0020 | 0.0385 |
| 353 | title | 0.0543 | 0.3182 |
| 353 | title + description | 0.0080 | 0.3636 |
| 353 | title + narrative | 0.0100 | 0.4545 |
| 354 | title | 0.0155 | 0.2800 |
| 354 | title + description | 0.0080 | 0.3200 |
| 359 | title | 0.0011 | 0.1667 |

The complete measurements are available in `phase3-retrieval/results/satish_performance.txt`.

## Dataset Note

The original `ft911` corpus is intentionally not included in this public-ready repository. The code expects the dataset to be placed in an `ft911/` directory when reproducing Phases 1 and 2. This avoids redistributing course or third-party source material whose public redistribution rights may be restricted.

## What This Project Demonstrates

- Information retrieval fundamentals
- Text preprocessing and normalization
- Dictionary construction
- Forward and inverted indexing
- Term-frequency statistics
- TF-IDF vector-space modeling
- Cosine-similarity ranking
- Batch query processing
- Precision/recall evaluation
- C++ data structures and file processing

## Author

Satish Vepuri
