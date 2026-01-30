#!/bin/bash
set +H

echo "Generating wifi_config_h"

cat > components/wifi/include/wifi_config.h << 'EOF'
// Auto-generated wifi configuration - DO NOT EDIT
// This file is regenerated on each build
#pragma once

// WiFi Station Mode (client) credentials
#define WIFI_STA_SSID "SMART"
#define WIFI_STA_PASS "!@#$MECH47R0nic4nE7W0rk2025%&*($)"

// WiFi Access Point Mode credentials
#define WIFI_AP_SSID "CablePeeler"
#define WIFI_AP_PASS "password"
EOF

echo "✓ Generated components/wifi/include/wifi_config.h"
echo "  STA SSID: SMART"
echo "  STA PASS: !@#$MECH47R0nic4nE7W0rk2025%&*($)"
echo "  AP SSID:  CablePeeler"
echo "  AP PASS:  password"