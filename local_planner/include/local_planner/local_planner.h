#ifndef LOCAL_PLANNER_LOCAL_PLANNER_H
#define LOCAL_PLANNER_LOCAL_PLANNER_H

#include <sensor_msgs/image_encodings.h>
#include "avoidance_output.h"
#include "box.h"
#include "candidate_direction.h"
#include "cost_parameters.h"
#include "histogram.h"

#include <dynamic_reconfigure/server.h>
#include <local_planner/LocalPlannerNodeConfig.h>

#include <Eigen/Dense>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <tf/transform_listener.h>

#include <sensor_msgs/Image.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/PointCloud2.h>

#include <nav_msgs/GridCells.h>
#include <nav_msgs/Path.h>

#include <ros/time.h>
#include <deque>
#include <string>
#include <vector>

namespace avoidance {

class StarPlanner;
class TreeNode;

class LocalPlanner {
 private:
  bool use_back_off_;
  bool use_VFH_star_;
  bool adapt_cost_params_;
  bool stop_in_front_;

  bool reach_altitude_ = false;
  bool obstacle_ = false;
  bool first_brake_ = true;
  bool waypoint_outside_FOV_ = false;
  bool back_off_ = false;
  bool hist_is_empty_ = false;

  int e_FOV_max_, e_FOV_min_;
  size_t dist_incline_window_size_ = 50;
  int origin_;
  int tree_age_ = 0;
  int children_per_node_;
  int n_expanded_nodes_;
  int reproj_age_;
  int counter_close_points_backoff_ = 0;

  float curr_yaw_;
  float curr_pitch_;
  float velocity_around_obstacles_;
  float velocity_far_from_obstacles_;
  float keep_distance_;
  ros::Time integral_time_old_;
  float no_progress_slope_;
  float new_yaw_;
  float distance_to_closest_point_;
  int min_cloud_size_ = 160;
  float min_dist_backoff_;
  float relevance_margin_z_degree_ = 40.0f;
  float relevance_margin_e_degree_ = 25.0f;
  float velocity_sigmoid_slope_ = 1.0;
  float min_realsense_dist_ = 0.2f;
  float costmap_direction_e_;
  float costmap_direction_z_;

  waypoint_choice waypoint_type_;
  ros::Time last_path_time_;

  std::vector<int> e_FOV_idx_;
  std::vector<int> z_FOV_idx_;
  std::deque<float> goal_dist_incline_;
  std::vector<float> cost_path_candidates_;
  std::vector<int> cost_idx_sorted_;
  std::vector<int> closed_set_;
  std::vector<int> reprojected_points_age_;

  std::vector<TreeNode> tree_;
  std::unique_ptr<StarPlanner> star_planner_;
  costParameters cost_params_;

  pcl::PointCloud<pcl::PointXYZ> reprojected_points_, final_cloud_;

  Eigen::Vector3f position_ = Eigen::Vector3f::Zero();
  Eigen::Vector3f velocity_ = Eigen::Vector3f::Zero();
  Eigen::Vector3f goal_ = Eigen::Vector3f::Zero();
  Eigen::Vector3f back_off_point_ = Eigen::Vector3f::Zero();
  Eigen::Vector3f back_off_start_point_ = Eigen::Vector3f::Zero();
  Eigen::Vector3f position_old_ = Eigen::Vector3f::Zero();
  Eigen::Vector3f closest_point_ = Eigen::Vector3f::Zero();

  Histogram polar_histogram_ = Histogram(ALPHA_RES);
  Histogram to_fcu_histogram_ = Histogram(ALPHA_RES);
  Eigen::MatrixXf cost_matrix_;
  std::vector<candidateDirection> candidate_vector_;

  /**
  * @brief     reprojectes the histogram from the previous algorithm iteration
  *around the current vehicle position
  * @param     histogram, histogram from the previous algorith iteration
  **/
  void reprojectPoints(Histogram histogram);
  /**
  * @brief     calculates the cost function weights to fly around or over
  *obstacles based on the progress towards the goal over time
  **/
  void evaluateProgressRate();
  /**
  * @brief     stops the vehicle in front of an obstacle
  **/
  void stopInFrontObstacles();
  /**
  * @brief     fills message to send histogram to the FCU
  **/
  void updateObstacleDistanceMsg(Histogram hist);
  /**
  * @brief      fills message to send empty histogram to the FCU
  **/
  void updateObstacleDistanceMsg();
  /**
  * @brief      creates a polar histogram representation of the pointcloud
  * @params[in] send_to_fcu, true if the histogram is sent to the FCU
  **/
  void create2DObstacleRepresentation(const bool send_to_fcu);
  /**
  * @brief     generates an image represention of the polar histogram
  * @param     histogram, polar histogram representing obstacles
  * @returns   histogram image
  **/
  sensor_msgs::Image generateHistogramImage(Histogram &histogram);

