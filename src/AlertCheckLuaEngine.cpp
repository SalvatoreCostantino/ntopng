/*
 *
 * (C) 2019 - ntop.org
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "ntop_includes.h"

/* ****************************************** */

AlertCheckLuaEngine::AlertCheckLuaEngine(AlertEntity alert_entity, ScriptPeriodicity script_periodicity,  NetworkInterface *iface) : LuaEngine() {
  const char *lua_file = NULL;

  p = script_periodicity;

  switch(alert_entity) {
  case alert_entity_host:
    lua_file = "host.lua";
    break;
  case alert_entity_network:
    lua_file = "network.lua";
    break;
  case alert_entity_interface:
    lua_file = "interface.lua";
    break;
  default:
    /* Example: lua_file = "generic.lua" to handle a generic entity */
    break;
  }

  if(lua_file) {
    snprintf(script_path, sizeof(script_path),
	     "%s/callbacks/interface/alerts/%s",
	     ntop->getPrefs()->get_scripts_dir(),
	     lua_file);
    ntop->fixPath(script_path);

    load_script(script_path, iface);

    lua_getglobal(L, "setup");         /* Called function   */
    lua_pushstring(L, Utils::periodicityToScriptName(p)); /* push 1st argument */

    if(!pcall(1 /* 1 argument */, 0))
      return;
  } else {
    /* Possibly handle a generic entity */
    script_path[0] = '0';
  }
}

/* ****************************************** */

ScriptPeriodicity AlertCheckLuaEngine::getPeriodicity() const {
  return p;
}

/* ****************************************** */

const char * AlertCheckLuaEngine::getGranularity() const {
  return Utils::periodicityToScriptName(p);
}

/* ****************************************** */

bool AlertCheckLuaEngine::pcall(int num_args, int num_results) {
  if(lua_pcall(L, num_args, num_results, 0)) {
    ntop->getTrace()->traceEvent(TRACE_WARNING, "Script failure [%s]", lua_tostring(L, -1));
    return(false);
  }

  /*
    Refresh entity (if necessary): this guarantees that we do at most one
    refresh per entity, regardless of the number of triggered alerts
   */
  if(getHost())
    getHost()->refreshAlerts();
  else if(getNetwork())
    getNetwork()->refreshAlerts();
  else if(getNetworkInterface())
    getNetworkInterface()->refreshAlerts();

  return(true);
}
