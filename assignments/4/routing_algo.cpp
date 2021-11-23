#include "node.h"
#include <iostream>

using namespace std;

void printRT(vector<RoutingNode*> nd){
/*Print routing table entries*/
	for (int i = 0; i < nd.size(); i++) {
	  nd[i]->printTable();
	}
}

void routingAlgo(vector<RoutingNode*> nd){
  //Your code here


  /*Print routing table entries after routing algo converges */
  printRT(nd);
}


void RoutingNode::recvMsg(RouteMsg *msg) {
  //your code here
}




