#include "kimera_dsg_builder/dsg_lcd_module.h"

#include <gtest/gtest.h>

namespace kimera {
namespace lcd {

TEST(DsgLcdMatchingTests, TestCosineDistanceFixedSize) {
  Descriptor d1;
  d1.values.resize(5, 1);
  d1.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d1.normalized = false;

  Descriptor d2;
  d2.values.resize(5, 1);
  d2.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d2.normalized = false;

  {  // identical vectors: score of 1
    float result = computeCosineDistance(d1, d2);
    EXPECT_NEAR(1.0f, result, 1.0e-6f);
  }

  {  // one vector is 0: score of 0
    d1.values = Eigen::MatrixXf::Zero(5, 1);

    float result = computeCosineDistance(d1, d2);
    EXPECT_NEAR(0.0f, result, 1.0e-6f);
  }

  {  // one normalized vector
    d1.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
    d1.values.normalize();
    d1.normalized = true;

    float result = computeCosineDistance(d1, d2);
    EXPECT_NEAR(1.0f, result, 1.0e-6f);
  }

  {  // other normalized vector
    d1.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
    d2.values.normalize();
    d1.normalized = false;
    d2.normalized = true;

    float result = computeCosineDistance(d1, d2);
    EXPECT_NEAR(1.0f, result, 1.0e-6f);
  }

  {  // both normalized
    d1.values.normalize();
    d2.values.normalize();
    d1.normalized = true;
    d2.normalized = true;

    float result = computeCosineDistance(d1, d2);
    EXPECT_NEAR(1.0f, result, 1.0e-6f);
  }
}

TEST(DsgLcdMatchingTests, TestCosineDistanceVariableSize) {
  Descriptor d1;
  d1.values.resize(5, 1);
  d1.words.resize(5, 1);
  d1.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d1.words << 1, 2, 3, 4, 5;
  d1.normalized = false;

  Descriptor d2;
  d2.values.resize(5, 1);
  d2.words.resize(5, 1);
  d2.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d2.words << 1, 2, 3, 4, 5;
  d2.normalized = false;

  {  // vectors are the same: score of 1
    float result = computeCosineDistance(d1, d2);
    EXPECT_NEAR(1.0f, result, 1.0e-6f);
  }

  {  // vectors are essentially the same: score of 1
    d1.words << 1, 2, 4, 5, 7;
    d1.values << 1.0f, 2.0f, 3.0f, 0.0f, 6.0f;
    d2.words << 1, 2, 3, 4, 7;
    d2.values << 1.0f, 2.0f, 0.0f, 3.0f, 6.0f;

    float result = computeCosineDistance(d1, d2);
    EXPECT_NEAR(1.0f, result, 1.0e-6f);
  }
}

TEST(DsgLcdMatchingTests, TestComputeL1DistanceBow) {
  Descriptor d1;
  d1.values.resize(5, 1);
  d1.words.resize(5, 1);
  d1.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d1.words << 1, 2, 3, 4, 5;
  d1.normalized = false;

  Descriptor d2;
  d2.values.resize(5, 1);
  d2.words.resize(5, 1);
  d2.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d2.words << 1, 2, 3, 4, 5;
  d2.normalized = false;

  {  // vectors are the same: score of 1
    float result = computeL1Distance(d1, d2);
    EXPECT_NEAR(0.0f, result, 1.0e-6f);
  }

  {  // vectors are essentially the same: score of 1
    d1.words << 1, 2, 4, 5, 7;
    d1.values << 1.0f, 2.0f, 3.0f, 0.0f, 6.0f;
    d2.words << 1, 2, 3, 4, 7;
    d2.values << 1.0f, 2.0f, 0.0f, 3.0f, 6.0f;

    float result = computeL1Distance(d1, d2);
    EXPECT_NEAR(0.0f, result, 1.0e-6f);
  }

  {  // vectors have different norms: score is less than 1
    d1.words << 1, 2, 4, 5, 7;
    d1.values << 1.0f, 2.0f, 1.0f, 0.0f, 6.0f;
    d2.words << 1, 2, 3, 4, 7;
    d2.values << 1.0f, 2.0f, 0.0f, 9.0f, 6.0f;

    float result = computeL1Distance(d1, d2);
    EXPECT_LT(0.0f, result);
  }
}

TEST(DsgLcdMatchingTests, TestComputeL1DistanceFixed) {
  Descriptor d1;
  d1.values.resize(5, 1);
  d1.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d1.normalized = false;

  Descriptor d2;
  d2.values.resize(5, 1);
  d2.values << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;
  d2.normalized = false;

  {  // vectors are the same: score of 1
    float result = computeL1Distance(d1, d2);
    EXPECT_NEAR(0.0f, result, 1.0e-6f);
  }

  {  // vectors are essentially the same: score of 1
    d1.values << 1.0f, 2.0f, 3.0f, 0.0f, 6.0f;
    d2.values << 1.0f, 2.0f, 0.0f, 3.0f, 6.0f;

    float result = computeL1Distance(d1, d2);
    EXPECT_NEAR(0.5f, result, 1.0e-6f);
  }
}

template <typename... Ts>
Descriptor::Ptr makeDescriptor(Ts... args) {
  Descriptor::Ptr descriptor(new Descriptor());

  std::vector<float> data{args...};
  descriptor->values = Eigen::VectorXf(data.size(), 1);
  std::memcpy(descriptor->values.data(),
              data.data(),
              descriptor->values.size() * sizeof(float));
  return descriptor;
}

void fillDescriptor(Descriptor& descriptor, NodeId root, std::set<NodeId> nodes) {
  descriptor.root_node = root;
  descriptor.nodes = nodes;
}

TEST(DsgLcdMatchingTests, SearchDescriptorsNoValid) {
  Descriptor::Ptr query = makeDescriptor(1.0f);

  DescriptorMatchConfig config;
  config.min_score = 0.8f;

  std::set<NodeId> valid_matches;

  DescriptorCache descriptors;
  descriptors[1] = makeDescriptor(0.9f);
  descriptors[2] = makeDescriptor(0.9f);
  descriptors[3] = makeDescriptor(0.9f);

  std::map<NodeId, std::set<NodeId>> root_leaf_map;
  root_leaf_map[1] = {};
  root_leaf_map[2] = {};
  root_leaf_map[3] = {};
  LayerSearchResults results =
      searchDescriptors(*query, config, valid_matches, descriptors, root_leaf_map, 5);
  EXPECT_LT(results.best_score, config.min_score);
  EXPECT_TRUE(results.valid_matches.empty());
  EXPECT_TRUE(results.query_nodes.empty());
  EXPECT_TRUE(results.match_nodes.empty());
}

TEST(DsgLcdMatchingTests, SearchDescriptorsValidNoMatches) {
  Descriptor::Ptr query = makeDescriptor(1.0f);

  DescriptorMatchConfig config;
  config.min_score = 1.5f;

  std::set<NodeId> valid_matches{1, 2, 3};

  DescriptorCache descriptors;
  descriptors[1] = makeDescriptor(0.9f);
  descriptors[2] = makeDescriptor(0.9f);
  descriptors[3] = makeDescriptor(0.9f);

  std::map<NodeId, std::set<NodeId>> root_leaf_map;
  root_leaf_map[1] = {};
  root_leaf_map[2] = {};
  root_leaf_map[3] = {};

  LayerSearchResults results =
      searchDescriptors(*query, config, valid_matches, descriptors, root_leaf_map, 5);
  EXPECT_LT(results.best_score, config.min_score);
  EXPECT_TRUE(results.valid_matches.empty());
  EXPECT_TRUE(results.query_nodes.empty());
  EXPECT_TRUE(results.match_nodes.empty());
}

TEST(DsgLcdMatchingTests, SearchDescriptorsValidSomeMatches) {
  Descriptor::Ptr query = makeDescriptor(1.0f, 0.0f);
  fillDescriptor(*query, 0, {13, 14, 15});

  DescriptorMatchConfig config;
  config.min_score = 0.9f;
  config.min_time_separation_s = 0.0;

  std::set<NodeId> valid_matches{1, 2, 3};

  DescriptorCache descriptors;
  descriptors[1] = makeDescriptor(0.9f, 0.1f);
  fillDescriptor(*descriptors[1], 1, {4, 5, 6});
  descriptors[2] = makeDescriptor(0.9f, 0.9f);
  fillDescriptor(*descriptors[2], 2, {7, 8, 9});
  descriptors[3] = makeDescriptor(0.9f, 0.05f);
  fillDescriptor(*descriptors[3], 3, {10, 11, 12});

  std::map<NodeId, std::set<NodeId>> root_leaf_map;
  root_leaf_map[1] = {};
  root_leaf_map[2] = {};
  root_leaf_map[3] = {};

  LayerSearchResults results =
      searchDescriptors(*query, config, valid_matches, descriptors, root_leaf_map, 5);
  EXPECT_GT(results.best_score, config.min_score);
  EXPECT_EQ(3u, results.best_node);

  std::set<NodeId> expected_matches{1, 3};
  EXPECT_EQ(expected_matches, results.valid_matches);

  std::set<NodeId> expected_match_nodes{10, 11, 12};
  EXPECT_EQ(expected_match_nodes, results.match_nodes);
  EXPECT_EQ(3u, results.match_root);

  std::set<NodeId> expected_query_nodes{13, 14, 15};
  EXPECT_EQ(expected_query_nodes, results.query_nodes);
  EXPECT_EQ(0u, results.query_root);
}

TEST(DsgLcdMatchingTests, searchLeafDescriptorsNoValid) {
  Descriptor::Ptr query = makeDescriptor(1.0f);

  DescriptorMatchConfig config;

  std::set<NodeId> valid_matches;

  DescriptorCacheMap descriptors;
  descriptors[1] = DescriptorCache();
  descriptors[2] = DescriptorCache();
  descriptors[1][1] = makeDescriptor(0.9f);
  descriptors[1][2] = makeDescriptor(0.9f);
  descriptors[1][3] = makeDescriptor(0.9f);
  descriptors[2][4] = makeDescriptor(0.9f);

  LayerSearchResults results =
      searchLeafDescriptors(*query, config, valid_matches, descriptors, 10);
  EXPECT_EQ(0.0f, results.best_score);
  EXPECT_TRUE(results.valid_matches.empty());
  EXPECT_TRUE(results.query_nodes.empty());
  EXPECT_TRUE(results.match_nodes.empty());
}

TEST(DsgLcdMatchingTests, SearchLeafDescriptorsAllValid) {
  Descriptor::Ptr query = makeDescriptor(1.0f);

  DescriptorMatchConfig config;
  config.min_time_separation_s = 0.0;

  std::set<NodeId> valid_matches{1, 2};

  DescriptorCacheMap descriptors;
  descriptors[1] = DescriptorCache();
  descriptors[2] = DescriptorCache();
  descriptors[1][1] = makeDescriptor(0.9f);
  descriptors[1][2] = makeDescriptor(0.9f);
  descriptors[1][3] = makeDescriptor(0.9f);
  descriptors[2][4] = makeDescriptor(0.9f);

  LayerSearchResults results =
      searchLeafDescriptors(*query, config, valid_matches, descriptors, 10);
  EXPECT_EQ(1.0f, results.best_score);
  EXPECT_EQ(1u, results.best_node);
  std::set<NodeId> expected_matches{1};
  EXPECT_EQ(expected_matches, results.valid_matches);

  EXPECT_TRUE(results.query_nodes.empty());
  std::set<NodeId> expected_match_nodes{1};
  EXPECT_EQ(expected_match_nodes, results.match_nodes);
}

TEST(DsgLcdMatchingTests, SearchLeafDescriptorsTimeSeparation) {
  Descriptor::Ptr query = makeDescriptor(1.0f);

  DescriptorMatchConfig config;
  config.min_time_separation_s = 10.0;

  std::set<NodeId> valid_matches{1, 2};

  DescriptorCacheMap descriptors;
  descriptors[1] = DescriptorCache();
  descriptors[2] = DescriptorCache();
  descriptors[1][1] = makeDescriptor(0.9f);
  descriptors[1][2] = makeDescriptor(0.9f);
  descriptors[1][3] = makeDescriptor(0.9f);
  descriptors[2][4] = makeDescriptor(0.9f);

  LayerSearchResults results =
      searchLeafDescriptors(*query, config, valid_matches, descriptors, 10);
  EXPECT_EQ(0.0f, results.best_score);
  EXPECT_TRUE(results.valid_matches.empty());
  EXPECT_TRUE(results.query_nodes.empty());
  EXPECT_TRUE(results.match_nodes.empty());
}

}  // namespace lcd
}  // namespace kimera
