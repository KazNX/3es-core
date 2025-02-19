#include "FrameDisplay.h"

#include <3escore/CollatedPacketDecoder.h>
#include <3escore/Endian.h>
#include <3escore/Log.h>
#include <3escore/Messages.h>
#include <3escore/PacketBuffer.h>
#include <3escore/PacketWriter.h>
#include <3escore/StreamUtil.h>
#include <3escore/TcpSocket.h>

#include <array>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <cxxopts.hpp>

// Note: this program is provided as a C++ implementation of command line packet recording. It is
// equivalent to the 3esrec program which is part of the C#/dotnet 3es project. It is recommended
// that the C# version be use as there are not significant performance differences when running with
// the '-m-' option (passthrough) and the C# code is more fully featured.

#define PACKET_TIMING 0

namespace tes
{
enum class Mode : int
{
  CollateAndCompress,
  CollateOnly,
  FileCompression,
  Uncompressed,
  Passthrough,

  Default = Passthrough
};

struct EndPoint
{
  std::string host;
  uint16_t port = 0;
};

class TesRec
{
#if PACKET_TIMING
  static const unsigned PacketLimit = 500u;
#endif  // PACKET_TIMING
public:
  struct Options
  {
    EndPoint end_point = { "127.0.0.1", defaultPort() };
    std::string prefix = "tes";
    bool persist = false;
    bool quiet = false;
    bool overwrite = false;
    bool help_mode = false;
  };

  [[nodiscard]] bool quit() const { return _quit; }
  [[nodiscard]] bool argsOk() const { return _args_ok; }
  [[nodiscard]] bool helpMode() const { return _opt.help_mode; }
  [[nodiscard]] bool connected() const { return _connected; }
  [[nodiscard]] bool persist() const { return _opt.persist; }
  [[nodiscard]] bool overwrite() const { return _opt.overwrite; }
  [[nodiscard]] bool quiet() const { return _opt.quiet; }

  [[nodiscard]] Mode decodeMode() const { return _decode_mode; }

  [[nodiscard]] unsigned totalFrames() const { return _total_frames; }
  // IPEndPoint ServerEndPoint { get; private set; }
  [[nodiscard]] const std::string &outputPrefix() const { return _opt.prefix; }
  [[nodiscard]] constexpr static const char *defaultPrefix() { return "tes"; }
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  [[nodiscard]] constexpr static uint16_t defaultPort() { return 33500u; }
  [[nodiscard]] constexpr static const char *defaultIP() { return "127.0.0.1"; }

  [[nodiscard]] static const char *modeToArg(Mode m);

  [[nodiscard]] static Mode argToMode(const char *arg);

  TesRec(int argc, const char **args);

  void run(FrameDisplay *frame_display);

  void requestQuit() { _quit = true; }

private:
  std::unique_ptr<TcpSocket> attemptConnection();

  std::unique_ptr<std::iostream> createOutputWriter();

  std::string generateNewOutputFile();

  static bool parseArgs(Options &opt, int argc, const char *const *argv);

  ServerInfoMessage _server_info;
  int _next_output_number = 0;
  unsigned _total_frames = 0;
  Mode _decode_mode = Mode::Default;

  Options _opt;

  bool _quit = false;
  bool _args_ok = true;
  bool _connected = false;

