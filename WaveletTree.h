
#ifndef __WAVELET_TREE_H__
#define __WAVELET_TREE_H__

#include "DynamicBitvector.h"
#include <vector>
#include <iostream>

using std::vector;
typedef uint32_t ui32;

template<typename T, ui32 B>
class WaveletTree {

private:
    bool reserved;

    vector<vector<DynamicBitvector<B>>> layers;
    vector<vector<T>> leaf_values;

    ui32 alph_size;
    ui32 max_depth;
    ui32 size;

    ui32 range_raw(const vector<T> &p_leaf, const ui32 &index_l, const ui32 &index_r, const T &L, const T &U) const {
        ui32 count = 0;
        for (ui32 i = index_l; i < index_r; i++)
            if (L <= p_leaf[i] && p_leaf[i] < U)
                count++;
        return count;
    }
    static inline T middle(const T &a, const T &b) {
        return (a + b) / 2;
    }

    static inline bool valid_alph_size(const ui32 &_alph_size) { return _alph_size > 1; };
    static inline bool valid_max_depth(const ui32 &_max_depth) { return _max_depth > 0; };
    static inline ui32 child_left(const ui32 &index) { return 2 * index; };
    static inline ui32 child_right(const ui32 &index) { return 2 * index + 1; };

public:

    WaveletTree() : reserved(false), layers(), alph_size(0), max_depth(0), size(0) {};

    void set_alph_size(const ui32 &new_alph_size) {
        if (!layers.empty())
            throw std::logic_error("Cannot change the alphabet size after construction");
        if (!valid_alph_size(new_alph_size))
            throw std::logic_error("The alphabet size is not valid!");
        alph_size = new_alph_size;
    }
    ui32 get_alph_size() const {
        return alph_size;
    }
    void set_max_depth(const ui32 &new_max_depth) {
        if (!layers.empty())
            throw std::logic_error("Cannot change the maximum depth after construction");
        if (!valid_max_depth(new_max_depth))
            throw std::logic_error("The depth is not valid!");

        max_depth = new_max_depth;
    }
    void set_max_depth_leaf(const ui32 &new_size, const ui32 &min_leaf_size) {
        ui32 node_size = new_size;
        ui32 i = 1;
        while (node_size > min_leaf_size) {
            i++;
            node_size = (node_size + 1) / 2;
        }
        set_max_depth(i);
    }
    ui32 get_max_depth() const {
        return max_depth;
    }

    void reserve(const ui32 &new_size, const ui32 &decay_factor) {
        if (decay_factor < 1)
            throw std::invalid_argument("The decay factor must be at least 1!");
        if (!valid_max_depth(max_depth))
            throw std::logic_error("The maximum depth is not initialized! Call set_max_depth to set the depth.");
        if (!valid_alph_size(alph_size))
            throw std::logic_error("The alphabet size is not initialized! Call set_alph_size to set the alphabet size.");

        if (size > 0 || !layers.empty() || !leaf_values.empty())
            throw std::logic_error("The internal memory has already been reserved! Please clear it before reserving again.");

        reserved = true;
        layers = vector<vector<DynamicBitvector<B>>>(max_depth + 1);

        ui32 layer_size = 1;
        ui32 layer_width = new_size;
        for (ui32 i = 0; i <= max_depth; i++) {
            vector<DynamicBitvector<B>> &cur_layer = layers[i];
            cur_layer = vector<DynamicBitvector<B>>(layer_size);
            for (ui32 j = 0; j < layer_size; j++)
                cur_layer[j].reserve(layer_width);

            layer_size = 2 * layer_size;
            layer_width = (layer_width + decay_factor - 1) / decay_factor;
        }

        leaf_values = vector<vector<T>>(layer_size);
        for (ui32 i = 0; i < layer_size; i++) {
            vector<T> &cur_leaf = leaf_values[i];
            cur_leaf.reserve(layer_width);
        }
    }

