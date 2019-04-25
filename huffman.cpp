#include "huffman.hpp"


huffman::decoder::decoder(const std::array<std::vector<uint8_t>, 16> &symbol) {
    // leng = std::array<uint8_t, 1 << 16>{};
    // code = std::array<uint8_t, 1 << 16>{};
    int mask = 0;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < (int)symbol[i].size(); ++j) {
            leng[mask] = (uint8_t)(i + 1);
            code[mask] = symbol[i][j];
            mask++;
        }
        mask <<= 1;
    }
}

uint8_t huffman::decoder::next(buffer *buf) {
    int mask = 0;
    uint8_t len = 0;
    while (true) {
        mask = (mask << 1 | buf->read_bits(1));
        ++len;

        if (len == leng[mask])
            return code[mask];

        if (len > 16) {
            fprintf(stderr, "Codelength too long\n");
            exit(1);
        }
    }
    fprintf(stderr, "[Error] Huffman decoder insufficient buffer\n");
    exit(1);
}


huffman::encoder::encoder(uint8_t id): id(id) {}

void huffman::encoder::add_freq(uint8_t sym, size_t f = 1) {
    freq[sym] += f;
}

// TODO: should refactor
void huffman::encoder::encode() {
    std::vector<std::pair<size_t, uint8_t>> symb = {{1, 0}};
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0)
            symb.emplace_back(freq[i], i + 1);
    }
    
    std::sort(symb.begin(), symb.end());

    symb[0].first += symb[1].first;
    int n = (int)symb.size();
    int root = 0, leaf = 2;

    for (int i = 1; i < n - 1; ++i) {
        if (leaf >= n || symb[root].first < symb[leaf].first) {
            symb[i].first = symb[root].first;
            symb[root++].first = i; 
        } else {
            symb[i].first += symb[leaf++].first;
        }
        if (leaf >= n || (root < i && symb[root].first < symb[leaf].first)) {
            symb[i].first += symb[root].first;
            symb[root++].first = i;
        } else {
            symb[i].first += symb[leaf++].first;
        }
    }
    symb[n - 2].first = 0;
    for (int i = n - 3; i >= 0; --i) {
        symb[i].first = symb[symb[i].first].first + 1;
    }

    int spac = 1, used = 0, dep = 0, nxt = n - 1;
    root = n - 2;

    while (spac > 0) {
        while (root >= 0 && (int)symb[root].first == dep) {
            used++;
            root--;
        }
        while (spac > used) {
            symb[nxt--].first = dep;
            spac--;
        }
        spac = 2 * used;
        dep++;
        used = 0;
    }

    std::array<size_t, 257> cnt{}; 
    for (int i = 0; i < (int)symb.size(); ++i)
        cnt[symb[i].first]++;

    static const size_t limit = 16;
    if ((int)symb.size() > 1) {
        for (int i = (int)limit + 1; i < 257; ++i)
            cnt[limit] += cnt[i];

        size_t sum = 0;
        for (int i = (int)limit; i > 0; --i) 
            sum += ((size_t)cnt[i] << (limit - i));

        while (sum != (1ull << limit)) {
            cnt[limit]--;
            for (int i = (int)limit - 1; i > 0; --i) {
                if (cnt[i]) {
                    cnt[i]--;
                    cnt[i + 1] += 2;
                    break;
                }
            }
            sum--;
        }
    }

    std::vector<uint8_t> v;
    for (int i = (int)symb.size() - 1; i >= 1; --i) 
        if (symb[i].second > 0)
            v.push_back((uint8_t)(symb[i].second - 1));

    for (int i = (int)limit; i >= 1; --i) {
        if (cnt[i] > 0) {
            cnt[i] -= 1;
            break;
        }
    }

    int mask = 0, j = 0;
    for (int i = 1; i <= (int)limit; ++i) {
        for (int k = 0; k < (int)cnt[i]; ++k, ++j) {
            assert(j < (int)v.size());
            tab[i - 1].push_back(v[j]);
        }

        std::sort(tab[i - 1].begin(), tab[i - 1].end());
        for (int k = 0; k < (int)tab[i - 1].size(); ++k) {
            leng[tab[i - 1][k]] = (uint8_t)i;
            code[tab[i - 1][k]] = mask++;
        }
        mask <<= 1;
    }

#ifdef DEBUG
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < (int)tab[i].size(); ++j) {
            fprintf(stderr, "%d -> ", (int)tab[i][j]);
            for (int k = (int)leng[tab[i][j]] - 1; k >= 0; --k)
                fprintf(stderr, "%d", (code[tab[i][j]] >> k & 1));
            fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "exit\n");
    assert(decodable());
#endif
}

bool huffman::encoder::decodable() const {
    for (int i = 0; i < 256; ++i) {
        if (freq[i] == 0) continue;
        for (int j = 0; j < 256; ++j) {
            if (freq[j] == 0) continue;
            if (i == j) continue;
            if (leng[i] == leng[j] && code[i] == code[j])
                return false;

            int x = i, y = j;
            if (leng[x] > leng[y]) std::swap(x, y);

            if ((code[y] >> (leng[y] - leng[x]) & ((1 << leng[x]) - 1)) == code[x]) {
#ifdef DEBUG
                fprintf(stderr, "x = %d y = %d\n", x, y);
#endif
                return false;
            }
        }
    }
    return true;
}
