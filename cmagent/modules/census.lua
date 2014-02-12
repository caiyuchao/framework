-- Copyright (c) 2013 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local io, os, string, unpack = io, os, string, unpack
local auth = require "auth"
local util = require "hiwifi.util"
local ltn12 = require "ltn12"
local nixio = require "nixio"
local fs = require "nixio.fs"
require "nixio.util"
local https = require "ssl.https"
local tw = require "tw"

local URL = "https://cloud.hiwifi.com/api/new_census?id=%s&token=%s"
local CACHE_PATH = "/tmp/cron/"

function myexec(command)
  local pipe_read, pipe_write = nixio.pipe()
  local pid, errno, err_info = nixio.fork()
  if errno ~= nil then
    return errno, err_info
  end
  if pid == 0 then
    pipe_read:close()
    nixio.dup(pipe_write, io.stdout)
    pipe_write:close()
    nixio.exec(unpack(command))
    os.exit(112)
  end
  pipe_write:close()
  data = pipe_read:readall()
  pipe_read:close()
  local child_pid, state, exit_code = nixio.wait()
  return exit_code, (data ~= nil and data or "CMAgent: got nil.")
end

local code, output = myexec(arg)
if code ~= 0 then
  print "Failed to run given command."
  os.exit(1)
end

-- Compare with cached content.
local old_data = fs.readfile(CACHE_PATH .. string.gsub(arg[1], "/", "_"))
if old_data == data then
  print "Discard unchanged data."
  os.exit(0)
end

local token = auth.get_token("cron")

-- Upload result.
local response = util.download_to_string({
  url = string.format(URL, tw.get_mac(), token),
  method = "POST",
  source = ltn12.source.string(output),
  headers = {
    ["Content-Type"] = "text/plain",
    ["Content-Length"] = #output,
  }
})

-- Save to cache.
if response == "OK" then
  fs.mkdirr(CACHE_PATH)
  fs.writefile(CACHE_PATH .. string.gsub(arg[1], "/", "_"), data)
end
