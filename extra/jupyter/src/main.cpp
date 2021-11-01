
#include "codon.h"
#include <memory>
#include <string>
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus/xserver_zmq.hpp>

using std::make_unique;
using std::move;
using std::string;

int main(int argc, char *argv[]) {
  string file_name = (argc == 1) ? "connection.json" : argv[2];
  xeus::xconfiguration config = xeus::load_configuration(file_name);

  auto context = xeus::make_context<zmq::context_t>();
  auto interpreter = make_unique<codon::CodonJupyter>();
  xeus::xkernel kernel(config, xeus::get_user_name(), move(context), move(interpreter),
                       xeus::make_xserver_zmq);
  kernel.start();

  return 0;
}