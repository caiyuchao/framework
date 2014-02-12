-- Copyright (c) 2013 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local os = os

local cmagent = require "hiwifi.cmagent"
local firmware = require "hiwifi.firmware"

--- Downloads the latest firmware to cache and notify the user.
--@param latest_firmware the firmware to be downloaded
--@return 0 if success
local function cache_and_notify(latest_firmware)
  local error_code = firmware.download_to_cache(latest_firmware)
  if error_code ~= 0 then
    return error_code
  end
  return firmware.enable_notification()
end

-- Parse message.
local data = cmagent.parse_data()

local latest_firmware, code, err = firmware.get_update_info(data.from)
if latest_firmware == nil then
  print("No updates found.")
  print(code)
  print(err)
  os.exit(102)
end

local return_code
if data.cmd == "upgrade" then
  return_code = firmware.upgrade_to_latest(latest_firmware)
elseif data.cmd == "notify" then
  return_code = cache_and_notify(latest_firmware)
elseif data.cmd == "download" then
  return_code = firmware.download_to_cache(latest_firmware)
else
  return_code = 101
end

os.exit(return_code)
