#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Wraps the ticos-firmware-sdk up as a Particle library:
//! https://docs.particle.io/tutorials/device-os/libraries/

#include "Particle.h"

// Convenience include of all ticos-firmware-sdk headers. This allows
// an end user to simply this file, "ticos.h", to get access to all Ticos APIs
#include "ticos-firmware-sdk/components/include/ticos/components.h"

class Ticos {
 public:

  //! Constructor for Ticos library
  //!
  //! @param product_version The particle version of the application running on the device
  //!   See https://ticos.io/particle-versioning for more details
  //! @param build_metadata Additional build metadata (such as a git short SHA or timestamp)
  //!   to append to the "software_version" reported to Ticos.
  //! @param hardware_version When set, overrides the default strategy for reporting the
  //!  hardware_version using the PLATFORM_NAME compiled against
  explicit Ticos(const uint16_t product_version = 0, const char *build_metadata = NULL,
                    const char *hardware_version = NULL);

  //! Should be called from your applications loop handler
  //!
  //!
  //! static Ticos s_ticos(your_product_version);
  //! void loop() {
  //!   [...]
  //!   ticos.process();
  //! }
  void process(void);

  //! Underlying functionality is disabled by default, but can be enabled
  //! by adding the following to your ticos_particle_user_config.h
  //!
  //! #define TICOS_PARTICLE_PORT_DEBUG_API_ENABLE 1
  //!
  //! @return >0 on success or error code on failure
  int run_debug_cli_command(const char *command, int argc, char **argv);

 private:
  //! Hooks into system "cloud_status_ event for tracking m_connected and recording connectivity
  //! heartbeat metrics
  void handle_cloud_connectivity_event(system_event_t event, int param);

  //! true if a particle cloud connection is available, false otherwise
  bool m_connected;
};
