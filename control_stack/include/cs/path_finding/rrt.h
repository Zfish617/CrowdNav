#pragma once
// Copyright 2020 kvedder@seas.upenn.edu
// School of Engineering and Applied Sciences,
// University of Pennsylvania
//
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ========================================================================

#include <math.h>
#include <boost/functional/hash.hpp>

#include <algorithm>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include <list>

#include "cs/path_finding/path_finder.h"
#include "cs/util/constants.h"
#include "cs/util/visualization.h"
#include "libMultiRobotPlanning/bounded_a_star.hpp"
#include "shared/math/geometry.h"
#include "shared/math/math_util.h"

namespace cs {
namespace path_finding {

namespace params {
CONFIG_INT(num_samples, "rrt.num_samples");
CONFIG_FLOAT(cost_bias, "rrt.cost_bias");
CONFIG_FLOAT(path_length, "rrt.path_length");
}  // namespace params

template <int max_samples>
class RRT : public PathFinder {
 private:
  std::vector<Path2f> paths_;

  // Calculates euclidian distance from path end point to goal
  double euclid_dist_(const Path2f& path, const Eigen::Vector2f& goal) {
    return sqrt(pow(path.waypoints.back()[0] - goal[0], 2) + pow(path.waypoints.back()[1] - goal[1], 2));
  }

  // TODO: Calculates to probability of colliding with a pedestrian along a path
  float find_collision_prob_(const ped_detection::PedDetector& ped_detector, const Path2f& path) {
    (void) ped_detector;
    (void) path;
    return -1;
  }


  // Calculates cost for a path based on its euclidian distance to the goal and the probability of
  // colliding with a pedestrian along the path
  void calculate_cost_(
    Path2f& path,
    const ped_detection::PedDetector& ped_detector, 
    const Eigen::Vector2f& goal) {
      float collision_prob = find_collision_prob_(ped_detector, path);
      float dist_from_goal = euclid_dist_(path, goal);
      float cost = dist_from_goal + params::CONFIG_cost_bias * collision_prob;
      path.collision_prob = collision_prob;
      path.dist_from_goal = dist_from_goal;
      path.cost  = cost;

  }
 public:
  explicit RRT(const util::vector_map::VectorMap& map,
                 const float& robot_radius,
                 const float& safety_margin,
                 const float& inflation)
      : PathFinder(map, robot_radius, safety_margin, inflation), paths_() {}


  // Calculates the cost for each sampled path and returns the lowest cost path
  Path2f FindPath(const ped_detection::PedDetector& ped_detector,
                  const Eigen::Vector2f& start,
                  const Eigen::Vector2f& goal) {    
    (void) goal;
    (void) ped_detector;
    paths_.clear();

    // Samples paths around the clock based on number of samples
    for (int i = 0; i < params::CONFIG_num_samples; i++) {
      float theta = 2 * i * M_PI / params::CONFIG_num_samples;
      const Eigen::Vector2f sample(params::CONFIG_path_length * sin(theta), 
                                   params::CONFIG_path_length * cos(theta));
      Path2f path;
      path.waypoints.push_back(start);
      path.waypoints.push_back(start + sample);

      calculate_cost_(path, ped_detector, goal);
      paths_.push_back(path);
      // ROS_INFO("Goal: x: %f, y: %f ", goal[0], goal[1]);
      // ROS_INFO("Path: x: %f, y: %f, dist: %f, cost: %f", path.waypoints[1][0], path.waypoints[1][1], path.dist_from_goal, path.cost);
    }


    // Find the minimum cost path
    Path2f min_cost_path = *std::min_element(begin(paths_), end(paths_),
                                    [](const Path2f& a, const Path2f& b){
        return a.cost < b.cost;
    });
    // ROS_INFO("start: %f, %f", start[0], start[1]);
    // ROS_INFO("Path waypoint size: %i", (int) paths_.size());

    prev_path_ = min_cost_path;
    // ROS_INFO("Path Start: x: %f, y: %f", min_cost_path.waypoints[0][0], min_cost_path.waypoints[0][1]);
    // ROS_INFO("Path End: x: %f, y: %f", min_cost_path.waypoints[1][0], min_cost_path.waypoints[1][1]);
    return min_cost_path;
    //return SmoothPath(start, dynamic_map, path);
  }

  std::vector<Path2f> GetCandidatePaths(int num_paths) override {
    return std::vector<Path2f>(paths_.begin(), paths_.begin() + num_paths);
  }
};

}  // namespace path_finding
}  // namespace cs