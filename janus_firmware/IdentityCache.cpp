#include "IdentityCache.h"

#include <Preferences.h>

namespace {
constexpr const char* kNvsNamespace = "janus";
constexpr const char* kNvsKey       = "rfid_uids";
}  // namespace

void IdentityCache::begin() {
  loadFromNvs();
  Serial.print(F("[id] loaded "));
  Serial.print(_uids.size());
  Serial.println(F(" UIDs from NVS"));
}

void IdentityCache::update(const std::vector<String>& uids) {
  // Detect a no-op early: same length AND same elements (order-insensitive
  // would need sort, but server returns them in a stable order so equality
  // by index is good enough here).
  bool sameAsCurrent = (uids.size() == _uids.size());
  if (sameAsCurrent) {
    for (size_t i = 0; i < uids.size(); ++i) {
      if (uids[i] != _uids[i]) { sameAsCurrent = false; break; }
    }
  }
  if (sameAsCurrent) return;

  _uids = uids;     // atomic swap from the caller's perspective
  saveToNvs();
  Serial.print(F("[id] cache updated, "));
  Serial.print(_uids.size());
  Serial.println(F(" UIDs"));
}

bool IdentityCache::contains(const String& uid) const {
  for (const String& known : _uids) {
    if (known == uid) return true;
  }
  return false;
}

void IdentityCache::loadFromNvs() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, /*readOnly=*/true)) {
    return;  // namespace doesn't exist yet — first boot
  }
  String packed = prefs.getString(kNvsKey, "");
  prefs.end();

  _uids.clear();
  if (packed.length() == 0) return;

  int start = 0;
  for (int i = 0; i <= (int)packed.length(); ++i) {
    if (i == (int)packed.length() || packed[i] == ',') {
      if (i > start) {
        _uids.push_back(packed.substring(start, i));
      }
      start = i + 1;
    }
  }
}

void IdentityCache::saveToNvs() const {
  String packed;
  // Pre-size: 8 chars per 4-byte UID plus separators is a safe lower bound.
  packed.reserve(_uids.size() * 16);
  for (size_t i = 0; i < _uids.size(); ++i) {
    if (i > 0) packed += ',';
    packed += _uids[i];
  }

  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, /*readOnly=*/false)) {
    Serial.println(F("[id] WARNING: failed to open NVS for write"));
    return;
  }
  prefs.putString(kNvsKey, packed);
  prefs.end();
}
