#pragma once

#include "spatial/geometry/bbox.hpp"
#include "spatial/geometry/sgl.hpp"
#include "spatial/util/math.hpp"

#include "duckdb/common/allocator.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/common/types/vector.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>

namespace duckdb {

//======================================================================================================================
// Flat RTree
//======================================================================================================================

template <class T>
class typed_view {
public:
	size_t size() const {
		return len;
	}
	T *data() {
		return ptr;
	}
	const T *data() const {
		return ptr;
	}
	T &operator[](size_t idx) {
		return data()[idx];
	}
	const T &operator[](size_t idx) const {
		return data()[idx];
	}

	void set(T *ptr_p, const size_t len_p) {
		ptr = ptr_p;
		len = len_p;
	}

private:
	T *ptr = nullptr;
	size_t len = 0;
};

//----------------------------------------------------------------------------------------------------------------------
// Intersection scan state
//----------------------------------------------------------------------------------------------------------------------
class FlatRTree;

class FlatRTreeScanState {
	friend class FlatRTree;
	using Box = Box2D<float>;

public:
	explicit FlatRTreeScanState() : matches(LogicalType::POINTER) {
	}

public:
	Vector matches;
	idx_t matches_count = 0;
	idx_t matches_idx = 0;

private:
	queue<size_t> search_queue;
	Box search_box;
	size_t entry_beg = 0;
	size_t entry_pos = 0;
	bool exhausted = true;
};

//----------------------------------------------------------------------------------------------------------------------
// KNN scan state
//----------------------------------------------------------------------------------------------------------------------

class FlatRTreeKNNState {
	friend class FlatRTree;

private:
	struct HeapEntry {
		double min_dist_sq;
		uint32_t node_idx;
		bool is_leaf;
		data_ptr_t row_ptr;

		bool operator>(const HeapEntry &other) const {
			return min_dist_sq > other.min_dist_sq;
		}
	};

	void Push(const HeapEntry &entry) {
		heap.push_back(entry);
		std::push_heap(heap.begin(), heap.end(), std::greater<HeapEntry>());
	}

	HeapEntry Pop() {
		std::pop_heap(heap.begin(), heap.end(), std::greater<HeapEntry>());
		const auto entry = heap.back();
		heap.pop_back();
		return entry;
	}

	vector<HeapEntry> heap;
};

//----------------------------------------------------------------------------------------------------------------------
// FlatRTree
//----------------------------------------------------------------------------------------------------------------------

class FlatRTree {
public:
	using Box = Box2D<float>;

	FlatRTree(Allocator &alloc, uint32_t item_count_p, uint32_t node_size_p)
	    : item_count(item_count_p), node_size(node_size_p) {

		ComputeLayerBounds();

		if (item_count_p == 0) {
			return;
		}

		const auto nodes = layer_bounds.back();

		box_array_mem = alloc.Allocate(sizeof(Box) * nodes);
		idx_array_mem = alloc.Allocate(sizeof(uint32_t) * nodes);
		row_array_mem = alloc.Allocate(sizeof(data_ptr_t) * item_count);

		box_array.set(reinterpret_cast<Box *>(box_array_mem.get()), nodes);
		idx_array.set(reinterpret_cast<uint32_t *>(idx_array_mem.get()), nodes);
		row_array.set(reinterpret_cast<data_ptr_t *>(row_array_mem.get()), item_count);

		for (size_t i = 0; i < nodes; i++) {
			box_array[i] = Box();
			idx_array[i] = 0;
		}
		for (size_t i = 0; i < item_count; i++) {
			row_array[i] = nullptr;
		}
	}

	// The bounding box covering all items in the tree (for DWithin joins this is already expanded by
	// the constant distance, since the per-item boxes are expanded before being pushed)
	const Box &Bounds() const {
		return tree_box;
	}

	// Returns the number of leaf items in the R-tree.
	uint32_t Count() const {
		return item_count;
	}

	uint32_t Push(const Box &box, data_ptr_t row) {
		idx_array[current_position] = current_position;
		box_array[current_position] = box;
		tree_box.Union(box);
		row_array[current_position] = row;
		return current_position++;
	}

	static void Sort(vector<uint32_t> &curve, typed_view<Box> &box_array, typed_view<uint32_t> &idx_array) {
		Sort(curve, box_array, idx_array, 0, curve.size() - 1);
	}

