#pragma once

// Device-hosted WiFi AP + web page for editing wearer name, alert numbers,
// and the geofence -- this is "the website" from the spec, no cloud/backend
// needed since it's config the wearer's household edits occasionally.
namespace WebPortal {
  void begin(); // starts AP "SafetyBand-Setup" and the HTTP server
}
