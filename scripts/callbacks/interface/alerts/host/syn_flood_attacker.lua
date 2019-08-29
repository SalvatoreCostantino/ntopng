--
-- (C) 2019 - ntop.org
--

local alerts_api = require("alerts_api")
local alert_consts = require("alert_consts")

local check_module = {
  key = "syn_flood_attacker",
  check_function = alerts_api.threshold_check_function,
  threshold_type_builder = alerts_api.synFloodType,
  default_value = "syn_flood_attacker;gt;50",

  granularity = {
     -- executed only in the minute-by-minute check
     "min"
  },

  gui = {
    i18n_title = "entity_thresholds.syn_attacker_title",
    i18n_description = "entity_thresholds.syn_attacker_description",
    i18n_field_unit = alert_consts.field_units.syn_sec,
    input_builder = alerts_api.threshold_cross_input_builder,
    field_max = 65535,
    field_min = 1,
    field_operator = "gt";
  }
}

-- #################################################################

function check_module.get_threshold_value(granularity, info)
  return(info["hits.syn_flood_attacker"] or 0)
end

-- #################################################################

return check_module
