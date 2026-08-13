#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace oed::cpt {

constexpr float SECTOR_SIZE = 512.0f;
constexpr int DEPTH_MAP_SIZE = 1024;

struct Vertex {
  float x = 0.0f;  // local X within node
  float y = 0.0f;  // local Y within node
  float z = 0.0f;  // height (depth / 256.0)
};

struct Strip {
  std::vector<int> indices;
};

struct QuadTreeNode {
  int sector_x = 0;
  int sector_y = 0;
  int size = 0;
  int level = 0;

  std::vector<Vertex> vertices;
  std::vector<Strip> strips;

  std::unique_ptr<QuadTreeNode> children[4];
  bool has_children = false;
  QuadTreeNode* parent = nullptr;

  QuadTreeNode() = default;
  QuadTreeNode(int sx, int sy, int sz, int lvl)
      : sector_x(sx), sector_y(sy), size(sz), level(lvl) {}
};

class CptFile {
 public:
  CptFile() = default;
  ~CptFile() = default;

  // Non-copyable due to unique_ptr trees
  CptFile(const CptFile&) = delete;
  CptFile& operator=(const CptFile&) = delete;
  CptFile(CptFile&&) = default;
  CptFile& operator=(CptFile&&) = default;

  // Header info
  int initial_size() const { return initial_size_; }
  int num_lod_levels() const { return num_lod_levels_; }
  int min_node_size() const { return min_node_size_; }

  // Access depth map (unsigned 16-bit, matching original game)
  const std::vector<uint16_t>& depth_map() const { return depth_map_; }
  float get_height_at(int x, int y) const;

  // Access quadtree
  QuadTreeNode* root_node() const { return root_node_.get(); }

  // Get nodes at specific LOD level (0 = root, higher = finer detail)
  std::vector<QuadTreeNode*> get_nodes_at_lod(int level) const;

  // Sector offset mapping: sector 1-4 -> (x, y) offset
  static std::pair<int, int> get_sector_offset(int sector_index);

  // Calculate total node count for given LOD levels
  static int get_total_node_count(int lod_levels);

 private:
  friend bool parse_cpt(const uint8_t* data, size_t size, CptFile& out, std::string& error);

  int initial_size_ = 0;
  int num_lod_levels_ = 0;
  int min_node_size_ = 0;

  std::vector<uint16_t> depth_map_;  // Unsigned to match original game
  std::unique_ptr<QuadTreeNode> root_node_;

  void collect_nodes_at_level(QuadTreeNode* node, int target, int current,
                              std::vector<QuadTreeNode*>& result) const;
};

// Parse from memory
bool parse_cpt(const uint8_t* data, size_t size, CptFile& out, std::string& error);

// Parse from file path
bool parse_cpt(const std::string& path, CptFile& out, std::string& error);

}  // namespace oed::cpt
