#include "tcp_node.hh"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << "Only node index is needed." << endl;
    exit(-1);
  }
  TCPNode node(strtoul(argv[1], nullptr, 10));
  node.start();
  return 0;
}