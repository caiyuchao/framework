-- Copyright (c) 2012 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local print = print
local net = require "hiwifi.net"
local safe_mac = require "hiwifi.safe_mac"
local json = require "hiwifi.json"
local traffic_stats = require "hiwifi.traffic_stats"
local device_names = require "hiwifi.device_names"

local wifi = net.get_wifi_device()
local clients = net.get_wifi_client_list()
local safe_macs = safe_mac.get_all()
local dhcp_list = net.get_dhcp_client_list()
local traffic = traffic_stats.read_stats()
local device_name_all = device_names.get_all()

local function add_ip_and_hostname(client)
  client['ip'] = ''
  client['name'] = ''
  for _, dhcp_client in pairs(dhcp_list) do
    if dhcp_client['mac'] == client['mac'] then
      client['ip'] = dhcp_client['ip']
      client['name'] = dhcp_client['name']
      return
    end
  end
end

local function fix_hostname(client)
  local mac = client['mac']
  if device_name_all[mac] ~= "" and device_name_all[mac] ~= nil then
    client['name'] = device_name_all[mac]
  end
  return
end

for _, client in pairs(clients) do
  client['is_safe'] = safe_macs[client['mac']] and 1 or 0
  add_ip_and_hostname(client)
  fix_hostname(client)
end

print(json.encode({
  wifi = wifi,
  clients = clients,
  traffic = traffic
}))
