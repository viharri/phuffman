#include "CodeTableAdapter.hpp"
#include "constants.h"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <ostream>

namespace phuffman {

    bool _LeafComparator(const DepthCounterNode* left, const DepthCounterNode* right) {
        // Compare nodes first by codelength then by value
        return ((left->depth > right->depth) ||
                ((left->depth == right->depth) && (left->element < right->element)));
    }

    bool _FrequencyComparator(const Frequency& left, const Frequency& right) {
        return left.frequency < right.frequency;
    }

    CodeTableAdapter::CodeTableAdapter(const char *file_data, size_t size) {
        assert(size == ALPHABET_SIZE);

        memset(_adaptee.codes, 0, sizeof(Code)*ALPHABET_SIZE);

        unsigned char lengths[ALPHABET_SIZE];
        for (size_t i=0; i<size; ++i) {
            lengths[i] = file_data[i];
        }

        Nodes leaves;
        for (size_t i=0; i<ALPHABET_SIZE; ++i) {
            if (lengths[i] > 0) {
                assert(lengths[i] < MAXIMUM_CODELENGTH);
                DepthCounterNode* leaf = new DepthCounterNode(i);
                leaf->depth = lengths[i];
                leaves.push_back(leaf);
            }
        }

        // Leaves contain code length for each symbol in data
        sort(leaves.begin(), leaves.end(), _LeafComparator);

        _buildTable(leaves);

        Nodes::const_iterator first = leaves.begin(), last = leaves.end();
        while (first != last) {
            delete *first;
            ++first;
        }
    }

    CodeTableAdapter::CodeTableAdapter(const unsigned char *data, size_t size) {
        assert(size <= MAXIMUM_DATABLOCK_SIZE);

        memset(_adaptee.codes, 0, sizeof(Code)*ALPHABET_SIZE);

        Tree tree;
        Nodes leaves;

        // Build huffman tree
        Frequencies frequencies = _countFrequencies(data, size);
        Frequencies::const_iterator first = frequencies.begin(), last = frequencies.end();
        while (first != last) {
            DepthCounterNode* leaf = new DepthCounterNode(first->symbol);
            tree.insert(std::make_pair(first->frequency, leaf));
            leaves.push_back(leaf);
            ++first;
        }

        DepthCounterNode* root = NULL;
        if (tree.size() > 1) {
            root = _buildTree(&tree);
        }
        else {
            root = tree.begin()->second;
            assert(root->isLeaf);
            root->depth = 1;
        }

        // Leaves contain code length for each symbol in data
        sort(leaves.begin(), leaves.end(), _LeafComparator);

        // Build codes table
        _buildTable(leaves);

        delete root;
    }

    CodeTableAdapter::~CodeTableAdapter() {}

    CodeTableInfo CodeTableAdapter::info() const {
        return _adaptee.info;
    }

    Code CodeTableAdapter::operator[](size_t index) const {
        return at(index);
    }

    Code CodeTableAdapter::at(size_t index) const {
        assert(index < ALPHABET_SIZE);
        return _adaptee.codes[index];
    }

    const CodeTable* CodeTableAdapter::c_table() const {
        return &_adaptee;
    }

    CodeTableAdapter::Frequencies CodeTableAdapter::_countFrequencies(const unsigned char* data, size_t size) const {
        size_t freqs[ALPHABET_SIZE] = {0};
        for (size_t i=0; i<size; ++i) {
            unsigned char symbol = data[i];
            ++freqs[symbol];
        }
        Frequencies frequencies;
        frequencies.reserve(ALPHABET_SIZE);
        for (size_t i=0; i<ALPHABET_SIZE; ++i) {
            if (freqs[i] > 0) {
                frequencies.push_back(FrequencyMake(i, freqs[i]));
            }
        }
        std::sort(frequencies.begin(), frequencies.end(), _FrequencyComparator);

        return frequencies;
    }

    DepthCounterNode* CodeTableAdapter::_buildTree(Tree *tree) const {
        // 1. Get two rarest element
        // 2. Create new node that points to these elements and has sum of their frequencies
        // 3. Repeat until tree size is equal to 1
        for (size_t i=0, size=tree->size(); i<size-1; ++i) {
            Tree::iterator first = tree->begin(), second = tree->begin();
            ++second;
            size_t freq = first->first + second->first; // Calculate frequency for a node
            DepthCounterNode* node = new DepthCounterNode(first->second, second->second);
            ++second;
            tree->erase(first, second); // Remove two nodes with the smallest frequency
            tree->insert(std::make_pair(freq, node)); // Add node that points to previously removed nodes
        }
        assert(tree->size() == 1);
        return tree->begin()->second;
    }

    void CodeTableAdapter::_buildTable(const Nodes& leaves) {
        Nodes::const_iterator current_node = leaves.begin(), lastNode = leaves.end();
        // First longest element always has 0 code
        _adaptee.info.max_codelength = (*current_node)->depth;
        Code last_code = CodeMake((*current_node)->depth, 0);
        _adaptee.codes[(*current_node)->element] = last_code;
        ++current_node;

        while (current_node != lastNode) {
            // If current codeword and next codeword have equal lengths
            if ((*current_node)->depth == last_code.codelength) {
                // Just increase codeword by 1
                last_code.code += 1;
            }
            // Otherwise
            else {
                // We are iterating from longest to shortest code lengths
                assert(last_code.codelength > (*current_node)->depth);
                // Increase codeword by 1 and _after_ that shift codeword right
                last_code.code = (last_code.code + 1) >> (last_code.codelength - (*current_node)->depth);
            }
            last_code.codelength = (*current_node)->depth;
            assert(last_code.codelength < MAXIMUM_CODELENGTH);
            _adaptee.codes[(*current_node)->element] = last_code;
            ++current_node;
        }
    }

    std::ostream& operator<<(std::ostream& os, const CodeTableAdapter& table) {
        for (size_t i=0, size=ALPHABET_SIZE; i<size; ++i) {
            os << table[i].codelength;
        }
        return os;
    }

    bool operator==(const CodeTableAdapter& left, const CodeTableAdapter& right) {
		const CodeTable* left_table = left.c_table();
		const CodeTable* right_table = right.c_table();
		return (memcmp(left_table->codes, right_table->codes, sizeof(Code)*ALPHABET_SIZE) == 0);
    }

}
