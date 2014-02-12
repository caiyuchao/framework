-- Copyright (c) 2013 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local nixio = require "nixio"
local fs = require "nixio.fs"

local DIR_JOBS = "/etc/cron"
local DIR_TIMES = "/tmp/data/cron"

local function is_dir(filename)
  return fs.stat(filename, "type") == "dir"
end

local function get_last_run_time(job)
  local filename = DIR_TIMES .. "/" .. string.gsub(job, "/", "_")
  local content = fs.readfile(filename)
  if content == nil then
    return 0
  end
  local last_run_time = tonumber(content)
  return last_run_time == nil and 0 or last_run_time
end

local function set_last_run_time(job, time)
  fs.mkdirr(DIR_TIMES)
  local filename = DIR_TIMES .. "/" .. string.gsub(job, "/", "_")
  fs.remover(filename)
  fs.writefile(filename, time)
end

local function start_job(file, interval_minute)
  local now = os.time()
  if get_last_run_time(file) + interval_minute * 60 <= now then
    local pid = nixio.fork()
    if pid == 0 then
      set_last_run_time(file, now)
      nixio.exec(file)
    end
  end
end

local function start_subdir_jobs(dirname, interval_minute)
  local iter = fs.dir(dirname)
  if iter then
    for file in iter do
      start_job(dirname .. "/" .. file, interval_minute)
    end
  end
end

local function start_all_jobs()
  local iter = fs.dir(DIR_JOBS)
  if iter then
    for file in iter do
      local dirname = DIR_JOBS .. "/" .. file
      local interval_minute = tonumber(file)
      if is_dir(dirname) and interval_minute ~= nil then
        start_subdir_jobs(dirname, interval_minute)
      end
    end
  end
end

start_all_jobs()
