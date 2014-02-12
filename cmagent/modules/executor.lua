-- Copyright (c) 2012 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local io, os = io, os
local conf = require "hiwifi.conf"
local json = require "hiwifi.json"
local ltn12 = require "ltn12"
local nixio = require "nixio"
local fs = require "nixio.fs"
require "nixio.util"
local https = require "ssl.https"

-- Parse message.
local input_string = io.read("*a")
local message = json.decode(input_string)
-- Compatible to old version.
if message["app_id"] == "core" then
  message["app_id"] = message["data"]["cmd"]
end
--TODO(sh): more restrictions on msg_id and app_id.
if message == nil or message["msg_id"] == nil or message["app_id"] == nil then
  print "malformed JSON"
  os.exit(111)
end

-- Set up working directory.
nixio.chdir(conf.mem_path)
local working_dir =
    conf.disk_path .. "/cloud/" .. message["msg_id"] .. "_" .. nixio.getpid()
fs.mkdirr(working_dir)
nixio.chdir(working_dir)

function myexec(app_id, input)
  local pipe_read, pipe_write = nixio.pipe()
  local pipe_read2, pipe_write2 = nixio.pipe()
  local pid, errno, err_info = nixio.fork()
  if errno ~= nil then
    return errno, err_info
  end
  if pid == 0 then
    pipe_read2:close()
    pipe_write:close()
    nixio.dup(pipe_read, io.stdin)
    nixio.dup(pipe_write2, io.stdout)
    nixio.dup(pipe_write2, io.stderr)
    pipe_read:close()
    pipe_write2:close()
    nixio.exec(conf.lua_bin_file, "/usr/lib/cmagent/" .. app_id .. ".lua")
    os.exit(112)
  end
  pipe_write2:close()
  pipe_read:close()
  pipe_write:write(input)
  pipe_write:close()
  data = pipe_read2:readall()
  pipe_read2:close()
  local child_pid, state, exit_code = nixio.wait()
  return exit_code, (data ~= nil and data or "CMAgent: got nil.")
end

local code, output = myexec(message["app_id"], json.encode(message["data"]))

-- Upload result.
local params = {
  url = "https://cloud.hiwifi.com/message/report",
  method = "POST",
  source = ltn12.source.string(output),
  verify = "peer",
  capath = "/etc/ca",
  headers = {
    ["Content-Type"] = "text/plain",
    ["Content-Length"] = #output,
    ["X-Message-Id"] = message["msg_id"],
    ["X-Return-Code"] = code
  }
}
local result, code = https.request(params);
-- Time will change suddenly after booting, it causes a timeout in https library,
-- so try it once more.
if result ~= 1 then
  https.request(params)
end

-- Clean up.
nixio.chdir("..")
fs.remover(working_dir)
