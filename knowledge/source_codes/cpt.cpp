#include "cpt/cpt.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <queue>

namespace {

// BitReader for parsing variable-width bit fields (LSB-first, matching NovaLogic format)
class BitReader {
 public:
  BitReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

  uint32_t read_bits(int count) {
    if (count <= 0 || count > 32) return 0;

    uint32_t result = 0;
    int bits_remaining = count;
    int result_shift = 0;

    while (bits_remaining > 0) {
      if (byte_pos_ >= size_) return result;

      uint8_t current_byte = data_[byte_pos_];
      int bits_available = 8 - bit_pos_;
      int bits_to_read = std::min(bits_remaining, bits_available);
      uint32_t mask = (1u << bits_to_read) - 1;

      // Read bits from LSB of current byte starting at bit_pos_
      uint32_t bits = (current_byte >> bit_pos_) & mask;
      result |= bits << result_shift;

      result_shift += bits_to_read;
      bits_remaining -= bits_to_read;
      byte_pos_ += (bit_pos_ + bits_to_read) / 8;
      bit_pos_ = (bit_pos_ + bits_to_read) % 8;
    }

    return result;
  }

  uint8_t read_u8() {
    align_to_byte();
    if (byte_pos_ >= size_) return 0;
    return data_[byte_pos_++];
  }

  int16_t read_i16() {
    align_to_byte();
    if (byte_pos_ + 2 > size_) return 0;
    int16_t val = static_cast<int16_t>(data_[byte_pos_] | (data_[byte_pos_ + 1] << 8));
    byte_pos_ += 2;
    return val;
  }

  int32_t read_i32() {
    align_to_byte();
    if (byte_pos_ + 4 > size_) return 0;
    int32_t val = data_[byte_pos_] | (data_[byte_pos_ + 1] << 8) |
                  (data_[byte_pos_ + 2] << 16) | (data_[byte_pos_ + 3] << 24);
    byte_pos_ += 4;
    return val;
  }

  void read_bytes(uint8_t* out, size_t count) {
    align_to_byte();
    size_t to_read = std::min(count, size_ - byte_pos_);
    if (to_read > 0) {
      std::memcpy(out, data_ + byte_pos_, to_read);
      byte_pos_ += to_read;
    }
    // Zero-fill remaining if we hit end
    if (to_read < count) {
      std::memset(out + to_read, 0, count - to_read);
    }
  }

  void skip_bytes(size_t count) {
    align_to_byte();
    byte_pos_ = std::min(byte_pos_ + count, size_);
  }

  void align_to_byte() {
    if (bit_pos_ > 0) {
      bit_pos_ = 0;
      ++byte_pos_;
    }
  }

  void align_to_4_bytes() {
    align_to_byte();
    size_t rem = byte_pos_ % 4;
    if (rem != 0) {
      byte_pos_ += (4 - rem);
    }
  }

  size_t position() const { return byte_pos_; }
  bool at_end() const { return byte_pos_ >= size_; }

 private:
  const uint8_t* data_;
  size_t size_;
  size_t byte_pos_ = 0;
  int bit_pos_ = 0;
};

}  // namespace

