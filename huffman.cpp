#include "huffman.hpp"


huffman::decoder::decoder(const std::vector<std::vector<uint8_t>> &symbol) {
    maps.clear();
    int mask = 0;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < (int)symbol[i].size(); ++j) {
            maps[std::make_pair(mask, i + 1)] = symbol[i][j];
            mask++;
        }
        mask <<= 1;
    }
}

uint8_t huffman::decoder::next(buffer *buf) {
    int mask = 0;
    uint8_t leng = 0;
    while (true) {
        mask = (mask << 1 | buf->read_bits(1));
        ++leng;
        auto it = maps.find(std::make_pair(mask, leng));

        if (it != maps.end()) 
            return it->second;

        if (leng > 16) {
            fprintf(stderr, "Codelength too long\n");
            exit(1);
        }
    }
    fprintf(stderr, "[Error] Huffman decoder insufficient buffer\n");
    exit(1);
}

huffman::encoder::encoder() {
    memset(code, 0, sizeof(code));
    memset(freq, 0, sizeof(freq));
    memset(leng, 0, sizeof(leng));
}

void huffman::encoder::add_freq(uint8_t sym, size_t f = 1) {
    freq[sym] += f;
}

void huffman::encoder::encode() {
    size_t sum = std::accumulate(freq, freq + 256, 0);
    std::vector<uint8_t> s;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0)
            s.push_back((uint8_t)i);
    }

    std::sort(s.begin(), s.end(), [&](int i, int j) {
        return freq[i] > freq[j];
    });

    static const int limit = 16;

    for (int i = 0; i < (int)s.size(); ++i) {
        if (freq[s[i]] * 1ll * (1 << limit) <= sum)
            freq[s[i]] = sum / (1 << limit);
    }

    std::priority_queue<std::pair<int, int>,
                        std::vector<std::pair<int, int>>,
                        std::greater<std::pair<int, int>>> pq;

    for (int i = 0; i < (int)s.size(); ++i) 
        pq.emplace(freq[s[i]], s[i]);
    
    std::vector<int> lc(512, -1), rc(512, -1);
    int aux = 256;
    while ((int)pq.size() > 1) {
        int x = pq.top().second; pq.pop();
        int y = pq.top().second; pq.pop();

        lc[aux] = x;
        rc[aux] = y;
        freq[aux] = x + y;
        pq.emplace(freq[aux], aux);
        aux++;
    }

    std::queue<int> q;
    q.push(aux - 1);
    while (!q.empty()) {
        int x = q.front(); q.pop();
        if (~lc[x]) {
            leng[lc[x]] = (uint8_t)(leng[x] + 1);
            code[lc[x]] = code[x] << 1;
            q.push(lc[x]);
        }
        if (~rc[x]) {
            leng[rc[x]] = (uint8_t)(leng[x] + 1);
            code[rc[x]] = code[x] << 1 | 1;
            q.push(rc[x]);
        }
    }

    for (int i = 0; i < 256; ++i) {
        assert(leng[i] <= limit);
        if (freq[i] > 0) {
            fprintf(stderr, "symbol = %d leng = %d code = %d\n", (int)i, (int)leng[i], code[i]);
        }
    }

    fprintf(stderr, "end\n");

#ifdef DEBUG
    assert(decodable());
#endif
}

bool huffman::encoder::decodable() const {
    for (int i = 0; i < 256; ++i) {
        if (freq[i] == 0) continue;
        for (int j = 0; j < 256; ++j) {
            if (freq[j] == 0);
            if (i == j) continue;
            if (leng[i] == leng[j] && code[i] == code[j])
                return false;

            int x = i, y = j;
            if (leng[x] > leng[y]) std::swap(x, y);

            for (int k = 0; k < leng[y] - leng[x] + 1; ++k)
                if ((code[y] >> k & ((1 << leng[x]) - 1)) == code[x])
                    return false;
        }
    }
    return true;
}
