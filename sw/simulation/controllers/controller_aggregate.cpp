#include "controller_keep_aggregate.h"
#include "agent.h"
#include "particle.h"
#include "main.h"
#include "randomgenerator.h"
#include "auxiliary.h"

#include <algorithm> // std::sort

Controller_Aggregate::Controller_Aggregate() : Controller(),
moving(nagents,0),
moving_timer(nagents,0),
selected_action(nagents,-1)
{
  state_action_matrix.clear();
  terminalinfo ti;
  ifstream state_action_matrix_file("./conf/state_action_matrices/state_action_matrix_free.txt");

  if (state_action_matrix_file.is_open()) {
    ti.info_msg("Opened state action matrix file.");

    // Collect the data inside the a stream, do this line by line
    while (!state_action_matrix_file.eof()) {
      string line;
      getline(state_action_matrix_file, line);
      stringstream stream_line(line);
      int buff[10] = {};
      int columns = 0;
      int state_index;
      bool index_checked = false;
      while (!stream_line.eof()) {
        if (!index_checked) {
          stream_line >> state_index;
          index_checked = true;
        } else {
          stream_line >> buff[columns];
          columns++;
        }
      }
      if (columns > 0) {
        vector<int> actions_index( begin(buff), begin(buff) + columns );
        state_action_matrix.insert( pair<int, vector<int>>(state_index, actions_index) );
      }
    }
    state_action_matrix_file.close();
  }
  else {
    ti.debug_msg("Unable to open state action matrix file.");
  }

}

float Controller_Aggregate::f_attraction(float u, float b_eq)
{
  float w = log((_ddes / _kr - 1) / exp(-_ka * _ddes)) / _ka;
  return 1 / (1 + exp(-_ka * (u - w)));
}

float Controller_Aggregate::f_repulsion(float u) { return -_kr / u; }
float Controller_Aggregate::f_extra(float u) {return 0;}

float Controller_Aggregate::get_attraction_velocity(float u, float b_eq)
{
  return f_attraction(u, b_eq) + f_repulsion(u) + f_extra(u);;
}

void Controller_Aggregate::attractionmotion(const float &v_r, const float &v_b, float &v_x, float &v_y)
{
  v_x = v_r * cos(v_b);
  v_y = v_r * sin(v_b);
}

void Controller_Aggregate::latticemotion(const float &v_r, const float &v_adj, const float &v_b, const float &bdes, float &v_x, float &v_y)
{
  attractionmotion(v_r+v_adj, v_b, v_x, v_y);
  // Additional force for for reciprocal alignment
  v_x += -v_adj * cos(bdes * 2 - v_b);
  v_y += -v_adj * sin(bdes * 2 - v_b);
}

void Controller_Aggregate::actionmotion(const int selected_action, float &v_x, float &v_y)
{
  int actionspace_x[8] = {1, 1, 0, -1, -1, -1, 0, 1};
  int actionspace_y[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  v_x = _v_adj * (float)actionspace_x[selected_action];
  v_y = _v_adj * (float)actionspace_y[selected_action];
}


bool Controller_Aggregate::fill_template(vector<bool> &q, const float b_i, const float u, float dmax)
{
  vector<float> blink;

  // Angles to check for neighboring links
  blink.push_back(0);
  blink.push_back(M_PI / 4.0);
  blink.push_back(M_PI / 2.0);
  blink.push_back(3 * M_PI / 4.0);
  blink.push_back(M_PI);
  blink.push_back(deg2rad(180 + 45));
  blink.push_back(deg2rad(180 + 90));
  blink.push_back(deg2rad(180 + 135));
  blink.push_back(2 * M_PI);

  // Determine link (cycle through all options)
  if (u < dmax) {
    for (int j = 0; j < (int)blink.size(); j++) {
      if (abs(b_i - blink[j]) < deg2rad(22.49)) {
        if (j == (int)blink.size() - 1) { // last element is back to 0
          q[0] = true;
        } else {
          q[j] = true;
        }
        return true;
      }
    }
  }
  return false;
}

float Controller_Aggregate::get_preferred_bearing(const vector<float> &bdes, const float v_b)
{
  // Define in bv all equilibrium angles at which the agents can organize themselves
  vector<float> bv;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < (int)bdes.size(); j++) {
      bv.push_back(bdes[j]);
    }
  }

  // Find what the desired angle is in bdes
  for (int i = 0; i < (int)bv.size(); i++) {
    if (i < (int)bdes.size() * 1) {
      bv[i] = abs(bv[i] - 2 * M_PI - v_b);
    } else if (i < (int)bdes.size() * 2) {
      bv[i] = abs(bv[i] - M_PI - v_b);
    } else if (i < (int)bdes.size() * 3) {
      bv[i] = abs(bv[i] - v_b);
    } else if (i < (int)bdes.size() * 4) {
      bv[i] = abs(bv[i] + M_PI - v_b);
    } else if (i < (int)bdes.size() * 5) {
      bv[i] = abs(bv[i] + 2 * M_PI - v_b);
    }
  }

  int minindex = 0;
  for (int i = 1; i < (int)bv.size(); i++) {
    if (bv[i] < bv[minindex]) {
      minindex = i;
    }
  }

  // Reduce the index for the angle of interest from bdes
  while (minindex >= (int)bdes.size()) {
    minindex -= (int)bdes.size();
  }

  // Returned the desired equilibrium bearing
  return bdes[minindex];
}

