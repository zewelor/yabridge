// yabridge: a Wine VST bridge
// Copyright (C) 2020  Robbert van der Helm
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <boost/filesystem.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/environment.hpp>

/**
 * Boost 1.72 was released with a known breaking bug caused by a missing
 * typedef: https://github.com/boostorg/process/issues/116.
 *
 * Luckily this is easy to fix since it's not really possible to downgrade Boost
 * as it would break other applications.
 *
 * Check if this is still needed for other distros after Arch starts packaging
 * Boost 1.73.
 */
class patched_async_pipe : public boost::process::async_pipe {
   public:
    using boost::process::async_pipe::async_pipe;

    typedef typename handle_type::executor_type executor_type;
};

/**
 * A tag to differentiate between 32 and 64-bit plugins, used to determine which
 * host application to use.
 */
enum class PluginArchitecture { vst_32, vst_64 };

/**
 * Create a logger prefix based on the unique socket path for easy
 * identification. The socket path contains both the plugin's name and a unique
 * identifier.
 *
 * @param socket_path The path to the socket endpoint in use.
 *
 * @return A prefix string for log messages.
 */
std::string create_logger_prefix(const boost::filesystem::path& socket_path);

/**
 * Determine the architecture of a VST plugin (or rather, a .dll file) based on
 * it's header values.
 *
 * See https://docs.microsoft.com/en-us/windows/win32/debug/pe-format for more
 * information on the PE32 format.
 *
 * @param plugin_path The path to the .dll file we're going to check.
 *
 * @return The detected architecture.
 * @throw std::runtime_error If the file is not a .dll file.
 */
PluginArchitecture find_vst_architecture(boost::filesystem::path);

/**
 * Finds the Wine VST host (either `yabridge-host.exe` or `yabridge-host.exe`
 * depending on the plugin). For this we will search in two places:
 *
 *   1. Alongside libyabridge.so if the file got symlinked. This is useful
 *      when developing, as you can simply symlink the the libyabridge.so
 *      file in the build directory without having to install anything to
 *      /usr.
 *   2. In the regular search path.
 *
 * @param plugin_arch The architecture of the plugin, either 64-bit or 32-bit.
 *   Used to determine which host application to use, if available.
 *
 * @return The a path to the VST host, if found.
 * @throw std::runtime_error If the Wine VST host could not be found.
 */
boost::filesystem::path find_vst_host(PluginArchitecture plugin_arch);

/**
 * Find the VST plugin .dll file that corresponds to this copy of
 * `libyabridge.so`. This should be the same as the name of this file but with a
 * `.dll` file extension instead of `.so`. In case this file does not exist and
 * the `.so` file is a symlink, we'll also repeat this check for the file it
 * links to. This is to support the workflow described in issue #3 where you use
 * symlinks to copies of `libyabridge.so`.
 *
 * @return The a path to the accompanying VST plugin .dll file.
 * @throw std::runtime_error If no matching .dll file could be found.
 */
boost::filesystem::path find_vst_plugin();

/**
 * Locate the Wine prefix this file is located in, if it is inside of a wine
 * prefix. This is done by locating the first parent directory that contains a
 * directory named `dosdevices`.
 *
 * @return Either the path to the Wine prefix (containing the `drive_c?`
 *   directory), or `std::nullopt` if it is not inside of a wine prefix.
 */
std::optional<boost::filesystem::path> find_wineprefix();

/**
 * Generate a unique name for the Unix domain socket endpoint based on the VST
 * plugin's name. This will also generate the parent directory if it does not
 * yet exist since we're using this in the constructor's initializer list.
 *
 * @return A path to a not yet existing Unix domain socket endpoint.
 * @throw std::runtime_error If no matching .dll file could be found.
 */
boost::filesystem::path generate_endpoint_name();

/**
 * Return a path to this `.so` file. This can be used to find out from where
 * this link to or copy of `libyabridge.so` was loaded.
 */
boost::filesystem::path get_this_file_location();

/**
 * Return the installed Wine version. This is obtained by from `wine --version`
 * and then stripping the `wine-` prefix. This respects the `WINELOADER`
 * environment variable used in the scripts generated by winegcc.
 *
 * This will *not* throw when Wine can not be found, but will instead return
 * '<NOT FOUND>'. This way the user will still get some useful log files.
 */
std::string get_wine_version();

/**
 * Locate the Wine prefix and set the `WINEPREFIX` environment variable if
 * found. This way it's also possible to run .dll files outside of a Wine prefix
 * using the user's default prefix.
 */
boost::process::environment set_wineprefix();
