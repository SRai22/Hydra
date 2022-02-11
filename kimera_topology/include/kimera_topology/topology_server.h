#pragma once
#include "kimera_topology/configs.h"
#include "kimera_topology/topology_server_visualizer.h"

#include <hydra_utils/display_utils.h>
#include <kimera_topology/ActiveLayer.h>
#include <kimera_topology/ActiveMesh.h>
#include <voxblox_ros/conversions.h>
#include <voxblox_ros/mesh_vis.h>
#include <voxblox_ros/ros_params.h>

#include <glog/logging.h>
#include <ros/ros.h>

namespace kimera {
namespace topology {

template <typename BaseTsdfServerType>
class TopologyServer {
 public:
  class TsdfServerType : public BaseTsdfServerType {
   public:
    TsdfServerType(const ros::NodeHandle& nh, const ros::NodeHandle& pnh)
        : BaseTsdfServerType(nh, pnh), has_pose(false) {}

    virtual void newPoseCallback(const voxblox::Transformation& T_G_C) override {
      has_pose = true;
      T_G_C_last = T_G_C;
    }

    bool has_pose;
    voxblox::Transformation T_G_C_last;
  };

  explicit TopologyServer(const ros::NodeHandle& nh) : nh_(nh) {
    setupConfig("~");
    setupLayers();

    visualizer_.reset(new TopologyServerVisualizer("~"));

    // we need two publishers for the mesh: voxblox offers no way to distinguish between
    // deleted blocks and blocks that were cleared by observation
    if (config_.publish_archived) {
      mesh_pub_ = nh_.advertise<kimera_topology::ActiveMesh>("active_mesh", 1, true);
    } else {
      mesh_pub_ = nh_.advertise<voxblox_msgs::Mesh>("active_mesh", 1, true);
    }

    mesh_viz_pub_ = nh_.advertise<voxblox_msgs::Mesh>("mesh_viz", 1, true);

    layer_pub_ = nh_.advertise<kimera_topology::ActiveLayer>("active_layer", 2, false);

    update_timer_ = nh_.createTimer(
        ros::Duration(config_.update_period_s),
        [&](const ros::TimerEvent& event) { runUpdate(event.current_real); });
  }

  void spin() const { ros::spin(); }

 private:
  void setupLayers() {
    // this intentionally disables marching cubes in the native voxblox server
    nh_.setParam("update_mesh_every_n_sec", 0.0);
    // TODO(nathan) explicit configs
    tsdf_server_.reset(new TsdfServerType(ros::NodeHandle(), nh_));

    tsdf_layer_ = tsdf_server_->getTsdfMapPtr()->getTsdfLayerPtr();
    CHECK_NOTNULL(tsdf_layer_);

    gvd_layer_.reset(
        new Layer<GvdVoxel>(tsdf_layer_->voxel_size(), tsdf_layer_->voxels_per_side()));
    mesh_layer_.reset(new MeshLayer(tsdf_layer_->block_size()));

    gvd_integrator_.reset(
        new GvdIntegrator(gvd_config_, tsdf_layer_, gvd_layer_, mesh_layer_));
  }

  void setupConfig(const std::string& config_ns) {
    gvd_config_ = config_parser::load_from_ros<GvdIntegratorConfig>(config_ns);
    config_ = config_parser::load_from_ros<TopologyServerConfig>(config_ns);
  }

  void publishMesh(const ros::Time& timestamp, const BlockIndexList& archived_blocks) {
    voxblox_msgs::Mesh mesh_msg;
    generateVoxbloxMeshMsg(mesh_layer_, config_.mesh_color_mode, &mesh_msg);
    mesh_msg.header.frame_id = config_.world_frame;
    mesh_msg.header.stamp = timestamp;
    mesh_viz_pub_.publish(mesh_msg);

    auto iter = mesh_msg.mesh_blocks.begin();
    while (iter != mesh_msg.mesh_blocks.end()) {
      // we can't just check if the message is empty (it's valid for an observed and
      // active block to be empty), so we have to check if the GVD layer has pruned the
      // corresponding block yet)
      BlockIndex idx(iter->index[0], iter->index[1], iter->index[2]);
      if (!gvd_layer_->hasBlock(idx)) {
        iter = mesh_msg.mesh_blocks.erase(iter);
        continue;
      }

      ++iter;
    }

    if (!config_.publish_archived) {
      mesh_pub_.publish(mesh_msg);
      return;
    }

    voxblox_msgs::Mesh archived_msg;
    for (const auto& block_idx : archived_blocks) {
      voxblox_msgs::MeshBlock block;
      block.index[0] = block_idx.x();
      block.index[1] = block_idx.y();
      block.index[2] = block_idx.z();
      archived_msg.mesh_blocks.push_back(block);
    }

    kimera_topology::ActiveMesh msg;
    msg.header.stamp = timestamp;
    msg.mesh = mesh_msg;
    msg.archived_blocks = archived_msg;
    mesh_pub_.publish(msg);
  }