  using DefaultArgsArray = std::array<const char *, 4>;
  using DefaultModesAray = std::array<const char *, 5>;
  static const DefaultArgsArray DefaultArgs;
  static const DefaultModesAray ModeArgStrings;
};

const TesRec::DefaultArgsArray TesRec::DefaultArgs = { "--ip", "127.0.0.1", "--port", "33500" };
const TesRec::DefaultModesAray TesRec::ModeArgStrings = { "mc", "mC", "mz", "mu", "m-" };


const char *TesRec::modeToArg(Mode m)
{
  const auto mi = static_cast<size_t>(m);
  if (mi && mi <= ModeArgStrings.size())
  {
    return ModeArgStrings[mi];
  }

  return "";
}

Mode TesRec::argToMode(const char *arg)
{
  std::string arg_str(arg);
  while (!arg_str.empty() && arg_str[0] == '-')
  {
    arg_str.erase(0, 1);
  }

  for (size_t i = 0; i < ModeArgStrings.size(); ++i)
  {
    if (arg_str == ModeArgStrings[i])
    {
      return static_cast<Mode>(i);
    }
  }

  return Mode::Default;
}

TesRec::TesRec(int argc, const char **args)
{
  initDefaultServerInfo(&_server_info);
  if (argc)
  {
    _args_ok = parseArgs(_opt, argc, args);
  }
  else
  {
    _args_ok = parseArgs(_opt, static_cast<int>(DefaultArgs.size()), DefaultArgs.data());
  }
}


void TesRec::run(FrameDisplay *frame_display)
{
  const int connection_poll_time_sec_ms = 250;
  const auto socket_buffer_size = 4u * 1024u * 1024u;
  const auto decode_buffer_size = 4u * 1024u;
  const auto sleep_interval = std::chrono::microseconds(500);
  std::vector<uint8_t> socket_buffer(socket_buffer_size);
  std::vector<uint8_t> decode_buffer(decode_buffer_size);
  std::unique_ptr<TcpSocket> socket = nullptr;
  std::unique_ptr<PacketBuffer> packet_buffer;
  std::unique_ptr<std::iostream> io_stream;
  CollatedPacketDecoder collated_decoder;
#if PACKET_TIMING
  using TimingClock = std::chrono::high_resolution_clock;
  auto start_time = TimingClock::now();                   // Set start_time type
  auto timing_elapsed = TimingClock::now() - start_time;  // Set timing_elapsed type
#endif                                                    // PACKET_TIMING
  bool once = true;

  if (!quiet())
  {
    std::cout << "Connecting to " << _opt.end_point.host << ":" << _opt.end_point.port << std::endl;
  }

  while (!_quit && (persist() || once))
  {
    once = false;
    // First try establish a connection.
    while (!_quit && !_connected)
    {
      socket = attemptConnection();
      if (socket)
      {
#if PACKET_TIMING
        start_time = TimingClock::now();
#endif  // PACKET_TIMING
        _total_frames = 0u;
        frame_display->reset();
        if (!quiet())
        {
          frame_display->start();
        }

        io_stream = createOutputWriter();
        if (io_stream)
        {
          _connected = true;
          // Create a new packet buffer for this connection.
          packet_buffer = std::make_unique<PacketBuffer>();
        }
        // Log.Flush();
      }
      else
      {
        // Log.Flush();
        // Wait the timeout period before attempting to reconnect.
        std::this_thread::sleep_for(std::chrono::milliseconds(connection_poll_time_sec_ms));
      }
    }

    // Read while connected or data still available.
    bool have_data = false;
    while (!_quit && socket && (socket->isConnected() || have_data))
    {
      // We have a connection. Read messages while we can.
      const int bytes_read =
        socket->readAvailable(socket_buffer.data(), static_cast<int>(socket_buffer.size()));
      have_data = false;
      if (bytes_read <= 0)
      {
        std::this_thread::sleep_for(sleep_interval);
        continue;
      }

      have_data = true;
      packet_buffer->addBytes(socket_buffer.data(), static_cast<size_t>(bytes_read));

      while (PacketHeader *new_packet_header = packet_buffer->extractPacket(decode_buffer))
      {
        PacketReader completed_packet(new_packet_header);

        if (!completed_packet.checkCrc())
        {
          std::cout << "CRC failure" << std::endl;
          continue;
        }

        // TODO(KS): Check for dropped byte in the packet buffer.

        if (_decode_mode == Mode::Passthrough)
        {
          io_stream->write(reinterpret_cast<const char *>(new_packet_header),
                           completed_packet.packetSize());

          if (completed_packet.routingId() == MtControl)
          {
            if (completed_packet.messageId() == CIdFrame)
            {
              ++_total_frames;
              frame_display->incrementFrame();
#if PACKET_TIMING
              if (_total_frames >= PacketLimit)
              {
                timing_elapsed = TimingClock::now() - start_time;
                _quit = true;
              }
#endif  // PACKET_TIMING
            }
          }
        }
        else
        {
          // Decode and decompress collated packets. This will just return the same packet
          // if not collated.
          collated_decoder.setPacket(new_packet_header);
          while (const PacketHeader *decoded_packet_header = collated_decoder.next())
          {
            PacketReader decoded_packet(decoded_packet_header);

            switch (completed_packet.routingId())
            {
            case MtControl:
              if (decoded_packet.messageId() == CIdFrame)
              {
                ++_total_frames;
                frame_display->incrementFrame();
#if PACKET_TIMING
                if (_total_frames >= PacketLimit)
                {
                  timing_elapsed = TimingClock::now() - start_time;
                  _quit = true;
                }
#endif  // PACKET_TIMING
              }
              break;

            case MtServerInfo:
              if (_server_info.read(decoded_packet))
              {
                std::cout << "\nFailed to decode ServerInfo message" << std::endl;
                _quit = true;
              }
              break;

            default:
              break;
            }

            // TODO(KS): use a local collated packet to re-compress data.
            io_stream->write(reinterpret_cast<const char *>(new_packet_header),
                             completed_packet.packetSize());
          }
        }
      }
    }

    frame_display->stop();

    if (io_stream)
    {
      streamutil::finaliseStream(*io_stream, _total_frames);
      io_stream->flush();
      io_stream.reset(nullptr);
    }

    if (!quiet())
    {
      std::cout << "\nConnection closed" << std::endl;
    }

    // Disconnected.
    if (socket)
    {
      socket->close();
      socket.reset(nullptr);
    }

    _connected = false;
  }

#if PACKET_TIMING
  const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(timing_elapsed);
  std::cout << "Processed " << PacketLimit << " packets in " << static_cast<int>(elapsedMs.count())
            << "ms" << std::endl;
#endif  // PACKET_TIMING
}

std::unique_ptr<TcpSocket> TesRec::attemptConnection()
{
  std::unique_ptr<TcpSocket> socket(new TcpSocket);

  if (socket->open(_opt.end_point.host.c_str(), _opt.end_point.port))
  {
    socket->setNoDelay(true);
    socket->setWriteTimeout(0);
    socket->setReadTimeout(0);
    socket->setReadBufferSize(1024 * 1024);
    return socket;
  }

  return nullptr;
}


std::unique_ptr<std::iostream> TesRec::createOutputWriter()
{
  const std::string file_path = generateNewOutputFile();
  if (file_path.empty())
  {
    std::cout << "Unable to generate a numbered file name using the prefix: " << outputPrefix()
              << "Try cleaning up the output directory" << std::endl;
    return nullptr;
  }
  std::cout << "Recording to: " << file_path << std::endl;

  std::unique_ptr<std::fstream> stream(new std::fstream);

  stream->open(file_path.c_str(),
               std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);

  if (!stream->is_open())
  {
    return nullptr;
  }

  // Write the recording header uncompressed to the file.
  // We'll rewind here later and update the frame count.
  // Write to a memory stream to prevent corruption of the file stream when we wrap it
  // in a GZipStream.
  streamutil::initialiseStream(*stream, &_server_info);

  return stream;

  // TODO(KS): implement compression modes
  // switch (DecodeMode)
  // {
  //   case Mode.CollateAndCompress:
  //     stream = new CollationStream(fileStream, true);
  //     break;
  //   case Mode.CollateOnly:
  //     stream = new CollationStream(fileStream, false);
  //     break;
  //   case Mode.FileCompression:
  //     stream = new GZipStream(fileStream, CompressionMode.Compress,
  //     CompressionLevel.BestCompression); break;
  //   case Mode.Uncompressed:
  //     stream = fileStream;
  //     break;
  // }

  // return new NetworkWriter(stream);
}


std::string TesRec::generateNewOutputFile()
{
  const int max_files = 1000;
  std::string output_path;
  _next_output_number = _next_output_number % max_files;
  for (int i = _next_output_number; i < max_files; ++i)
  {
    std::ostringstream output_path_stream;
    output_path_stream << outputPrefix() << std::setw(3) << std::setfill('0') << i << ".3es";
    output_path = output_path_stream.str();

    // Check if the file exists.
    bool path_ok = overwrite();
    if (!path_ok)
    {
      const std::ifstream in_test(output_path.c_str(), std::ios::binary);
      path_ok = !in_test.is_open();
    }

    if (path_ok)
    {
      _next_output_number = i + 1;
      return output_path;
    }
  }

  return {};
}


bool TesRec::parseArgs(Options &opt, int argc, const char *const *argv)
{
  cxxopts::Options parser(
    "3esrec", "This program attempts to connect to and record a Third Eye Scene server.");

  // clang-format off
  parser.add_options()
    ("h,help", "Show command line help")
    ("i,ip", "Specifies the server IP address to connect to.", cxxopts::value(opt.end_point.host)->default_value(opt.end_point.host))
    ("p,port", "Specifies the port to connect on.", cxxopts::value(opt.end_point.port)->default_value(std::to_string(defaultPort())))
    ("persist", "Persist running after the first connection closes, waiting for a new connection. Use Ctrl-C to terminate.", cxxopts::value(opt.persist)->implicit_value("true"))
    ("q,quiet", "Run in quiet mode (disable non-critical logging).", cxxopts::value(opt.quiet)->implicit_value("true"))
    ("overwrite", "Overwrite existing files using the current prefix. The current session numbering will not overwrite until it loops to 0.", cxxopts::value(opt.overwrite)->implicit_value("true"))
    ("prefix", "Specifies the file prefix used for recording. The recording file is "
         "formulated as {prefix###.3es}, where the number used is the first missing "
         "file up to 999. At that point the program will complain that there are no "
         "more available file names.", cxxopts::value(opt.prefix)->default_value(opt.prefix))
  ;
  // clang-format on

  parser.parse_positional({ "prefix" });

  try
  {
    cxxopts::ParseResult parsed = parser.parse(argc, argv);

    if (parsed.count("help"))
    {
      // Force output: don't log as that could be filtered by log level.
      std::cout << parser.help() << std::endl;
      // Help already shown by cxxopts. Flag exit.
      opt.help_mode = true;
    }
  }
  catch (const cxxopts::exceptions::parsing &e)
  {
    tes::log::error("Argument error\n", e.what());
    return false;
  }

  return true;
}
}  // namespace tes

namespace
{
tes::TesRec *g_prog = nullptr;

void onSignal(int signal)
{
  (void)signal;
  if (g_prog)
  {
    g_prog->requestQuit();
    g_prog = nullptr;
  }
}
}  // namespace

int main(int argc, const char **argv)
{
  tes::TesRec prog(argc, argv);
  g_prog = &prog;

  signal(SIGINT, onSignal);
  signal(SIGTERM, onSignal);

  if (prog.helpMode() || !prog.argsOk())
  {
    return 1;
  }

  tes::FrameDisplay frame_display;
  prog.run(&frame_display);
  frame_display.stop();

  g_prog = nullptr;

  return 0;
}
