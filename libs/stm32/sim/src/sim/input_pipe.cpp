/**
 * @file input_pipe.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Named pipe input interface implementation.
 * @version 0.1
 * @date 2026-03-26
 * @copyright Copyright (c) 2026
 */

#include "input_pipe.hpp"

#include <cstring>

namespace Embys::Stm32::Sim
{

InputPipe::InputPipe(const std::string &pipe_path)
  : m_pipe_path(pipe_path), m_initialized(false), m_fd(-1)
{
  init();
}

InputPipe::~InputPipe()
{
  deinit();
}

bool
InputPipe::init()
{
  // Remove old pipe if exists
  unlink(m_pipe_path.c_str());

  // Create named pipe
  if (mkfifo(m_pipe_path.c_str(), 0666) == -1)
  {
    if (errno != EEXIST)
    {
      std::cerr << "Failed to create named pipe: " << m_pipe_path << " - "
                << strerror(errno) << std::endl;
      return false;
    }
  }

  // Open pipe persistently in non-blocking mode
  m_fd = open(m_pipe_path.c_str(), O_RDONLY | O_NONBLOCK);
  if (m_fd == -1)
  {
    std::cerr << "Failed to open named pipe: " << m_pipe_path << " - "
              << strerror(errno) << std::endl;
    unlink(m_pipe_path.c_str());
    return false;
  }

  m_initialized = true;
  std::cout << "Input pipe created: " << m_pipe_path << std::endl;
  std::cout << "Send commands: echo \"command args...\" > " << m_pipe_path
            << std::endl;
  return true;
}

void
InputPipe::deinit()
{
  if (m_initialized)
  {
    if (m_fd != -1)
    {
      close(m_fd);
      m_fd = -1;
    }
    unlink(m_pipe_path.c_str());
    m_initialized = false;
  }
}

void
InputPipe::register_command(const std::string &command, CommandHandler handler)
{
  m_commands[command] = {handler};
}

void
InputPipe::process()
{
  if (!m_initialized || m_fd == -1)
  {
    return;
  }

  // Read all available data from the persistent file descriptor
  char buffer[1024];
  ssize_t bytes_read;

  while ((bytes_read = read(m_fd, buffer, sizeof(buffer) - 1)) > 0)
  {
    buffer[bytes_read] = '\0';
    m_buffer += buffer;

    // Process complete lines
    size_t pos;
    while ((pos = m_buffer.find('\n')) != std::string::npos)
    {
      std::string line = m_buffer.substr(0, pos);
      m_buffer.erase(0, pos + 1);

      // Trim whitespace and process
      if (!line.empty())
      {
        execute_command(line);
      }
    }
  }

  // Note: With non-blocking read, EAGAIN/EWOULDBLOCK is expected when no data
  // available, so we don't treat it as an error
}

void
InputPipe::execute_command(const std::string &line)
{
  // Parse command and arguments
  std::istringstream iss(line);
  std::string cmd;
  iss >> cmd;

  if (cmd.empty())
  {
    return;
  }

  // Extract arguments
  std::vector<std::string> args;
  std::string arg;
  while (iss >> arg)
  {
    args.push_back(arg);
  }

  // Find and execute handler
  auto it = m_commands.find(cmd);
  if (it != m_commands.end())
  {
    const command_entry &entry = it->second;
    entry.handler(cmd, args);
  }
  else
  {
    std::cerr << "Unknown command: " << cmd << std::endl;
  }
}

}; // namespace Embys::Stm32::Sim
