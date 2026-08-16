# Phase 2 — Indexer

Builds forward and inverted indexes, term/document dictionaries, and supports query testing.

```bash
g++ main.cpp -o indexer -std=c++17 -O2 -Wno-write-strings
./indexer ft911 stopwordlist.txt output testdata.txt
```
