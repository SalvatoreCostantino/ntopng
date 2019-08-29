/*
 *
 * (C) 2013-19 - ntop.org
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

// #define DEBUG_BROADCAST_DOMAINS

/* *************************************** */

BroadcastDomains::BroadcastDomains(NetworkInterface *_iface) {
  iface = _iface;
  inline_broadcast_domains = new (std::nothrow) AddressTree(false);
  broadcast_domains = broadcast_domains_shadow = NULL;
  next_update = last_update = 0;
  next_domain_id = 0;
}

/* *************************************** */

BroadcastDomains::~BroadcastDomains() {
  if(inline_broadcast_domains) { delete(inline_broadcast_domains); inline_broadcast_domains = NULL; }
  if(broadcast_domains)        { delete(broadcast_domains); broadcast_domains = NULL; }
  if(broadcast_domains_shadow) { delete(broadcast_domains_shadow); broadcast_domains_shadow = NULL; }
}

/* *************************************** */

void BroadcastDomains::inlineAddAddress(const IpAddress * const ipa, int network_bits) {
  patricia_node_t *addr_node;

  if(!inline_broadcast_domains)
    return;

  if(next_domain_id == ((u_int16_t)-1)) {
    ntop->getTrace()->traceEvent(TRACE_WARNING, "Too many broadcast domains defined on interface %s", iface->get_name());
    return;
  }

  if((addr_node = inline_broadcast_domains->match(ipa, network_bits)) != NULL) {
    /* Already exists, only increment the hits */
    domains_info[addr_node->user_data].hits++;

#ifdef DEBUG_BROADCAST_DOMAINS
    {
      char buf[128];
      ntop->getTrace()->traceEvent(TRACE_NORMAL, "Broadcast domain %s/%d [hits=%u]", ipa->print(buf, sizeof(buf)), network_bits, domains_info[addr_node->user_data].hits);
    }
#endif

    return;
  }

#ifdef DEBUG_BROADCAST_DOMAINS
    {
      char buf[128];
      ntop->getTrace()->traceEvent(TRACE_NORMAL, "New broadcast domain: %s/%d", ipa->print(buf, sizeof(buf)), network_bits);
    }
#endif

  addr_node = inline_broadcast_domains->addAddress(ipa, network_bits, true /* Compact after add */);

  if(addr_node) {
    struct bcast_domain_info info;
    info.is_ghost_network = !iface->isInterfaceNetwork(ipa, network_bits);
    info.hits = 1;

    addr_node->user_data = next_domain_id++;
    domains_info[addr_node->user_data] = info;
  }

  if(!next_update)
    next_update = time(NULL) + 1;
}

/* *************************************** */

void BroadcastDomains::inlineReloadBroadcastDomains(bool force_immediate_reload) {
  time_t now = time(NULL);

  if(force_immediate_reload)
    goto reload;

  if(next_update) {
    if(now > next_update) {
      /* do the swap */
    reload:
      if(broadcast_domains_shadow)
	delete broadcast_domains_shadow;

      broadcast_domains_shadow = broadcast_domains;
      broadcast_domains = new (std::nothrow) AddressTree(*inline_broadcast_domains);

      last_update = now;
      next_update = 0;
    }
  }

  if(broadcast_domains_shadow && now > last_update + 1) {
    delete broadcast_domains_shadow;
    broadcast_domains_shadow = NULL;
  }
}

/* *************************************** */

bool BroadcastDomains::isLocalBroadcastDomain(const IpAddress * const ipa, int network_bits, bool isInlineCall) const {
  AddressTree *cur_tree = isInlineCall ? inline_broadcast_domains : broadcast_domains;

  return cur_tree && cur_tree->match(ipa, network_bits);
}

/* *************************************** */

bool BroadcastDomains::isLocalBroadcastDomainHost(const Host * const h, bool isInlineCall) const {
  AddressTree *cur_tree = isInlineCall ? inline_broadcast_domains : broadcast_domains;

  if(cur_tree && h)
    return h->match(cur_tree);

  return false;
}

/* *************************************** */

struct bcast_domain_walk_data {
  std::map<u_int16_t, bcast_domain_info> domains_info;
  lua_State *vm;
};

static void bcast_domain_lua(patricia_node_t *node, void *data, void *user_data) {
  char address[128];
  struct bcast_domain_walk_data *bdata = (struct bcast_domain_walk_data *)user_data;
  std::map<u_int16_t, bcast_domain_info>::iterator it;

  if((it = bdata->domains_info.find(node->user_data)) != bdata->domains_info.end()) {
    lua_State *vm = bdata->vm;

    if((!node->prefix) || !Utils::ptree_prefix_print(node->prefix, address, sizeof(address)))
      return;

    lua_newtable(vm);
    lua_push_bool_table_entry(vm, "ghost_network", it->second.is_ghost_network);
    lua_push_uint64_table_entry(vm, "hits", it->second.hits);

    lua_pushstring(vm, address);
    lua_insert(vm, -2);
    lua_settable(vm, -3);
  }
}

/* *************************************** */

void BroadcastDomains::lua(lua_State *vm) const {
  AddressTree *cur_tree = broadcast_domains;
  struct bcast_domain_walk_data data;

  /* Make a copy to avoid iterating it during inline insertion */
  data.domains_info = domains_info;
  data.vm = vm;

  lua_newtable(vm);

  if(cur_tree)
    cur_tree->walk(bcast_domain_lua, &data);

  lua_pushstring(vm, "bcast_domains");
  lua_insert(vm, -2);
  lua_settable(vm, -3);
}
