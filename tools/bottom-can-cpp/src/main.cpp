#ifdef __linux__

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

constexpr canid_t kIdRpy = 0x210;
constexpr canid_t kIdAccel = 0x211;

struct Args {
  std::string command;
  std::string interface = "can0";
  double rate_hz = 100.0;
  bool once = false;
  bool print = false;
  bool demo = false;
  double roll = 0.0;
  double pitch = 0.0;
  double yaw = 0.0;
  double ax = 0.0;
  double ay = 0.0;
  double az = 9.81;
  uint8_t limits = 0;
  uint8_t report_id = 0x28;
};

int16_t clamp_i16(int value) {
  return static_cast<int16_t>(std::max(-32768, std::min(32767, value)));
}

int16_t scale_i16(double value, double scale) {
  return clamp_i16(static_cast<int>(std::lround(value * scale)));
}

void put_i16_be(std::array<uint8_t, 8> &data, size_t offset, int16_t value) {
  auto raw = static_cast<uint16_t>(value);
  data[offset] = static_cast<uint8_t>(raw >> 8);
  data[offset + 1] = static_cast<uint8_t>(raw & 0xFF);
}

int16_t get_i16_be(const std::array<uint8_t, 8> &data, size_t offset) {
  uint16_t raw = (static_cast<uint16_t>(data[offset]) << 8) |
                 static_cast<uint16_t>(data[offset + 1]);
  return static_cast<int16_t>(raw);
}

std::array<uint8_t, 8> pack_rpy(double roll, double pitch, double yaw,
                                uint8_t limits, uint8_t sequence) {
  std::array<uint8_t, 8> data{};
  put_i16_be(data, 0, scale_i16(roll, 100.0));
  put_i16_be(data, 2, scale_i16(pitch, 100.0));
  put_i16_be(data, 4, scale_i16(yaw, 100.0));
  data[6] = limits & 0x0F;
  data[7] = sequence;
  return data;
}

std::array<uint8_t, 8> pack_accel(double ax, double ay, double az,
                                  uint8_t limits, uint8_t report_id) {
  std::array<uint8_t, 8> data{};
  put_i16_be(data, 0, scale_i16(ax, 1000.0));
  put_i16_be(data, 2, scale_i16(ay, 1000.0));
  put_i16_be(data, 4, scale_i16(az, 1000.0));
  data[6] = limits & 0x0F;
  data[7] = report_id;
  return data;
}

std::string hex_byte(uint8_t value) {
  std::ostringstream out;
  out << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
      << static_cast<int>(value);
  return out.str();
}

std::string data_hex(const std::array<uint8_t, 8> &data) {
  std::ostringstream out;
  for (size_t i = 0; i < data.size(); ++i) {
    if (i != 0) {
      out << ' ';
    }
    out << hex_byte(data[i]);
  }
  return out.str();
}

class SocketCan {
public:
  explicit SocketCan(const std::string &interface) {
    fd_ = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd_ < 0) {
      throw std::runtime_error("socket failed: " + std::string(std::strerror(errno)));
    }

    ifreq ifr{};
    std::snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface.c_str());
    if (::ioctl(fd_, SIOCGIFINDEX, &ifr) < 0) {
      throw std::runtime_error("unknown CAN interface " + interface + ": " +
                               std::string(std::strerror(errno)));
    }

    sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
      throw std::runtime_error("bind failed: " + std::string(std::strerror(errno)));
    }
  }

  ~SocketCan() {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  void send_std(canid_t id, const std::array<uint8_t, 8> &data) {
    can_frame frame{};
    frame.can_id = id & CAN_SFF_MASK;
    frame.can_dlc = 8;
    std::copy(data.begin(), data.end(), frame.data);

    ssize_t n = ::write(fd_, &frame, sizeof(frame));
    if (n != static_cast<ssize_t>(sizeof(frame))) {
      throw std::runtime_error("CAN write failed: " + std::string(std::strerror(errno)));
    }
  }

  can_frame read_frame() {
    can_frame frame{};
    ssize_t n = ::read(fd_, &frame, sizeof(frame));
    if (n < 0) {
      throw std::runtime_error("CAN read failed: " + std::string(std::strerror(errno)));
    }
    if (n != static_cast<ssize_t>(sizeof(frame))) {
      throw std::runtime_error("short CAN read");
    }
    return frame;
  }

private:
  int fd_ = -1;
};

void print_usage() {
  std::cout
      << "Usage:\n"
      << "  bottom-can-cpp monitor --interface can0\n"
      << "  bottom-can-cpp send --interface can0 [--print]\n";
}

uint8_t parse_u8(const std::string &value) {
  return static_cast<uint8_t>(std::stoul(value, nullptr, 0) & 0xFF);
}

