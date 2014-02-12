-- Copyright (c) 2012 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local os = os

local cmagent = require "hiwifi.cmagent"
local firmware = require "hiwifi.firmware"

-- Parse message.
local data = cmagent.parse_data()

if data.url == nil or data.md5 == nil then
  os.exit(1)
end

local error_code = firmware.download_to_cache(data)
if error_code ~= 0 then
  print("Failed to download, code " .. error_code)
  os.exit(2)
end
local ok = firmware.enable_notification()
os.exit(ok)