  void publishActiveLayer(const ros::Time& timestamp) const {
    kimera_topology::ActiveLayer msg;
    msg.header.stamp = timestamp;
    msg.header.frame_id = config_.world_frame;

    // non-const, as clearDeletedNodes modifies internal state
    GraphExtractor& extractor = gvd_integrator_->getGraphExtractor();
    std::unordered_set<NodeId> active_nodes = extractor.getActiveNodes();
    std::unordered_set<NodeId> removed_nodes = extractor.getDeletedNodes();
    extractor.clearDeletedNodes();
    msg.layer_contents = extractor.getGraph().serializeLayer(active_nodes);
    msg.deleted_nodes.insert(
        msg.deleted_nodes.begin(), removed_nodes.begin(), removed_nodes.end());
    layer_pub_.publish(msg);

    for (const auto& id : active_nodes) {
      auto& attr =
          extractor.getGraph().getNode(id)->get().attributes<PlaceNodeAttributes>();
      for (const auto& connection : attr.voxblox_mesh_connections) {
        BlockIndex idx = Eigen::Map<const BlockIndex>(connection.block);
        // mesh api is stupid and logs warnings...
        if (!gvd_layer_->hasBlock(idx)) {
          continue;
        }
        CHECK(connection.vertex < mesh_layer_->getMeshByIndex(idx).size())
            << "invalid vertex @ " << idx.transpose() << " -> " << connection.vertex
            << " >= " << mesh_layer_->getMeshByIndex(idx).size() << " for "
            << NodeSymbol(id).getLabel();
      }
    }
  }

  void showStats(const ros::Time& timestamp) const {
    LOG(INFO) << "Timings: (stamp: " << timestamp.toNSec() << ")" << std::endl
              << voxblox::timing::Timing::Print();
    const std::string tsdf_memory_str =
        hydra_utils::getHumanReadableMemoryString(tsdf_layer_->getMemorySize());
    const std::string gvd_memory_str =
        hydra_utils::getHumanReadableMemoryString(gvd_layer_->getMemorySize());
    const std::string mesh_memory_str =
        hydra_utils::getHumanReadableMemoryString(mesh_layer_->getMemorySize());
    LOG(INFO) << "Memory used: [TSDF=" << tsdf_memory_str << ", GVD=" << gvd_memory_str
              << ", Mesh= " << mesh_memory_str << "]";
  }

  void runUpdate(const ros::Time& timestamp) {
    if (!tsdf_layer_ || tsdf_layer_->getNumberOfAllocatedBlocks() == 0) {
      return;
    }

    gvd_integrator_->updateFromTsdfLayer(true);

    BlockIndexList archived_blocks;
    if (config_.clear_distant_blocks && tsdf_server_->has_pose) {
      archived_blocks =
          gvd_integrator_->removeDistantBlocks(tsdf_server_->T_G_C_last.getPosition(),
                                               config_.dense_representation_radius_m);

      // this needs to be paired with publishMesh (i.e. generateVoxbloxMeshMsg)
      // to actual remove allocated blocks (instead of getting rid of the contents).
      // handled through publishMesh
      mesh_layer_->clearDistantMesh(tsdf_server_->T_G_C_last.getPosition(),
                                    config_.dense_representation_radius_m);
    }

    publishMesh(timestamp, archived_blocks);
    publishActiveLayer(timestamp);

    visualizer_->visualize(gvd_integrator_->getGraphExtractor(),
                           gvd_integrator_->getGraph(),
                           *gvd_layer_,
                           *tsdf_layer_);

    if (config_.show_stats) {
      showStats(timestamp);
    }
  }

 private:
  ros::NodeHandle nh_;

  TopologyServerConfig config_;
  GvdIntegratorConfig gvd_config_;

  std::unique_ptr<TopologyServerVisualizer> visualizer_;

  ros::Publisher mesh_viz_pub_;
  ros::Publisher mesh_pub_;
  ros::Publisher layer_pub_;

  Layer<TsdfVoxel>* tsdf_layer_;
  Layer<GvdVoxel>::Ptr gvd_layer_;
  MeshLayer::Ptr mesh_layer_;

  std::unique_ptr<TsdfServerType> tsdf_server_;
  std::unique_ptr<GvdIntegrator> gvd_integrator_;

  ros::Timer update_timer_;
};

}  // namespace topology
}  // namespace kimera
