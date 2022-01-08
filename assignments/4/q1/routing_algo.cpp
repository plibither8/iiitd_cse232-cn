#include "node.h"
#include <iostream>

using namespace std;

void printRT(vector<RoutingNode *> nd) {
  /*Print routing table entries*/
  for (int i = 0; i < nd.size(); i++) {
    nd[i]->printTable();
  }
}

void routingAlgo(vector<RoutingNode *> nd) {
  bool changed = false;
  vector<struct routingtbl> rt_copy;

  do {
    changed = false;
    // Clear the copy and copy the routing table
    rt_copy.clear();
    for (auto &node : nd)
      rt_copy.push_back(node->getTable());
    // Send a message from all nodes
    for (auto &node : nd)
      node->sendMsg();
    // Check if the routing table has changed
    for (int i = 0; i < nd.size() && !changed; i++) {
      vector<RoutingEntry> old_rt = rt_copy[i].tbl,
                           new_rt = nd[i]->getTable().tbl;
      changed = (old_rt.size() != new_rt.size());
      for (int j = 0; j < old_rt.size() && !changed; j++)
        changed = (old_rt[j].ip_interface != new_rt[j].ip_interface ||
                   old_rt[j].nexthop != new_rt[j].nexthop);
    }
  } while (changed);

  /*Print routing table entries after routing algo converges */
  printRT(nd);
}

void RoutingNode::recvMsg(RouteMsg *msg) {
  // Iterate through all the entries received in the tables,
  // compare it with those that I already have of this node,
  // and for those that are in common, apply BF and for those
  // that are not, add an entry in my table for it
  for (auto &their_entry : msg->mytbl->tbl) {
    bool is_common_entry = false;

    for (auto &my_entry : mytbl.tbl) {
      if (their_entry.dstip == my_entry.dstip) {
        is_common_entry = true;
        // If a lower cost alternative has been found through this entry, we
        // shall update my entry table (this is Bellman-Ford)
        if (my_entry.nexthop == msg->from)
          my_entry.cost = their_entry.cost + 1;
        else if (their_entry.cost + 1 < my_entry.cost) {
          my_entry.ip_interface = msg->from;
          my_entry.nexthop = their_entry.ip_interface;
          my_entry.cost = their_entry.cost + 1;
        }
        break;
      }
    }

    // If no common entry is found, but we know a route still exists
    // we will add it to our entry table
    if (!is_common_entry) {
      RoutingEntry new_entry;
      new_entry.dstip = their_entry.dstip;
      new_entry.cost =
          msg->from == their_entry.dstip ? 1 : their_entry.cost + 1;
      new_entry.nexthop = msg->from;
      new_entry.ip_interface = msg->recvip;
      mytbl.tbl.push_back(new_entry);
    }
  }
}