	static void Sort(vector<uint32_t> &curve, typed_view<Box> &box_array, typed_view<uint32_t> &idx_array, size_t l_idx,
	                 size_t r_idx) {
		if (l_idx < r_idx) {
			const auto pivot = curve[(l_idx + r_idx) >> 1];
			auto pivot_l = l_idx - 1;
			auto pivot_r = r_idx + 1;

			while (true) {
				do {
					++pivot_l;
				} while (curve[pivot_l] < pivot);
				do {
					--pivot_r;
				} while (curve[pivot_r] > pivot);

				if (pivot_l >= pivot_r) {
					break;
				}

				std::swap(curve[pivot_l], curve[pivot_r]);
				std::swap(box_array[pivot_l], box_array[pivot_r]);
				std::swap(idx_array[pivot_l], idx_array[pivot_r]);
			}

			Sort(curve, box_array, idx_array, l_idx, pivot_r);
			Sort(curve, box_array, idx_array, pivot_r + 1, r_idx);
		}
	}

	void Build() {
		D_ASSERT(current_position <= item_count);

		if (current_position < item_count) {
			// Fewer items were pushed than the tree was sized for, shrink to what was actually pushed.
			item_count = current_position;
			ComputeLayerBounds();
		}

		if (item_count == 0) {
			// Nothing was pushed, there is nothing to build, and scans are guarded by Count() == 0.
			return;
		}

		if (item_count <= node_size) {
			box_array[current_position++] = tree_box;
			return;
		}

		constexpr auto max_hilbert = std::numeric_limits<uint16_t>::max();
		const auto hw = max_hilbert / (tree_box.max.x - tree_box.min.x);
		const auto hh = max_hilbert / (tree_box.max.y - tree_box.min.y);

		vector<uint32_t> curve(item_count);
		for (idx_t i = 0; i < item_count; i++) {
			const auto &node_box = box_array[i];

			const auto hx = static_cast<uint32_t>(hw * ((node_box.min.x + node_box.max.x) / 2 - tree_box.min.x));
			const auto hy = static_cast<uint32_t>(hh * ((node_box.min.y + node_box.max.y) / 2 - tree_box.min.y));

			curve[i] = sgl::math::hilbert_encode(16, hx, hy);
		}

		Sort(curve, box_array, idx_array);

		size_t layer_idx = 0;
		size_t entry_idx = 0;

		while (layer_idx < layer_bounds.size() - 1) {
			const auto entry_end = layer_bounds[layer_idx];

			while (entry_idx < entry_end) {
				auto node_idx = entry_idx;
				auto node_box = box_array[entry_idx];

				size_t child_idx = 0;
				while (child_idx < node_size && entry_idx < entry_end) {
					node_box.Union(box_array[entry_idx]);
					child_idx++;
					entry_idx++;
				}

				idx_array[current_position] = node_idx;
				box_array[current_position] = node_box;
				current_position++;
			}

			layer_idx++;
		}
	}

