--
-- (C) 2019 - ntop.org
--

local alerts_api = require("alerts_api")
local alert_consts = require("alert_consts")

local check_module

-- #################################################################

local function check_tcp_flags(params)
  local info = params.entity_info
  local key = params.check_module.key
  local sent_stats = info["pktStats.sent"]
  local rcvd_stats = info["pktStats.recv"]
  local rst_ratio_threshold = 50 -- 50%

  local syn_flags_sent = alerts_api.host_delta_val(key .. "_syn_sent", params.granularity, sent_stats.syn, true --[[ skip first]])
  local syn_flags_rcvd = alerts_api.host_delta_val(key .. "_syn_rcvd", params.granularity, rcvd_stats.syn, true --[[ skip first]])
  local rst_flags_sent = alerts_api.host_delta_val(key .. "_rst_sent", params.granularity, sent_stats.rst, true --[[ skip first]])
  local rst_flags_rcvd = alerts_api.host_delta_val(key .. "_rst_rcvd", params.granularity, rcvd_stats.rst, true --[[ skip first]])

  local rst_sent_ratio = math.min((rst_flags_sent * 100) / (syn_flags_rcvd+1), 100)
  local rst_rcvd_ratio = math.min((rst_flags_rcvd * 100) / (syn_flags_sent+1), 100)

  local rst_sent_info = alerts_api.anomalousTCPFlagsType(syn_flags_rcvd, rst_flags_sent, rst_sent_ratio, true, params.granularity)
  local rst_rcvd_info = alerts_api.anomalousTCPFlagsType(syn_flags_sent, rst_flags_rcvd, rst_rcvd_ratio, false, params.granularity)

  if(rst_sent_ratio > rst_ratio_threshold) then
    alerts_api.trigger(params.alert_entity, rst_sent_info)
  else
    alerts_api.release(params.alert_entity, rst_sent_info)
  end

  if(rst_rcvd_ratio > rst_ratio_threshold) then
    alerts_api.trigger(params.alert_entity, rst_rcvd_info)
  else
    alerts_api.release(params.alert_entity, rst_rcvd_info)
  end
end

-- #################################################################

local function check_misbehaving_flows_ratio(params)
  local info = params.entity_info
  local key = params.check_module.key
  local bad_ratio_threshold = 30 -- 30%

  local cli_flows = alerts_api.host_delta_val(key .. "_cli_flows", params.granularity, info["total_flows.as_client"], true --[[ skip first]])
  local cli_bad_flows = alerts_api.host_delta_val(key .. "_cli_bad_flows", params.granularity, info["anomalous_flows.as_server"], true --[[ skip first]])
  local srv_flows = alerts_api.host_delta_val(key .. "_srv_flows", params.granularity, info["total_flows.as_client"], true --[[ skip first]])
  local srv_bad_flows = alerts_api.host_delta_val(key .. "_srv_bad_flows", params.granularity, info["anomalous_flows.as_server"], true --[[ skip first]])

  local bad_cli_ratio = math.min((cli_bad_flows * 100) / (cli_flows+1), 100)
  local bad_srv_ratio = math.min((srv_bad_flows * 100) / (srv_flows+1), 100)

  local bad_cli_info = alerts_api.misbehavingFlowsRatioType(cli_bad_flows, cli_flows, bad_cli_ratio, true, params.granularity)
  local bad_srv_info = alerts_api.misbehavingFlowsRatioType(srv_bad_flows, srv_flows, bad_srv_ratio, false, params.granularity)

  if(bad_cli_ratio > bad_ratio_threshold) then
    alerts_api.trigger(params.alert_entity, bad_cli_info)
  else
    alerts_api.release(params.alert_entity, bad_cli_info)
  end

  if(bad_srv_ratio > bad_ratio_threshold) then
    alerts_api.trigger(params.alert_entity, bad_srv_info)
  else
    alerts_api.release(params.alert_entity, bad_srv_info)
  end
end

-- #################################################################

local function experimental_checks(params)
  check_tcp_flags(params)
  check_misbehaving_flows_ratio(params)
end

-- #################################################################

check_module = {
  key = "experimental",
  check_function = experimental_checks,
  local_only = true,
  granularity = {"5mins"},

  gui = {
    i18n_title = "alerts_dashboard.experimental_checks",
    i18n_description = "alerts_dashboard.experimental_checks_description",
    input_builder = alerts_api.checkbox_input_builder,
  }
}

-- #################################################################

return check_module
