#include "wall_avoidance.h"
#include "main.h"

// #include

wall_avoidance::wall_avoidance(){
    multi_ranger = MultiRanger();
}

Eigen::Vector3f wall_avoidance::get_velocity_cmd(const uint16_t ID){
    Eigen::Vector3f v_curr = s[ID]->state.vel;
    // std::vector<int> range_idx;
    std::vector<float> ranges = multi_ranger.getMeasurements(s[ID]->state.pose);
    Eigen::Vector3f v_coll = Eigen::Vector3f::Zero();
    for (uint i=0; i < ranges.size(); i++){
        if (ranges[i]!=10000){ // if nothing is sensed the range is 10000. change this
            Eigen::Vector3f v_shill_cap = multi_ranger._rangers[i].getAvoidDirection();
            
            float v_diff_mag = (v_curr.normalized() - v_shill_cap).norm();
            // float gain = brake_decay();
            // v_coll += = nonlin_idx_max(v_shill_cap, Eigen::Vector3f::Zero(), 15, multi_ranger._rangers[i].range(),0,abs(multi_ranger._rangers[i].range()-ranges[i]), 0.3);
            v_coll += nonlin_idx(v_shill_cap, Eigen::Vector3f::Zero(), 15, multi_ranger._rangers[i].range(), ranges[i], 0.3);;
            // print(i, " coll speed: ", v_coll_i.transpose(), " dist: ", ranges[i]);
        }
    }

    return v_coll;
}

void wall_avoidance::animation(const uint16_t ID){
    draw d;
    multi_ranger.animate(d);
}