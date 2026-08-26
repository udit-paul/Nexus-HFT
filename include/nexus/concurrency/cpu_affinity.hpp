#pragma once

#include <thread>
#include <system_error>

namespace nexus {

/**
 * Pins the calling thread to the specified logical CPU core.
 * @param core_id The logical core index (0-based) to bind to.
 * @return true if successful, false otherwise.
 */
bool pin_thread_to_core(int core_id);

/**
 * Pins a given std::thread to the specified logical CPU core.
 * @param thread The thread to bind.
 * @param core_id The logical core index (0-based) to bind to.
 * @return true if successful, false otherwise.
 */
bool pin_thread_to_core(std::thread& thread, int core_id);

} // namespace nexus
