-- Copyright (c) 2013 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local os, print, type = os, print, type
local cmagent = require "hiwifi.cmagent"
local http = require "socket.http"

local ERROR_UNKOWN = 99
local ERROR_NOPAGE = 104
local ERROR_EXECUTE = 202

local function to_return_value(code)
  if type(code) ~= "number" then
    return ERROR_UNKOWN
  end
  if code >= 200 and code < 300 then
    return 0
  end
  if code == 404 then
    return ERROR_NOPAGE
  end
  if code == 502 then
    return ERROR_EXECUTE
  end
  return ERROR_UNKOWN
end

local data = cmagent.parse_data()

local url = "http://127.0.0.1/cgi-bin/turbo/" .. data.path
local body, code = http.request(url, data.body)

print(body)
os.exit(to_return_value(code))
