-- Copyright (c) 2012 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local cmagent = require "hiwifi.cmagent"
local market = require "hiwifi.market"
local util = require "hiwifi.util"
local os = require "os"

-- Parse message.
local data = cmagent.parse_data()

local return_code
if data.cmd == "install" then
  return_code = market.install(data)
elseif data.cmd == "uninstall" then
  return_code = market.uninstall(data.app)
elseif data.cmd == "list" then
  return_code = market.report() and 0 or 1
else
  return_code = market.call_function(data.app, data.cmd)
end

os.exit(return_code)
