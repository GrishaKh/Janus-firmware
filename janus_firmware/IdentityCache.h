// IdentityCache.h — local mirror of `/config.enrolled.rfid_uids`.
//
// On boot we load whatever was last written to NVS so we can authorize
// known cards even before WiFi/server come up. Each /config poll then
// atomically replaces the in-RAM set; the NVS copy only gets rewritten
// on actual diffs (avoid wear).
//
// Lookup is O(N) linear scan. With 200 cards that's microseconds — not
// worth a hashset until enrollment grows past ~1000.
#pragma once

#include <Arduino.h>
#include <vector>

class IdentityCache {
 public:
  void begin();

  // Replaces the cached set atomically. Persists to NVS only if the new
  // set differs from the old one.
  void update(const std::vector<String>& uids);

  bool   contains(const String& uid) const;
  size_t size() const { return _uids.size(); }

 private:
  std::vector<String> _uids;

  void loadFromNvs();
  void saveToNvs() const;
};
