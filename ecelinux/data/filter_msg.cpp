#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <zlib.h>

using namespace std;

bool is_allowed(unsigned char t) {
    return t == 'A' || t == 'E' || t == 'C' || t == 'X' ||
           t == 'D' || t == 'U';
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
    if (argc < 4) {
        std::cerr << "Usage: ./filter <input> <output> <TYPE>\n";
        return 1;
    }

    const char* input_path  = argv[1];
    const char* output_path = argv[2];
    unsigned char WANTED = argv[3][0];   // <-- Desired message type

    if (!is_allowed(WANTED)) {
        std::cerr << "Error: type must be A/E/C/X/D/U\n";
        return 1;
    }

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

    cout << "Searching for ONE message of type '" << WANTED << "'...\n";

    while (!reader.eof()) {

        unsigned char lenbuf[2];
        if (!reader.read(lenbuf, 2)) break;

        uint16_t L = (lenbuf[0] << 8) | lenbuf[1];

        vector<unsigned char> msg(L);
        if (!reader.read(msg.data(), L)) break;

        unsigned char type = msg[0];
        if (type != WANTED) continue;

        // Found the one we want — write and exit
        fout.write((char*)lenbuf, 2);
        fout.write((char*)msg.data(), L);

        cout << "Wrote 1 message of type " << WANTED << " with length " << L << "\n";
        return 0;
    }

    cout << "No message of type '" << WANTED << "' found.\n";
    return 0;
}
