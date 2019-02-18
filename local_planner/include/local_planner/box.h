
#ifndef BOX_H
#define BOX_H

#include <geometry_msgs/Point.h>
namespace avoidance {
class Box {
 public:
  Box();
  Box(const double& radius);
  ~Box() = default;

  /**
  * @brief     sets the bounding box coordinates limits around the vehicle
  *position
  * @param[in] pos, vehicle current position
  * @param[in] ground_distance, distance to the ground [m]
  **/
  void setBoxLimits(const geometry_msgs::Point& pos,
                    const double ground_distance);

  /**
  * @brief     checks if a pointcloud point is within the bounding box
  * @param[in] x, x-coordinate of the point
  * @param[in] y, y-coordinate of the point
  * @param[in] z, z-coordinate of the point
  * @returns   true, if the point is within the bounding box
  **/
  bool isPointWithinBox(const double& x, const double& y, const double& z);

  double radius_;
  double box_dist_to_ground_ = 2.0;
  double zmin_;

 private:
  double xmin_;
  double xmax_;
  double ymin_;
  double ymax_;
  double zmax_;
};
}
#endif  // BOX_H
