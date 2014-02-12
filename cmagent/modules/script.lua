-- Copyright (c) 2012 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local os = require "os"
local cmagent = require "hiwifi.cmagent"

local data = cmagent.parse_data()

-- remote script enable
local file,err=io.open("/etc/config/remote_script_enable");
if not file then 
	print("Remote script disable.")
	os.exit(1)
end

if data.script == nil then
  print "Script is missing."
  os.exit(1)
end

local ok = os.execute(data.script)
os.exit(ok / 256)
