/**
 * @file input_pipe.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Named pipe input interface for simulation commands.
 *
 * Provides a command input interface using named pipes (FIFO) that allows
 * sending commands to the simulation from terminal or scripts.
 *
 * Usage:
 *   InputPipe pipe("/tmp/embys_stm32_sim_pipe");
 *   pipe.register_command("blink", my_context, my_blink_handler);
 *
 *   // In main loop:
 *   pipe.process();
 *
 * From terminal:
 *   echo "blink 100 200 300" > /tmp/embys_stm32_sim_pipe
 *
 * @version 0.1
 * @date 2026-03-26
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace Embys::Stm32::Sim
{

/**
 * @brief Named pipe input interface for receiving commands.
 */
class InputPipe
{
public:
  /**
   * @brief Command handler function signature.
   * @param cmd Command name
   * @param args Command arguments
   */
  using CommandHandler = std::function<void(const std::string &,
                                            const std::vector<std::string> &)>;

  /**
   * @brief Construct and initialize the named pipe.
   * @param pipe_path Path to the named pipe (e.g., "/tmp/sim_pipe")
   */
  explicit InputPipe(const std::string &pipe_path);

  /**
   * @brief Destructor - cleans up the pipe.
   */
  ~InputPipe();

  // Disable copy
  InputPipe(const InputPipe &) = delete;
  InputPipe &
  operator=(const InputPipe &) = delete;

  /**
   * @brief Register a command handler.
   * @param command Command name to register
   * @param handler Function to call when command is received
   */
  void
  register_command(const std::string &command, CommandHandler handler);

  /**
   * @brief Check pipe and process all pending commands (non-blocking).
   *
   * This should be called regularly from your main loop.
   * Reads all available data from the pipe and executes registered handlers.
   */
  void
  process();

  /**
   * @brief Check if the pipe is initialized and ready.
   * @return true if pipe is operational
   */
  bool
  is_open() const
  {
    return m_initialized;
  }

  /**
   * @brief Get the pipe path.
   * @return Path to the named pipe
   */
  const std::string &
  get_path() const
  {
    return m_pipe_path;
  }

private:
  /**
   * @brief Initialize the named pipe.
   * @return true if successful
   */
  bool
  init();

  /**
   * @brief Clean up the named pipe.
   */
  void
  deinit();

  /**
   * @brief Parse and execute a single command line.
   * @param line Command line to parse
   */
  void
  execute_command(const std::string &line);

  struct command_entry
  {
    CommandHandler handler;
  };

  std::string m_pipe_path;
  bool m_initialized;
  int m_fd; // Persistent file descriptor for the pipe
  std::map<std::string, command_entry> m_commands;
  std::string m_buffer; // Buffer for incomplete lines
};

}; // namespace Embys::Stm32::Sim
