#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <zlib.h>

using namespace std;

bool is_allowed(unsigned char t) {
    return t == 'A' || t == 'E' || t == 'C' || t == 'X' ||
           t == 'D' || t == 'U';    // t == 'P'
}

class Reader {
public:
    bool gz_mode;
    ifstream fin;
    gzFile gzf;

    Reader(const char* path) {
        size_t len = strlen(path);
        gz_mode = (len > 3 && strcmp(path + len - 3, ".gz") == 0);

        if (gz_mode) {
            gzf = gzopen(path, "rb");
        } else {
            fin.open(path, ios::binary);
        }
    }

    bool good() const {
        return gz_mode ? (gzf != nullptr) : (bool)fin;
    }

    bool read(void* buf, size_t n) {
        if (gz_mode) {
            int r = gzread(gzf, buf, n);
            return (r == (int)n);
        } else {
            fin.read((char*)buf, n);
            return (bool)fin;
        }
    }

    bool eof() const {
        if (gz_mode)
            return gzeof(gzf);
        return fin.eof();
    }

    ~Reader() {
        if (gz_mode && gzf) gzclose(gzf);
        if (!gz_mode && fin.is_open()) fin.close();
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: ./filter <input or input.gz> <output>\n";
        return 1;
    }

    const char* input_path  = argv[1];
    const char* output_path = argv[2];

    Reader reader(input_path);
    if (!reader.good()) {
        cerr << "Error: cannot open input file " << input_path << "\n";
        return 1;
    }

    ofstream fout(output_path, ios::binary);
    if (!fout) {
        cerr << "Error: cannot create output file " << output_path << "\n";
        return 1;
    }

    // We want the first 2 of each type: A,E,C,X,D,U
    const uint64_t LIMIT_PER_TYPE = 2;

    uint64_t countA=0, countE=0, countC=0, countX=0,
             countD=0, countU=0;
    uint64_t total = 0;

    auto all_done = [&]() {
        return countA >= LIMIT_PER_TYPE &&
               countE >= LIMIT_PER_TYPE &&
               countC >= LIMIT_PER_TYPE &&
               countX >= LIMIT_PER_TYPE &&
               countD >= LIMIT_PER_TYPE &&
               countU >= LIMIT_PER_TYPE;
    };

    while (!all_done()) {
        unsigned char lenbuf[2];

        if (!reader.read(lenbuf, 2)) break;
        uint16_t L = (lenbuf[0] << 8) | lenbuf[1];

        vector<unsigned char> msg(L);
        if (!reader.read(msg.data(), L)) break;

        unsigned char type = msg[0];

        if (!is_allowed(type)) continue;

        // Decide whether we should keep this message
        bool keep = false;
        switch (type) {
            case 'A': keep = (countA < LIMIT_PER_TYPE); break;
            case 'E': keep = (countE < LIMIT_PER_TYPE); break;
            case 'C': keep = (countC < LIMIT_PER_TYPE); break;
            case 'X': keep = (countX < LIMIT_PER_TYPE); break;
            case 'D': keep = (countD < LIMIT_PER_TYPE); break;
            case 'U': keep = (countU < LIMIT_PER_TYPE); break;
        }

        if (!keep) continue;

        // Count + write
        switch (type) {
            case 'A': countA++; break;
            case 'E': countE++; break;
            case 'C': countC++; break;
            case 'X': countX++; break;
            case 'D': countD++; break;
            case 'U': countU++; break;
        }
        total++;

        fout.write((char*)lenbuf, 2);
        fout.write((char*)msg.data(), L);
    }

    cout << "AddOrder               (A): " << countA << endl;
    cout << "OrderExecuted          (E): " << countE << endl;
    cout << "OrderExecutedWithPrice (C): " << countC << endl;
    cout << "OrderCancel            (X): " << countX << endl;
    cout << "OrderDelete            (D): " << countD << endl;
    cout << "OrderReplace           (U): " << countU << endl;
    cout << "TOTAL                     : " << total << endl;


    return 0;
}
