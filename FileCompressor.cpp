
 #include <bits/stdc++.h>
using namespace std;

struct Node {
    char ch;
    int freq;
    Node *left, *right;
    Node(char c, int f) : ch(c), freq(f), left(NULL), right(NULL) {}
    Node(Node *l, Node *r) : ch('\0'), freq(l->freq + r->freq), left(l), right(r) {}
};

struct compare {
    bool operator()(Node *l, Node *r) {
        return l->freq > r->freq;
    }
};

// Free memory
void deleteTree(Node *root) {
    if (!root) return;
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

void buildCodes(Node *root, string str, unordered_map<char, string> &codes) {
    if (!root) return;
    if (!root->left && !root->right) {
        codes[root->ch] = str.empty() ? "0" : str; // Handle single char
    }
    buildCodes(root->left, str + "0", codes);
    buildCodes(root->right, str + "1", codes);
}

string encode(const string &text, unordered_map<char, string> &codes) {
    string encoded = "";
    for (char ch : text) encoded += codes[ch];
    return encoded;
}

string decode(Node *root, const string &encoded, int validBits) {
    if (!root) return "";
    string decoded = "";
    Node *curr = root;
    
    for (int i = 0; i < validBits; i++) {
        if (encoded[i] == '0') curr = curr->left;
        else curr = curr->right;
        
        if (!curr->left && !curr->right) {
            decoded += curr->ch;
            curr = root;
        }
    }
    return decoded;
}

// Serialize tree structure to file
void serializeTree(Node *root, ofstream &out) {
    if (!root) {
        out.put('N'); // Null marker
        return;
    }
    if (!root->left && !root->right) {
        out.put('L'); // Leaf marker
        out.put(root->ch);
    } else {
        out.put('I'); // Internal node marker
        serializeTree(root->left, out);
        serializeTree(root->right, out);
    }
}

// Deserialize tree from file
Node* deserializeTree(ifstream &in) {
    char marker;
    if (!in.get(marker)) return NULL;
    
    if (marker == 'N') return NULL;
    if (marker == 'L') {
        char ch;
        in.get(ch);
        return new Node(ch, 0);
    }
    // Internal node
    Node *left = deserializeTree(in);
    Node *right = deserializeTree(in);
    return new Node(left, right);
}

void compressFile(const string &inputFile, const string &outputFile) {
    ifstream in(inputFile);
    if (!in.is_open()) {
        cout << "Error: Cannot open input file.\n";
        return;
    }
    
    stringstream buffer;
    buffer << in.rdbuf();
    string text = buffer.str();
    in.close();
    
    if (text.empty()) {
        cout << "Error: Input file is empty.\n";
        return;
    }

    // Build frequency map
    unordered_map<char, int> freq;
    for (char ch : text) freq[ch]++;

    // Build Huffman tree
    priority_queue<Node*, vector<Node*>, compare> pq;
    for (auto p : freq) pq.push(new Node(p.first, p.second));
    
    Node *root = NULL;
    if (pq.size() == 1) {
        // Single character case
        root = new Node(pq.top(), NULL);
    } else {
        while (pq.size() > 1) {
            Node *left = pq.top(); pq.pop();
            Node *right = pq.top(); pq.pop();
            pq.push(new Node(left, right));
        }
        root = pq.top();
    }

    // Generate codes
    unordered_map<char, string> codes;
    buildCodes(root, "", codes);

    // Encode text
    string encoded = encode(text, codes);
    
    // Write to binary file
    ofstream out(outputFile, ios::binary);
    
    // Write tree structure
    serializeTree(root, out);
    
    // Write original text length (for verification)
    int textLen = text.size();
    out.write(reinterpret_cast<char*>(&textLen), sizeof(textLen));
    
    // Write number of valid bits in encoded data
    int validBits = encoded.size();
    out.write(reinterpret_cast<char*>(&validBits), sizeof(validBits));
    
    // Write encoded bits as bytes
    string byte = "";
    for (char bit : encoded) {
        byte += bit;
        if (byte.size() == 8) {
            bitset<8> b(byte);
            out.put(static_cast<unsigned char>(b.to_ulong()));
            byte = "";
        }
    }
    // Pad last byte if needed
    if (!byte.empty()) {
        while (byte.size() < 8) byte += '0';
        bitset<8> b(byte);
        out.put(static_cast<unsigned char>(b.to_ulong()));
    }
    
    out.close();
    deleteTree(root);

    cout << " Compression complete!\n";
    cout << "Original size: " << text.size() << " bytes (" << text.size() * 8 << " bits)\n";
    cout << "Compressed size: " << out.tellp() << " bytes\n";
    cout << "Encoded bits: " << encoded.size() << " bits\n";
    cout << "Compression ratio: " << fixed << setprecision(2)
         << (double)(text.size() * 8) / encoded.size() << "x\n";
}

void decompressFile(const string &inputFile, const string &outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in.is_open()) {
        cout << "Error: Cannot open input file.\n";
        return;
    }

    // Deserialize tree
    Node *root = deserializeTree(in);
    if (!root) {
        cout << "Error: Failed to read Huffman tree.\n";
        in.close();
        return;
    }

    // Read original text length
    int textLen;
    in.read(reinterpret_cast<char*>(&textLen), sizeof(textLen));
    
    // Read number of valid bits
    int validBits;
    in.read(reinterpret_cast<char*>(&validBits), sizeof(validBits));

    // Read encoded bits
    string bits = "";
    unsigned char c;
    while (in.read(reinterpret_cast<char*>(&c), 1)) {
        bitset<8> b(c);
        bits += b.to_string();
    }
    in.close();

    // Decode only valid bits
    string decoded = decode(root, bits, validBits);
    
    // Verify length
    if (decoded.size() != textLen) {
        cout << "Warning: Decoded length (" << decoded.size() 
             << ") doesn't match expected (" << textLen << ")\n";
    }

    // Write output
    ofstream out(outputFile);
    out << decoded;
    out.close();
    
    deleteTree(root);

    cout << " Decompression complete!\n";
    cout << "Output written to: " << outputFile << "\n";
    cout << "Decoded size: " << decoded.size() << " bytes\n";
}

int main() {
    int choice;
    cout << "==== Huffman File Compression Tool ====\n";
    cout << "1. Compress file\n";
    cout << "2. Decompress file\n";
    cout << "Enter choice: ";
    cin >> choice;

    if (choice == 1) {
        string inputFile, outputFile;
        cout << "Enter input file path: ";
        cin >> inputFile;
        cout << "Enter output file name (e.g., compressed.huf): ";
        cin >> outputFile;
        compressFile(inputFile, outputFile);
    } else if (choice == 2) {
        string inputFile, outputFile;
        cout << "Enter compressed file path: ";
        cin >> inputFile;
        cout << "Enter output file name (e.g., decompressed.txt): ";
        cin >> outputFile;
        decompressFile(inputFile, outputFile);
    } else {
        cout << "Invalid choice.\n";
    }

    return 0;
}