--
-- (C) 2013-19 - ntop.org
--

local dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path
require "lua_utils"
require "network_utils"
local json = require("dkjson")

sendHTTPContentTypeHeader('text/html')

-- sendHTTPHeader('application/json')

local network = tonumber(_GET["network"])

local n = interface.getNetworkStats(network)
local res = {}

for k, v in pairs(n) do
   res = network2record(interface.getStats()["id"], v)
   break
end

print(json.encode(res, nil))