    void print_layer(ui32 i) const {
        std::ostream &s = std::cout;
        for (ui32 j = 0; j < layers[i].size(); j++) {
            layers[i][j].print();
            s << ",";
        }
        s << std::endl;
    }
    void print() const {
        std::ostream &s = std::cout;
        s << "------------------------begin tree------------------------";
        s << std::endl;
        for (ui32 i = 0; i < max_depth; i++) {
            print_layer(i);
            s << std::endl;
        }
        for (ui32 i = 0; i < leaf_values.size(); i++) {
            if (leaf_values[i].size() > 0)
                s << leaf_values[i][0];
            for (ui32 j = 1; j < leaf_values[i].size(); j++)
                s << "," << leaf_values[i][j];
            s << "|";
        }
        s << std::endl;
        s << "------------------------end tree------------------------"; 
        s << std::endl;
    }
    void create_array(const T *p_new_values, const ui32 new_size) {
        if (max_depth == 0)
            throw std::logic_error("The maximum depth is not initialized! Call set_max_depth to set the depth.");
        if (alph_size <= 1)
            throw std::logic_error("The alphabet size is not initialized! Call set_alph_size to set the alphabet size.");

        if (!reserved)
            reserve(new_size, 1);

        vector<ui32> layer_a, layer_b;
        vector<vector<T>> layer_vals(1);

        vector<T> &orig_vals = layer_vals[0];
        orig_vals = vector<T>(new_size);
        for (ui32 i = 0; i < new_size; i++)
            orig_vals[i] = p_new_values[i];
        layer_a.push_back(0);
        layer_b.push_back(alph_size);

        for (ui32 i = 0; i <= max_depth; i++) {
            ui32 layer_size = static_cast<ui32>(layer_vals.size());
            vector<ui32> cur_a(layer_a.begin(), layer_a.end());
            vector<ui32> cur_b(layer_b.begin(), layer_b.end());
            vector<vector<T>> cur_vals(layer_vals.begin(), layer_vals.end());

            layer_a.clear();
            layer_b.clear();
            layer_vals.clear();
            layer_a.reserve(2 * layer_size);
            layer_b.reserve(2 * layer_size);
            layer_vals.reserve(2 * layer_size);
            for (ui32 j = 0; j < layer_size; j++) {
                ui32 a = cur_a[j];
                ui32 b = cur_b[j];
                ui32 m = middle(a, b);

                vector<T> cur_list = cur_vals[j];

                layers[i][j].init_size(static_cast<ui32>(cur_list.size()));

                layer_a.push_back(a);
                layer_a.push_back(m);
                layer_b.push_back(m);
                layer_b.push_back(b);
                layer_vals.push_back(vector<T>());
                layer_vals.push_back(vector<T>());

                vector<T> &leaf_left = layer_vals[layer_vals.size() - 2];
                vector<T> &leaf_right = layer_vals[layer_vals.size() - 1];
                leaf_left.reserve(cur_list.size());
                leaf_right.reserve(cur_list.size());

                for (ui32 k = 0; k < cur_list.size(); k++) {
                    if (cur_list[k] >= m) {
                        layers[i][j].set_bit(k, true);
                        leaf_right.push_back(cur_list[k]);
                    } else {
                        leaf_left.push_back(cur_list[k]);
                    }
                }
            }
        }

        leaf_values = vector<vector<T>>(layer_vals.begin(), layer_vals.end());
        size = new_size;
    }

    void orig_create(const T *p_new_values, const ui32 new_size) {
        if (max_depth == 0)
            throw std::logic_error("The maximum depth is not initialized! Call set_max_depth to set the depth.");
        if (alph_size <= 1)
            throw std::logic_error("The alphabet size is not initialized! Call set_alph_size to set the alphabet size.");

        if (!reserved)
            reserve(new_size, 1);

        vector<DynamicBitvector<B>> &root_layer = layers[0];
        DynamicBitvector<B> &root = root_layer[0];

        for (ui32 i = 0; i < new_size; i++) {
            if (i % 100000 == 0) {
                std::cout << "On the " << i << "th element!" << std::endl;
            }
            insert(i, p_new_values[i]);
        }
    }
    void clear() {
        layers.clear();
        leaf_values.clear();
        reserved = false;
        size = 0;
    }

    void insert(const ui32 &pos, const T &key) {
        T a = 0, b = static_cast<T>(alph_size);
        ui32 index = 0;
        ui32 pos_loc = pos;
        


        for (ui32 i = 0; i <= max_depth; i++) {
            T m = middle(a, b);
            DynamicBitvector<B> &cur_node = layers[i][index];
            cur_node.insert(pos_loc, key >= m);

            if (key >= m) {
                a = m;
                index = child_right(index);
                pos_loc = cur_node.rank1(pos_loc);
            } else {
                b = m;
                index = child_left(index);
                pos_loc = cur_node.rank0(pos_loc);
            }
        }

        leaf_values[index].insert(leaf_values[index].begin() + pos_loc, key);
        size++;
    }
    void remove(ui32 pos) {
        T a = 0, b = static_cast<T>(alph_size);
        ui32 index = 0;

        for (ui32 i = 0; i <= max_depth; i++) {
            T m = middle(a, b);
            DynamicBitvector<B> &cur_node = layers[i][index];
            ui32 next_pos;
            if (cur_node.get_bit(pos)) {
                a = m;
                index = child_right(index);
                next_pos = cur_node.rank1(pos);
            } else {
                b = m;
                index = child_left(index);
                next_pos = cur_node.rank0(pos);
            }
            cur_node.erase(pos);
            pos = next_pos;
        }
        vector<T> &leaf = leaf_values[index];
        leaf.erase(leaf.begin() + pos);
        size++;
    }
    void set_value(ui32 pos, T key) {
    }
    T get_value(ui32 pos) const {

    }

