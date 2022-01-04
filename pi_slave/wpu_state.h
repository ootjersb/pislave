namespace state {

enum class WpState : uint16_t {
  Init = 0,
  Off = 1,
  ChMode = 2,
  DhwMode = 3,
  FcMode = 4,
  VentMode = 5,
};

enum class WpSubState : uint16_t {
  PreRun = 0,
  PowerOnDelay = 1,
  PreRunSource = 2,
  ExpValveBalancing = 3,
  ExpValveClosing = 4,
  ExpValveStart = 5,
  OpenCoValve = 6,
  Run = 7,
  PrepareControlledStop = 8,
  ElectricElementOnly = 9,
  RunOn = 10,
  None = 255,
};

inline std::ostream& operator<<(std::ostream& os, const WpState& state) {
  switch (state) {
  case WpState::Init:
    os << "Init";
    break;
  case WpState::Off:
    os << "Off";
    break;
  case WpState::ChMode:
    os << "CH mode";
    break;
  case WpState::DhwMode:
    os << "DHW mode";
    break;
  case WpState::FcMode:
    os << "Free cooling mode";
    break;
  case WpState::VentMode:
    os << "Venting mode";
    break;
  default:
    os << "UNKNOWN";
    break;
  }

  return os;
}

inline std::ostream& operator<<(std::ostream& os, const WpSubState& sub_state) {
  switch (sub_state) {
  case WpSubState::PreRun:
    os << "Pre run";
    break;
  case WpSubState::PowerOnDelay:
    os << "Power on delay";
    break;
  case WpSubState::PreRunSource:
    os << "Pre run source";
    break;
  case WpSubState::ExpValveBalancing:
    os << "Expansion valve balancing";
    break;
  case WpSubState::ExpValveClosing:
    os << "Expansion valve closing";
    break;
  case WpSubState::ExpValveStart:
    os << "Expansion valve start";
    break;
  case WpSubState::OpenCoValve:
    os << "Open CO-valve";
    break;
  case WpSubState::Run:
    os << "Run";
    break;
  case WpSubState::PrepareControlledStop:
    os << "Prepare controlled stop";
    break;
  case WpSubState::ElectricElementOnly:
    os << "Electric element only";
    break;
  case WpSubState::RunOn:
    os << "Run on";
    break;
  case WpSubState::None:
    os << "None";
    break;
  default:
    os << "UNKNOWN";
    break;
  }

  return os;
}

}  // namespace state
