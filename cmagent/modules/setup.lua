-- Copyright (c) 2012 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local os = os
local cmagent = require "hiwifi.cmagent"
local mac_filter = require "hiwifi.mac_filter"
local net = require "hiwifi.net"

-- Parse message.
local data = cmagent.parse_data()

local return_code = 0
if data.cmd == "mac_filter_disable" then
  return_code = mac_filter.disable() and 0 or 1
elseif data.cmd == "mac_filter_deny" then
  return_code = mac_filter.deny_mac(data.mac) and 0 or 1
elseif data.cmd == "turn_wifi_on" then
  net.turn_wifi_on()
elseif data.cmd == "turn_wifi_off" then
  net.turn_wifi_off()
else
  return_code = 101
end

os.exit(return_code)
