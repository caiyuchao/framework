-- Copyright (c) 2012 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local io, os, string, table = io, os, string, table
local cmagent = require "hiwifi.cmagent"
local strings = require "hiwifi.strings"
local ltn12 = require "ltn12"
local http = require "socket.http"

-- Parse message.
local data = cmagent.parse_data()

if data.url == nil then
  print "URL is missing."
  os.exit(1)
end

local type = data.type or "tw"
local priority = data.pri or 70

--- Prefetches one URL.
--@param url the URL to be prefetched
--@param type the type of prefetching
--@param priority the priority of prefetching
--@return true if succeed, otherwise false following by a debug output string
local function prefetch_single(url, type, priority)
  -- Send HTTP request.
  local t = {}
  local param = {
    url = "http://127.0.0.1:81/",
    sink = ltn12.sink.table(t),
    headers = {
      ["Host"] = "tw-download",
      ["X-Tw-Durl"] = url,
      ["X-Tw-Dtype"] = type,
      ["X-Tw-Dpri"] = priority
    }
  }
  local ok, code, headers, status = http.request(param)

  -- Analyze result.
  if ok == 1 and code >= 200 and code <= 299 then
    return true
  end
  return false, string.format("%s %s %s", code, status, table.concat(t))
end

local error_output_list = {}
local urls = strings.split(data.url, ' ')
for _, url in pairs(urls) do
  local is_ok, output = prefetch_single(url, type, priority)
  if not is_ok then
    table.insert(error_output_list, string.format("%s: %s", url, output))
  end
end

if #error_output_list > 0 then
  print(table.concat(error_output_list, "\n"))
  os.exit(2)
end
os.exit(0)
