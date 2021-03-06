#include <iostream>
#include <fstream>
#include <ctime> 
#include <map>
#include <unordered_map>
#include "Compressor.h"
#include <set>
#include <algorithm>
#include <unordered_set>


using namespace std;

void to_seq(uint64_t x, ofstream* ofs) //https://rosettacode.org/wiki/Variable-length_quantity#C
{
    uint8_t out[10];
    int i, j;
    for (i = 9; i > 0; i--) {
        if (x & 127ULL << i * 7) break;
    }
    for (j = 0; j <= i; j++)
        out[j] = ((x >> ((i - j) * 7)) & 127) | 128;

    out[i] ^= 128;

    i = 0;
    do
    {
        *ofs << out[i];
    } while ((out[i++] & 128));
}

uint64_t from_seq(uint8_t* in)
{
    uint64_t r = 0;

    do {
        r = (r << 7) | (uint64_t)(*in & 127);
    } while (*in++ & 128);

    return r;
}

int main(int argc, char *argv[]) 
{
    ofstream ofs("asd.f", ios::binary | ios::out);
    for (int i = 0; i < 300; i++)
    {
        to_seq(i, &ofs);
    }
    ofs.close();

    const int START_COMPARE = 14;
    const int COMPARES = 2;
    const int SHOW_FIRST = 100;
    const int MIN_HITS = 2;

    if (argc > 0) 
    {
        cout << "Opening " << argv[argc - 1] << endl;
        ifstream file(argv[argc - 1], ios::binary | ios::ate | ios::in);
        if (file) 
        {
            string filen = argv[argc - 1];
            cout << "Opened " << filen.substr(filen.find_last_of("\\") + 1) << endl;
            clock_t time = clock();
            streamsize size = file.tellg();
            file.seekg(0, ios::beg);

            char* f = new char [size];
            Key fKey;
  
            if (file.read(f, size))
            {
                cout << "Read in " << ((float)(clock() - time) / CLOCKS_PER_SEC * 1000) << "ms" << endl;
                file.close();

                int totals[COMPARES];
                int totalTotals[COMPARES];
                unordered_map<Key, int> resultsMap[COMPARES];
                vector<pair<Key, int>> results[COMPARES];
                for (int i = START_COMPARE; i < START_COMPARE + COMPARES; i++)
                {
                    int found = 0;
                    int notfound = 0;
                    unordered_map<Key, int> pattern;
                    fKey.size = i;
                    for (int x = 0; x < size - i;) 
                    {
                        fKey.array = &f[x];
                        unordered_map<Key, int>::iterator it = pattern.find(fKey);
                        if (it != pattern.end())
                        {
                            it->second++;
                            x += i;
                            found++;
                        }
                        else 
                        {
                            Key insert;
                            insert.array = &f[x];
                            insert.size = i;
                            pattern.insert({ insert, 1 });
                            notfound++;
                            x++;
                        }

                        if (x % 1000 == 0)
                        {
                            cout << "finding for " << i << " (" << (i - START_COMPARE + 1) << "/" << COMPARES << "): " << ((int)((((double)x / size) * 1000)) / 10.0) << "% - " << x << "/" << size << " - " << found << " found, " << notfound << " not found" << "\r";
                        }
                    }
                    vector<pair<Key, int>> sorted;
                    for (pair<Key, int> entry : pattern)
                    {
                        if (entry.second >= MIN_HITS)
                        {
                            sorted.push_back(entry);
                        }
                    }

                    totals[i - START_COMPARE] = sorted.size();
                    int totalTotal = 0;
                    for (pair<Key, int> entry : sorted)
                    {
                        totalTotal += entry.second;
                    }
                    totalTotals[i - START_COMPARE] = totalTotal;
                    cout << "\r\nfound " << found << ", did not find " << notfound << ", actually found " << sorted.size() << " unique and " << totalTotal << " total " << endl;

                    sort(sorted.begin(), sorted.end(), 
                        [](pair<Key, int> a, pair<Key, int> b)
                        {
                            return a.second > b.second;
                        });
                    
                    if (sorted.size() > SHOW_FIRST)
                    {
                        sorted.resize(SHOW_FIRST);
                    }

                    reverse(sorted.begin(), sorted.end());

                    unordered_map<Key, int> p(sorted.begin(), sorted.end());
                    results[i - START_COMPARE] = sorted;
                    resultsMap[i - START_COMPARE] = p;
                }
                cout << "Finished after " << ((float)(clock() - time) / CLOCKS_PER_SEC) << " seconds" << endl;
                for (int i = 0; i < COMPARES; i++)
                {
                    for (pair<Key, int> pair : results[i])
                    {
                        cout << i + START_COMPARE << "=>" << pair.second << " ";

                        for (int x = 0; x < pair.first.size; x++)
                        {
                            printf("%x", pair.first.array[x] & 0xFF);
                            cout << " ";
                        }

                        for (int x = 0; x < pair.first.size; x++)
                        {
                            printf("%c", (pair.first.array[x] & 0xff));
                        }

                        cout << endl;
                    }
                }
                for (int i = 0; i < COMPARES; i++)
                {
                    cout << i + START_COMPARE << ": " << totals[i] << " unique, " << totalTotals[i] << " total" << endl;
                }
                ofstream outfile;
                string name = filen.substr(filen.find_last_of("\\") + 1);
                string nameName = name.substr(0, name.find_last_of(".")) + ".compressed";
                outfile.open(nameName, ios::out | ios::binary | ios::trunc);
                if (outfile)
                {
                    cout << "Created " << nameName << endl;
                    vector<pair<Key, int>> things;
                    unordered_map<Key, int> usedKeys[COMPARES];
                    for (int i = 0; i < size; i++)
                    {
                        Key match;
                        match.array = &f[i];
                        for (int x = COMPARES - 1; x >= 0; x--)
                        {
                            match.size = START_COMPARE + x;

                            unordered_map<Key, int>::iterator it = resultsMap[x].find(match);
                            if (it != resultsMap[x].end())
                            {
                                things.push_back(make_pair(match, i));
                                unordered_map<Key, int>::iterator it = usedKeys[x].find(match);
                                if (it != usedKeys[x].end())
                                {
                                    it->second++;;
                                }
                                else
                                {
                                    usedKeys[x].insert({ match, 1 });
                                }
                                i += x;
                                break;
                            }
                        }
                        if (i % 1000 == 0)
                        {
                            cout << "first pass: " <<i << "/" << size << "\r";
                        }
                    }
                    int bytesSaved = 0;
                    for (int i = 0; i < COMPARES; i++)
                    {
                        cout << i + START_COMPARE << "=>";
                        for (pair<Key, int> entry : usedKeys[i])
                        {
                            bytesSaved += entry.first.size * entry.second;
                            cout << entry.second << ", ";
                        }
                        cout << endl;
                    }
                    
                    cout << "Done with " << bytesSaved << endl;
                    unordered_map<Key, int> indexMap;
                    int x = 0;
                    for (int i = 0; i < COMPARES; i++)
                    {
                        to_seq(i, &outfile);
                        to_seq(usedKeys[i].size(), &outfile);
                        for (pair<Key, int> entry : usedKeys[i])
                        {
                            indexMap.insert({ entry.first, x++ });
                            outfile << entry.first.size;
                            for (int x = 0; x < entry.first.size; x++)
                            {
                                outfile << entry.first.array[x];
                            }
                        }
                    }
                    int pos = 0;
                    for (pair<Key, int> entry : things)
                    {
                        to_seq(entry.second-pos, &outfile);
                        outfile.write(&f[pos], entry.second-pos);
                        unordered_map<Key, int>::iterator it = indexMap.find(entry.first);
                        if (it != indexMap.end())
                        {
                            to_seq(it->second, &outfile);
                        }
                        else
                        {
                            cout << "oopsie poopsie" << endl;
                        }
                        pos = entry.second;
                    }
                    outfile.close();
                }
                else
                {
                    cout << "Could not create " << nameName << endl;
                }
            }
            else
            {
                cout << "Error opening " << filen << endl;
                return 1;
            }

            delete[] f;
        }
        else
        {
            cout << "Error opening " << argv[argc - 1] << endl;
            return 1;
        }
    }
    else 
    {
        cout << "Not enough arguments - please specify a file to open" << endl;
    }

    return 0;
}