	size_t UpperBound(size_t node_idx) const {
		const auto it = std::upper_bound(layer_bounds.begin(), layer_bounds.end(), node_idx);
		if (it == layer_bounds.end()) {
			return layer_bounds.back();
		}
		return *it;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Intersection scan
	//------------------------------------------------------------------------------------------------------------------

	void InitScan(FlatRTreeScanState &state, const Box &box) const {
		while (!state.search_queue.empty()) {
			state.search_queue.pop();
		}
		state.search_box = box;
		// The root node is the last entry of the top layer.
		// Note that this may be less than box_array.size() - 1 when Build() shrank the tree below its allocated size.
		state.entry_beg = layer_bounds.back() - 1;
		state.entry_pos = state.entry_beg;

		state.exhausted = false;
		state.matches_idx = 0;
		state.matches_count = 0;
	}

	bool Scan(FlatRTreeScanState &state) const {
		if (state.exhausted) {
			return false;
		}

		idx_t count = 0;
		const auto ptr = FlatVector::GetData<data_ptr_t>(state.matches);
		Lookup(state, [&](const data_ptr_t &row) {
			ptr[count++] = row;
			return count == STANDARD_VECTOR_SIZE;
		});
		state.matches_count = count;
		state.matches_idx = 0;

		return count > 0;
	}

	template <class CALLBACK>
	void Lookup(FlatRTreeScanState &state, CALLBACK &&callback) const {

		while (true) {

			const auto entry_end = std::min(state.entry_beg + node_size, UpperBound(state.entry_beg));

			while (state.entry_pos < entry_end) {
				if (!state.search_box.Intersects(box_array[state.entry_pos])) {
					state.entry_pos++;
					continue;
				}

				auto yield = false;

				if (state.entry_beg >= item_count) {
					state.search_queue.push(idx_array[state.entry_pos]);
				} else {
					yield = callback(row_array[idx_array[state.entry_pos]]);
				}

				state.entry_pos++;

				if (yield) {
					return;
				}
			}

			if (state.search_queue.empty()) {
				state.exhausted = true;
				return;
			}

			state.entry_beg = state.search_queue.front();
			state.entry_pos = state.entry_beg;
			state.search_queue.pop();
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	// KNN search (Hjaltason-Samet best-first traversal)
	//------------------------------------------------------------------------------------------------------------------

	void InitKNN(FlatRTreeKNNState &state, const Box &query) const {
		state.heap.clear();
		if (item_count == 0) {
			return;
		}
		const auto root_idx = layer_bounds.back() - 1;
		state.Push({MinDistanceSquared(query, box_array[root_idx]), root_idx, false, nullptr});
	}

	// Yields the rows in order of non-decreasing distance between their bounding box and the query box.
	// The reported distance is a lower bound on the distance between the geometries themselves.
	bool NextKNN(FlatRTreeKNNState &state, const Box &query, data_ptr_t &row, double &min_dist_sq) const {
		while (!state.heap.empty()) {
			const auto top = state.Pop();

			if (top.is_leaf) {
				row = top.row_ptr;
				min_dist_sq = top.min_dist_sq;
				return true;
			}

			const size_t entry_beg = top.node_idx;
			const auto entry_end = std::min(entry_beg + node_size, UpperBound(entry_beg));
			const auto is_leaf_level = entry_beg < item_count;

			for (size_t i = entry_beg; i < entry_end; i++) {
				const auto child_dist = MinDistanceSquared(query, box_array[i]);
				if (is_leaf_level) {
					state.Push({child_dist, idx_array[i], true, row_array[idx_array[i]]});
				} else {
					state.Push({child_dist, idx_array[i], false, nullptr});
				}
			}
		}
		return false;
	}

private:
	// Computed in double precision so that the result stays a lower bound for boxes that were rounded outwards
	static double MinDistanceSquared(const Box &lhs, const Box &rhs) {
		double dx = 0;
		double dy = 0;
		if (rhs.max.x < lhs.min.x) {
			dx = static_cast<double>(lhs.min.x) - static_cast<double>(rhs.max.x);
		} else if (rhs.min.x > lhs.max.x) {
			dx = static_cast<double>(rhs.min.x) - static_cast<double>(lhs.max.x);
		}
		if (rhs.max.y < lhs.min.y) {
			dy = static_cast<double>(lhs.min.y) - static_cast<double>(rhs.max.y);
		} else if (rhs.min.y > lhs.max.y) {
			dy = static_cast<double>(rhs.min.y) - static_cast<double>(lhs.max.y);
		}
		return dx * dx + dy * dy;
	}

private:
	//! (Re)compute the cumulative per-layer node counts for the current item_count
	void ComputeLayerBounds() {
		layer_bounds.clear();
		uint32_t count = item_count;
		uint32_t nodes = item_count;
		layer_bounds.push_back(nodes);
		if (item_count == 0) {
			return;
		}
		do {
			count = (count + node_size - 1) / node_size;
			nodes += count;
			layer_bounds.push_back(nodes);
		} while (count > 1);
	}

	vector<uint32_t> layer_bounds;

	AllocatedData box_array_mem;
	AllocatedData idx_array_mem;
	AllocatedData row_array_mem;

	// Buffer-managed storage (lazy-pin path)
	BufferHandle build_pin_box; // held during Push/Build, released after
	BufferHandle build_pin_idx;
	BufferHandle build_pin_row;

	typed_view<uint32_t> idx_array;
	typed_view<Box> box_array;
	typed_view<data_ptr_t> row_array;

	Box tree_box;

	uint32_t item_count = 0;
	uint32_t node_size = 0;
	uint32_t current_position = 0;
};

} // namespace duckdb