namespace oed::cpt {

float CptFile::get_height_at(int x, int y) const {
  if (depth_map_.empty()) return 0.0f;

  // Use DEPTH_MAP_SIZE (1024) consistently to match C# hardcoded behavior
  x = std::max(0, std::min(x, DEPTH_MAP_SIZE - 1));
  y = std::max(0, std::min(y, DEPTH_MAP_SIZE - 1));

  int idx = y * DEPTH_MAP_SIZE + x;
  if (idx >= 0 && idx < static_cast<int>(depth_map_.size())) {
    return depth_map_[idx] / 256.0f;
  }
  return 0.0f;
}

std::vector<QuadTreeNode*> CptFile::get_nodes_at_lod(int level) const {
  std::vector<QuadTreeNode*> result;
  if (root_node_) {
    collect_nodes_at_level(root_node_.get(), level, 0, result);
  }
  return result;
}

void CptFile::collect_nodes_at_level(QuadTreeNode* node, int target, int current,
                                      std::vector<QuadTreeNode*>& result) const {
  if (!node) return;
  if (current == target) {
    result.push_back(node);
    return;
  }
  if (node->has_children) {
    for (int i = 0; i < 4; ++i) {
      if (node->children[i]) {
        collect_nodes_at_level(node->children[i].get(), target, current + 1, result);
      }
    }
  }
}

std::pair<int, int> CptFile::get_sector_offset(int sector_index) {
  // Maps sector 1-4 to quadrant offsets (matches opennova2 behavior)
  switch (sector_index) {
    case 1: return {0, 0};                                          // top-left
    case 2: return {0, static_cast<int>(SECTOR_SIZE)};              // bottom-left
    case 3: return {static_cast<int>(SECTOR_SIZE), 0};              // top-right
    case 4: return {static_cast<int>(SECTOR_SIZE), static_cast<int>(SECTOR_SIZE)};  // bottom-right
    default: return {0, 0};
  }
}

int CptFile::get_total_node_count(int lod_levels) {
  // Sum of geometric series: (4^n - 1) / 3
  return (static_cast<int>(std::pow(4, lod_levels)) - 1) / 3;
}

bool parse_cpt(const uint8_t* data, size_t size, CptFile& out, std::string& error) {
  out = CptFile{};

  if (size < 0xA0) {
    error = "file too small for header";
    return false;
  }

  BitReader reader(data, size);

  // Read header magic
  uint8_t magic[4];
  reader.read_bytes(magic, 4);
  if (magic[0] != 'C' || magic[1] != 'P' || magic[2] != 'T') {
    error = "invalid magic (expected CPT)";
    return false;
  }

  out.initial_size_ = reader.read_i16();
  out.num_lod_levels_ = reader.read_i16();
  out.min_node_size_ = out.initial_size_ / static_cast<int>(std::pow(2, out.num_lod_levels_ - 1));

  // Skip rest of header (0xA0 bytes total)
  reader.skip_bytes(0xA0 - 8);

  // Pre-allocate depth map - always 1024x1024 to match C# hardcoded behavior
  out.depth_map_.resize(DEPTH_MAP_SIZE * DEPTH_MAP_SIZE, 0);

  // Parse chunks
  while (!reader.at_end()) {
    uint8_t chunk_magic[4];
    reader.read_bytes(chunk_magic, 4);

    // Check for null terminator
    if (chunk_magic[0] == 0 && chunk_magic[1] == 0 && chunk_magic[2] == 0 && chunk_magic[3] == 0) {
      break;
    }

    if (chunk_magic[0] == 'P' && chunk_magic[1] == 'O' && chunk_magic[2] == 'L' && chunk_magic[3] == 'Y') {
      // Parse POLY chunk - quadtree polygon data
      std::queue<QuadTreeNode*> nodes_queue;

      out.root_node_ = std::make_unique<QuadTreeNode>(0, 0, out.initial_size_, 0);
      nodes_queue.push(out.root_node_.get());

      while (!nodes_queue.empty()) {
        QuadTreeNode* current = nodes_queue.front();
        nodes_queue.pop();

        // Parse node data
        reader.align_to_4_bytes();

        int sector_x = reader.read_bits(10);
        int sector_y = reader.read_bits(10);
        int num_vertices = reader.read_bits(16);
        int vert_bit_size = reader.read_bits(4);
        int num_strips = reader.read_bits(7);
        bool is_flagged = reader.read_bits(1) != 0;
        (void)is_flagged;  // Not used in mesh generation
        int strip_chunk_size = reader.read_bits(8);

        current->sector_x = sector_x;
        current->sector_y = sector_y;

        // Parse vertices
        if (num_vertices > 0) {
          current->vertices.resize(num_vertices);
          for (int i = 0; i < num_vertices; ++i) {
            int x = reader.read_bits(vert_bit_size);
            int y = reader.read_bits(vert_bit_size);

            int row = (sector_y + y) & (DEPTH_MAP_SIZE - 1);
            int col = (sector_x + x) & (DEPTH_MAP_SIZE - 1);
            int idx = DEPTH_MAP_SIZE * row + col;

            current->vertices[i].x = static_cast<float>(x);
            current->vertices[i].y = static_cast<float>(y);
            // Height will be filled in after depth map is parsed
            // For now, just store a placeholder
            current->vertices[i].z = 0.0f;
          }
        }

        reader.align_to_4_bytes();

        // Parse strips
        if (num_strips > 0) {
          current->strips.resize(num_strips);
          for (int t = 0; t < num_strips; ++t) {
            reader.align_to_byte();

            int num_bits = reader.read_bits(16);
            int strip_vert_count = reader.read_bits(15);
            int flagged = reader.read_bits(1);
            (void)strip_vert_count;
            (void)flagged;

            Strip& strip = current->strips[t];

            for (int remaining = num_bits; remaining > 0; remaining -= strip_chunk_size) {
              int s = std::min(strip_chunk_size, remaining);

              int num_bits_needed = reader.read_bits(4);
              int m = reader.read_bits(12);

              for (int q = 0; q < s; ++q) {
                int idx = reader.read_bits(num_bits_needed);
                strip.indices.push_back(idx + m);
              }
            }
          }
        }

        // Create children if not at minimum node size
        if (current->size > out.min_node_size_) {
          int child_size = current->size / 2;
          int child_level = current->level + 1;

          for (int i = 0; i < 4; ++i) {
            int offset_x = (i % 2 == 1) ? child_size : 0;
            int offset_y = (i / 2 == 1) ? child_size : 0;

            auto child = std::make_unique<QuadTreeNode>(
                current->sector_x + offset_x,
                current->sector_y + offset_y,
                child_size,
                child_level);
            child->parent = current;

            nodes_queue.push(child.get());
            current->children[i] = std::move(child);
          }
          current->has_children = true;
        }
      }

      reader.align_to_byte();

    } else if (chunk_magic[0] == 'C' && chunk_magic[1] == 'D' && chunk_magic[2] == 'E' && chunk_magic[3] == 'P') {
      // Parse CDEP chunk - compressed depth map
      int dim_x = reader.read_bits(16);
      int dim_y = reader.read_bits(16);

      int idx = 0;
      for (int i = 0; i < dim_x && idx < static_cast<int>(out.depth_map_.size()); ++i) {
        int bit_width = reader.read_bits(4);
        int base_value = reader.read_bits(16);

        for (int j = 0; j < dim_y && idx < static_cast<int>(out.depth_map_.size()); ++j) {
          int delta = reader.read_bits(bit_width);
          out.depth_map_[idx] = static_cast<uint16_t>(delta + base_value);
          ++idx;
        }
      }

      reader.align_to_byte();
    }
    // Unknown chunks are skipped (we don't know their size, so this may fail)
  }

  // Post-process: Update all vertex heights using the depth map
  // This happens after all chunks are parsed, so CDEP and POLY can appear in any order
  if (out.root_node_ && !out.depth_map_.empty()) {
    std::queue<QuadTreeNode*> nodes_queue;
    nodes_queue.push(out.root_node_.get());

    while (!nodes_queue.empty()) {
      QuadTreeNode* node = nodes_queue.front();
      nodes_queue.pop();

      // Update vertex heights from depth map
      for (auto& vert : node->vertices) {
        int row = (node->sector_y + static_cast<int>(vert.y)) & (DEPTH_MAP_SIZE - 1);
        int col = (node->sector_x + static_cast<int>(vert.x)) & (DEPTH_MAP_SIZE - 1);
        int idx = DEPTH_MAP_SIZE * row + col;
        if (idx >= 0 && idx < static_cast<int>(out.depth_map_.size())) {
          vert.z = out.depth_map_[idx] / 256.0f;
        }
      }

      // Queue children
      for (int i = 0; i < 4; ++i) {
        if (node->children[i]) {
          nodes_queue.push(node->children[i].get());
        }
      }
    }
  }

  return true;
}

bool parse_cpt(const std::string& path, CptFile& out, std::string& error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "failed to open file";
    return false;
  }

  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> data(size);
  file.read(reinterpret_cast<char*>(data.data()), size);

  return parse_cpt(data.data(), data.size(), out, error);
}

}  // namespace oed::cpt