void Controller_Aggregate::assess_situation(uint8_t ID, vector<bool> &q, vector<int> &q_ID)
{
  q.clear();
  q.assign(8,false);

  vector<int> closest = o->request_closest(ID); // Get vector of all neighbor IDs from closest to furthest

  // Fill the template with respect to the agent in question
  for (uint8_t i = 0; i < nagents - 1; i++) {
    if (fill_template(q, // Vector to fill
          wrapTo2Pi_f(o->request_bearing(ID, closest[i])), // Bearing
          o->request_distance(ID, closest[i]), // Distance
          _ddes*1.5)) { // Sensor range
            q_ID.push_back(closest[i]);
          }
  }
}

void Controller_Aggregate::get_velocity_command(const uint8_t ID, float &v_x, float &v_y)
{
  v_x = 0;
  v_y = 0;
  
  vector<float> beta_des;
  beta_des.push_back(0.0);
  beta_des.push_back(M_PI/4.0);
  beta_des.push_back(M_PI/2.0);
  beta_des.push_back(3.0*M_PI/4.0);

  vector<int> closest = o->request_closest(ID); // Get vector of all neighbors from closest to furthest
  float v_b = wrapToPi_f(o->request_bearing(ID, closest[0]));
  float b_eq = get_preferred_bearing(beta_des, v_b);
  float v_r = get_attraction_velocity(o->request_distance(ID, closest[0]), b_eq);

  // State
  vector<bool> state(8, 0);
  vector<int>  state_ID;
  assess_situation(ID, state, state_ID); // The ID is just used for simulation purposes
  int state_index = bool2int(state);

  // Can I move or are my neighbors moving?
  bool canImove = true;
  for (uint8_t i = 0; i < state_ID.size(); i++) {
    if (moving[state_ID[i]]) { // Somebody nearby is already moving
      canImove = false;
    }
  }

  // Action
  std::map<int, vector<int>>::iterator state_action_row;
  state_action_row = state_action_matrix.find(state_index);
  
  // Try to find an action that suits the state, if available (otherwise you are in Sdes or Sblocked)
  // If you are already busy with an action, then don't change the action
  if (!moving[ID]) {
    if (state_action_row != state_action_matrix.end()) {
      if (motion_dir == 0 || state_action_row->second.size() < 2) {
        selected_action[ID] = *select_randomly(state_action_row->second.begin(),
                                               state_action_row->second.end());
      }
      else {
        // Possible actions
        vector<int> possibleactions = state_action_row->second;
        for_each(possibleactions.begin(), possibleactions.end(), [](int &d) { d++; });

        // Difference to favorite action
        vector<int> difference = state_action_row->second;
        for_each(difference.begin(), difference.end(), [](int &d) { d++; });
        for_each(difference.begin(), difference.end(), [&](int &d) { d -= motion_dir; }); // https://stackoverflow.com/questions/13461538/lambda-of-a-lambda-the-function-is-not-captured/13461594
        for_each(difference.begin(), difference.end(), [](int &d) { d = abs(d); });
        for_each(difference.begin(), difference.end(), [](int &d)
          { if (d>4) {d = 4-wraptosequence(d,0,4);} else {d = wraptosequence(d,0,4);} });
        
        // Sort
        std::pair<int, int> AB[difference.size()];
        for (size_t i = 0; i < difference.size(); ++i) {
          AB[i].first = difference[i];
          AB[i].second = possibleactions[i];
        }
        std::sort(AB, AB + difference.size());
        for (size_t i = 0; i < difference.size(); ++i) {
          difference[i] = AB[i].first;
          possibleactions[i] = AB[i].second;
        }

        // Select
        if (difference[0] != difference[1]) {
          selected_action[ID] = possibleactions[0] - 1;
          }
        else {
          selected_action[ID] = *select_randomly(possibleactions.begin(),
                                                 possibleactions.begin() + 1) - 1;
        }
      }
                                            
    } else {
      selected_action[ID] = -2; // State not found... no action to take.
    }
  }
  
  moving[ID] = false;
  if (selected_action[ID] > -1 && canImove && moving_timer[ID] < 200) {
    // You are in not blocked and you have priority. Take an action!
    actionmotion(selected_action[ID], v_x, v_y);
    moving[ID] = true;
    moving_timer[ID]++;
  } else if (canImove) {
    // You are static, but you still have priority! Fix your position.
    latticemotion(v_r, _v_adj, v_b, b_eq, v_x, v_y);
    moving_timer[ID] = 0;
  } else {
    // You are static, but also too slow, so no priority! Wait about till you do.
    attractionmotion(v_r, v_b, v_x, v_y);
    moving_timer[ID] = 0;
  }

  keepbounded(v_x, -1, 1);
  keepbounded(v_y, -1, 1);
}