    // Count the number of points in position [pos_l...pos_r)
    // within range [L, U).
    ui32 range(ui32 pos_l, ui32 pos_r, T L, T U) const {
        if (U <= L || pos_r <= pos_l)
            return 0;
        ui32 index_parent = 0;
        ui32 a = 0, b = alph_size;
        ui32 split_depth;
        ui32 m;
        bool broke = false;
        for (split_depth = 0; split_depth <= max_depth; split_depth++) {
            m = middle(a, b);
            if (U <= m) {
                b = m;
                pos_l = layers[split_depth][index_parent].rank0(pos_l);
                pos_r = layers[split_depth][index_parent].rank0(pos_r);
                index_parent = child_left(index_parent);
                continue;
            }
            if (m <= L) {
                a = m;
                pos_l = layers[split_depth][index_parent].rank1(pos_l);
                pos_r = layers[split_depth][index_parent].rank1(pos_r);
                index_parent = child_right(index_parent);
                continue;
            }
            broke = true;
            break;
        }

        if (!broke) {
            return range_raw(leaf_values[index_parent], pos_l, pos_r, L, U);
        }




        // Now i will be the depth at which some node splits [L, U)
        // into two halves. index_parent will be the node, and a
        // and b are the range of this node.

        ui32 count = 0;

        ui32 pos_r_l = layers[split_depth][index_parent].rank1(pos_l);
        ui32 pos_r_r = layers[split_depth][index_parent].rank1(pos_r);
        ui32 pos_l_l = layers[split_depth][index_parent].rank0(pos_l);
        ui32 pos_l_r = layers[split_depth][index_parent].rank0(pos_r);

        if (split_depth == max_depth) {
            // We need to split the last layer.
            ui32 left_count = range_raw(leaf_values[child_left(index_parent)], pos_l_l, pos_l_r, L, U);
            ui32 right_count = range_raw(leaf_values[child_right(index_parent)], pos_r_l, pos_r_r, L, U);
            return left_count + right_count;
        }

        ui32 index_r = child_right(index_parent);
        ui32 a_r = middle(a, b), b_r = b;
        ui32 m_r, i_r;

        ui32 index_l = child_left(index_parent);
        ui32 a_l = a, b_l = middle(a, b);
        ui32 m_l, i_l;

        for (i_r = split_depth + 1; i_r < max_depth; i_r++) {
            m_r = middle(a_r, b_r);
            if (U < m_r) {
                b_r = m_r;
                pos_r_l = layers[i_r][index_r].rank0(pos_r_l);
                pos_r_r = layers[i_r][index_r].rank0(pos_r_r);
                index_r = child_left(index_r);
            } else {
                count += layers[i_r][index_r].rank0(pos_r_r) - layers[i_r][index_r].rank0(pos_r_l);
                a_r = m_r;
                pos_r_l = layers[i_r][index_r].rank1(pos_r_l);
                pos_r_r = layers[i_r][index_r].rank1(pos_r_r);
                index_r = child_right(index_r);
            }
            if (m_r == U)
                break;
        }
        if (a_r < U) {
            m_r = middle(a_r, b_r);
            if (m_r <= U) {
                count += layers[i_r][index_r].rank0(pos_r_r) - layers[i_r][index_r].rank0(pos_r_l);
            }
            if (m_r < U) {
                pos_r_l = layers[i_r][index_r].rank1(pos_r_l);
                pos_r_r = layers[i_r][index_r].rank1(pos_r_r);
                count += range_raw(leaf_values[child_right(index_r)], pos_r_l, pos_r_r, L, U);
            }
            if (m_r > U) {
                pos_r_l = layers[i_r][index_r].rank0(pos_r_l);
                pos_r_r = layers[i_r][index_r].rank0(pos_r_r);
                count += range_raw(leaf_values[child_left(index_r)], pos_r_l, pos_r_r, L, U);
            }
        }

        for (i_l = split_depth + 1; i_l < max_depth; i_l++) {
            m_l = middle(a_l, b_l);
            if (L <= m_l) {
                count += layers[i_l][index_l].rank1(pos_l_r) - layers[i_l][index_l].rank1(pos_l_l);
                b_l = m_l;
                pos_l_l = layers[i_l][index_l].rank0(pos_l_l);
                pos_l_r = layers[i_l][index_l].rank0(pos_l_r);
                index_l = child_left(index_l);
            } else {
                a_l = m_l;
                pos_l_l = layers[i_l][index_l].rank1(pos_l_l);
                pos_l_r = layers[i_l][index_l].rank1(pos_l_r);
                index_l = child_right(index_l);
            }
            if (m_l == L) {
                break;
            }
        }

        if (L < b_l) {
            m_l = middle(a_l, b_l);
            if (L <= m_l) {
                count += layers[i_l][index_l].rank1(pos_l_r) - layers[i_l][index_l].rank1(pos_l_l);
            }
            if (m_l > L) {
                pos_l_l = layers[i_l][index_l].rank0(pos_l_l);
                pos_l_r = layers[i_l][index_l].rank0(pos_l_r);
                count += range_raw(leaf_values[child_left(index_l)], pos_l_l, pos_l_r, L, U);
            }
            if (m_l < L) {
                pos_l_l = layers[i_l][index_l].rank1(pos_l_l);
                pos_l_r = layers[i_l][index_l].rank1(pos_l_r);
                count += range_raw(leaf_values[child_right(index_l)], pos_l_l, pos_l_r, L, U);
            }
        }

        return count;
    }

};






#endif // __WAVELET_TREE_H__