Args parse_args(int argc, char **argv) {
  Args args;
  if (argc < 2) {
    print_usage();
    std::exit(2);
  }
  args.command = argv[1];

  for (int i = 2; i < argc; ++i) {
    std::string key = argv[i];
    auto need_value = [&](const char *name) -> std::string {
      if (i + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[++i];
    };

    if (key == "--interface" || key == "--if") {
      args.interface = need_value("--interface");
    } else if (key == "--rate-hz") {
      args.rate_hz = std::stod(need_value("--rate-hz"));
    } else if (key == "--once") {
      args.once = true;
    } else if (key == "--print") {
      args.print = true;
    } else if (key == "--demo") {
      args.demo = true;
    } else if (key == "--roll") {
      args.roll = std::stod(need_value("--roll"));
    } else if (key == "--pitch") {
      args.pitch = std::stod(need_value("--pitch"));
    } else if (key == "--yaw") {
      args.yaw = std::stod(need_value("--yaw"));
    } else if (key == "--ax") {
      args.ax = std::stod(need_value("--ax"));
    } else if (key == "--ay") {
      args.ay = std::stod(need_value("--ay"));
    } else if (key == "--az") {
      args.az = std::stod(need_value("--az"));
    } else if (key == "--limits") {
      args.limits = parse_u8(need_value("--limits"));
    } else if (key == "--report-id") {
      args.report_id = parse_u8(need_value("--report-id"));
    } else {
      throw std::runtime_error("unknown argument: " + key);
    }
  }

  if (args.command != "send" && args.command != "monitor") {
    throw std::runtime_error("command must be send or monitor");
  }
  return args;
}

void print_tx(canid_t id, const std::array<uint8_t, 8> &data) {
  std::cout << "TX 0x" << std::uppercase << std::hex << std::setw(3)
            << std::setfill('0') << id << std::dec << " [8] "
            << data_hex(data) << "\n";
}

void run_send(const Args &args) {
  SocketCan can(args.interface);
  auto period = std::chrono::duration<double>(1.0 / args.rate_hz);
  uint8_t sequence = 0;

  std::cout << "Sending 0x210/0x211 on " << args.interface << " at "
            << args.rate_hz << " Hz\n";

  while (true) {
    auto start = std::chrono::steady_clock::now();
    double roll = args.roll;
    double pitch = args.pitch;
    double yaw = args.yaw;

    if (args.demo) {
      double t = std::chrono::duration<double>(start.time_since_epoch()).count();
      roll = 15.0 * std::sin(t);
      pitch = 10.0 * std::sin(t * 0.7);
      yaw = std::fmod(t * 30.0, 360.0);
    }

    auto rpy = pack_rpy(roll, pitch, yaw, args.limits, sequence);
    auto accel = pack_accel(args.ax, args.ay, args.az, args.limits, args.report_id);

    can.send_std(kIdRpy, rpy);
    can.send_std(kIdAccel, accel);

    if (args.print) {
      print_tx(kIdRpy, rpy);
      print_tx(kIdAccel, accel);
    }

    ++sequence;
    if (args.once) {
      break;
    }
    std::this_thread::sleep_until(start + period);
  }
}

std::array<uint8_t, 8> frame_data_array(const can_frame &frame) {
  std::array<uint8_t, 8> data{};
  std::copy(frame.data, frame.data + std::min<int>(frame.can_dlc, 8), data.begin());
  return data;
}

void print_decoded(const can_frame &frame) {
  canid_t id = frame.can_id & CAN_SFF_MASK;
  auto data = frame_data_array(frame);

  if (id == kIdRpy && frame.can_dlc >= 8) {
    double roll = get_i16_be(data, 0) / 100.0;
    double pitch = get_i16_be(data, 2) / 100.0;
    double yaw = get_i16_be(data, 4) / 100.0;
    std::cout << "0x210 roll=" << roll << " pitch=" << pitch << " yaw=" << yaw
              << " limits=0x" << std::hex << static_cast<int>(data[6])
              << std::dec << " seq=" << static_cast<int>(data[7]) << "\n";
  } else if (id == kIdAccel && frame.can_dlc >= 8) {
    double ax = get_i16_be(data, 0) / 1000.0;
    double ay = get_i16_be(data, 2) / 1000.0;
    double az = get_i16_be(data, 4) / 1000.0;
    std::cout << "0x211 ax=" << ax << " ay=" << ay << " az=" << az
              << " limits=0x" << std::hex << static_cast<int>(data[6])
              << " report_id=0x" << static_cast<int>(data[7]) << std::dec
              << "\n";
  }
}

void run_monitor(const Args &args) {
  SocketCan can(args.interface);
  std::cout << "Monitoring " << args.interface << "\n";

  while (true) {
    can_frame frame = can.read_frame();
    print_decoded(frame);
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    Args args = parse_args(argc, argv);
    if (args.command == "send") {
      run_send(args);
    } else {
      run_monitor(args);
    }
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    print_usage();
    return 1;
  }

  return 0;
}

#else
#error "bottom-can-cpp uses Linux SocketCAN and must be built on Linux."
#endif

