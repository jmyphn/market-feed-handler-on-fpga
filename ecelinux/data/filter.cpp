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

    // Maximum messages to write
    const uint64_t MAX_MESSAGES = 1'000'000;

    uint64_t countA=0, countE=0, countC=0, countX=0,
             countD=0, countU=0, total=0;   // countP=0

    while (total < MAX_MESSAGES) {
        unsigned char lenbuf[2];

        if (!reader.read(lenbuf, 2)) break;
        uint16_t L = (lenbuf[0] << 8) | lenbuf[1];

        vector<unsigned char> msg(L);
        if (!reader.read(msg.data(), L)) break;

        unsigned char type = msg[0];

        if (is_allowed(type)) {
            total++;

            if (type == 'A') countA++;
            if (type == 'E') countE++;
            if (type == 'C') countC++;
            if (type == 'X') countX++;
            if (type == 'D') countD++;
            if (type == 'U') countU++;
            // if (type == 'P') countP++;

            fout.write((char*)lenbuf, 2);
            fout.write((char*)msg.data(), L);
        }
    }

    cout << "AddOrder               (A): " << countA << endl;
    cout << "OrderExecuted          (E): " << countE << endl;
    cout << "OrderExecutedWithPrice (C): " << countC << endl;
    cout << "OrderCancel            (X): " << countX << endl;
    cout << "OrderDelete            (D): " << countD << endl;
    cout << "OrderReplace           (U): " << countU << endl;
    // cout << "Trade                  (P): " << countP << endl;
    cout << "TOTAL                     : " << total << endl;

    return 0;
}