 public:
  float h_FOV_ = 59.0f;
  float v_FOV_ = 46.0f;
  Box histogram_box_;
  sensor_msgs::Image histogram_image_;
  bool use_vel_setpoints_;
  bool currently_armed_ = false;
  bool offboard_ = false;
  bool mission_ = false;
  bool smooth_waypoints_ = true;
  bool send_obstacles_fcu_ = false;
  bool stop_in_front_active_ = false;
  bool disable_rise_to_goal_altitude_ = false;

  double timeout_critical_;
  double timeout_termination_;
  double starting_height_ = 0.0;
  float speed_ = 1.0f;
  float ground_distance_ = 2.0;

  Eigen::Vector3f take_off_pose_ = Eigen::Vector3f::Zero();
  ;
  sensor_msgs::LaserScan distance_data_ = {};
  Eigen::Vector3f last_sent_waypoint_ = Eigen::Vector3f::Zero();

  // complete_cloud_ contains n complete clouds from the cameras
  std::vector<pcl::PointCloud<pcl::PointXYZ>> complete_cloud_;

  LocalPlanner();
  ~LocalPlanner();

  /**
  * @brief     setter method for vehicle position
  * @param[in] pos, vehicle position message coming from the FCU
  * @param[in] q, vehicle orientation message coming from the FCU
  **/
  void setPose(const Eigen::Vector3f &pos, const Eigen::Quaternionf &q);
  /**
  * @brief     setter method for mission goal
  * @param[in] mgs, goal message coming from the FCU
  **/
  void setGoal(const Eigen::Vector3f &goal);
  /**
  * @brief     getter method for current goal
  * @returns   position of the goal
  **/
  Eigen::Vector3f getGoal();
  /**
  * @brief    setter method for mission goal
  **/
  void applyGoal();
  /**
  * @brief     sets parameters from ROS parameter server
  * @param     config, struct containing all the paramters
  * @param     level, bitmsak to group together reconfigurable parameters
  **/
  void dynamicReconfigureSetParams(avoidance::LocalPlannerNodeConfig &config,
                                   uint32_t level);
  /**
  * @brief     getter method for current vehicle position and orientation
  * @returns   vehicle positiona and orientation
  **/
  Eigen::Vector3f getPosition();

  /**
  * @brief     getter method to visualize pointcloud in rviz
  * @param     final_cloud, filtered pointcloud from the current camera frame
  * @param     reprojected_points, pointcloud saved from previous frames
  **/
  void getCloudsForVisualization(
      pcl::PointCloud<pcl::PointXYZ> &final_cloud,
      pcl::PointCloud<pcl::PointXYZ> &reprojected_points);
  /**
  * @brief     setter method for vehicle velocity
  * @param[in]     vel, velocity message coming from the FCU
  **/
  void setCurrentVelocity(const Eigen::Vector3f &vel);

  /**
  * @brief     getter method to visualize the tree in rviz
  * @param[in]     closed_set, velocity message coming from the FCU
  * @param[in]     path_node_positions, velocity message coming from the FCU
  **/
  void getTree(std::vector<TreeNode> &tree, std::vector<int> &closed_set,
               std::vector<Eigen::Vector3f> &path_node_positions);
  /**
  * @brief     setter method to send obstacle distance information to FCU
  * @param[in]     obstacle_distance, obstacle distance message
  **/
  void sendObstacleDistanceDataToFcu(sensor_msgs::LaserScan &obstacle_distance);

  /**
  * @brief     getter method of the local planner algorithm
  * @param[in]     output of a local planner iteration
  **/
  avoidanceOutput getAvoidanceOutput();

  /**
  * @brief     determines the way the obstacle is avoided and the algorithm to
  *use
  **/
  void determineStrategy();
  /**
  * @brief     starts a iteration of the local planner algorithm
  **/
  void runPlanner();
};
}

#endif  // LOCAL_PLANNER_H
