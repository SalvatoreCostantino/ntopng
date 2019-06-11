--
-- (C) 2019 - ntop.org
--

local dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"
local format_utils = require("format_utils")
local json = require("dkjson")
local ts_utils = require("ts_utils")
local rtt_utils = require("rtt_utils")

sendHTTPContentTypeHeader('application/json')

-- ################################################

local currentPage  = _GET["currentPage"]
local perPage      = _GET["perPage"]
local sortColumn   = _GET["sortColumn"]
local sortOrder    = _GET["sortOrder"]
local cont_filter_s = _GET["custom_hosts"]

local sortPrefs = "rtt_hosts"

-- ################################################

if isEmptyString(sortColumn) or sortColumn == "column_" then
   sortColumn = getDefaultTableSort(sortPrefs)
else
   if((sortColumn ~= "column_")
    and (sortColumn ~= "")) then
      tablePreferences("sort_"..sortPrefs, sortColumn)
   end
end

if isEmptyString(_GET["sortColumn"]) then
  sortOrder = getDefaultTableSortOrder(sortPrefs, true)
end

if((_GET["sortColumn"] ~= "column_")
  and (_GET["sortColumn"] ~= "")) then
    tablePreferences("sort_order_"..sortPrefs, sortOrder, true)
end

if(currentPage == nil) then
   currentPage = 1
else
   currentPage = tonumber(currentPage)
end

if(perPage == nil) then
   perPage = getDefaultTableSize()
else
   perPage = tonumber(perPage)
   tablePreferences("rows_number", perPage)
end

local sOrder = ternary(sortOrder == "desc", rev_insensitive, asc_insensitive)
local to_skip = (currentPage-1) * perPage

-- ################################################

local rtt_hosts = rtt_utils.getHosts()
local totalRows = 0
local sort_to_key = {}

for key, config in pairs(rtt_hosts) do
  sort_to_key[key] = config.host

  totalRows = totalRows + 1
end

 --################################################

local res = {}
local i = 0

for key in pairsByValues(sort_to_key, sOrder) do
  if i >= to_skip + perPage then
    break
  end

  if (i >= to_skip) then
    local rtt_host = rtt_hosts[key]
    local chart = ""

    if ts_utils.exists("monitored_host:rtt", {host=key}) then
      chart = '<a href="'.. ntop.getHttpPrefix() ..'/lua/rtt_stats.lua?rtt_host='.. key ..'&page=historical"><i class="fa fa-area-chart fa-lg"></i></a>'
    end
 
    res[#res + 1] = {
      column_key = key,
      column_host = rtt_host.host,
      column_chart = chart,
      column_iptype = rtt_host.iptype,
      column_max_rrt = rtt_host.max_rtt,
    }
  end

  i = i + 1
end

-- ################################################

local result = {}
result["perPage"] = perPage
result["currentPage"] = currentPage
result["totalRows"] = totalRows
result["data"] = res
result["sort"] = {{sortColumn, sortOrder}}

print(json.encode(result))
