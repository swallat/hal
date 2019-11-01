#include "netlist/netlist.h"

#include "netlist/gate.h"
#include "netlist/module.h"
#include "netlist/net.h"
#include "netlist/netlist_constants.h"
#include "netlist/netlist_internal_manager.h"

#include "netlist/event_system/netlist_event_handler.h"

#include "core/log.h"

netlist::netlist(std::shared_ptr<gate_library> library) : m_gate_library(library)
{
    m_manager        = new netlist_internal_manager(this);
    m_netlist_id     = 1;
    m_next_gate_id   = 1;
    m_next_net_id    = 1;
    m_next_module_id = 1;
    m_top_module     = nullptr;    // this triggers the internal manager to allow creation of a module without parent
    m_top_module     = create_module("top module", nullptr);
}

netlist::~netlist()
{
    delete m_manager;
}

std::shared_ptr<netlist> netlist::get_shared()
{
    return shared_from_this();
}

u32 netlist::get_id() const
{
    return m_netlist_id;
}

void netlist::set_id(const u32 id)
{
    if (id != m_netlist_id)
    {
        auto old_id  = m_netlist_id;
        m_netlist_id = id;
        netlist_event_handler::notify(netlist_event_handler::event::id_changed, shared_from_this(), old_id);
    }
}

hal::path netlist::get_input_filename() const
{
    return m_file_name;
}

void netlist::set_input_filename(const hal::path& input_filename)
{
    if (input_filename != m_file_name)
    {
        m_file_name = input_filename;
        netlist_event_handler::notify(netlist_event_handler::event::input_filename_changed, shared_from_this());
    }
}

std::string netlist::get_design_name() const
{
    return m_design_name;
}

void netlist::set_design_name(const std::string& design_name)
{
    if (design_name != m_design_name)
    {
        m_design_name = design_name;
        netlist_event_handler::notify(netlist_event_handler::event::design_name_changed, shared_from_this());
    }
}

std::string netlist::get_device_name() const
{
    return m_device_name;
}

void netlist::set_device_name(const std::string& device_name)
{
    if (device_name != m_device_name)
    {
        m_device_name = device_name;
        netlist_event_handler::notify(netlist_event_handler::event::device_name_changed, shared_from_this());
    }
}

std::shared_ptr<gate_library> netlist::get_gate_library() const
{
    return m_gate_library;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

u32 netlist::get_unique_module_id()
{
    if (!m_free_module_ids.empty())
    {
        return *(m_free_module_ids.begin());
    }
    while (m_used_module_ids.find(m_next_module_id) != m_used_module_ids.end())
    {
        m_next_module_id++;
    }
    return m_next_module_id;
}

std::shared_ptr<module> netlist::create_module(const u32 id, const std::string& name, std::shared_ptr<module> parent)
{
    return m_manager->create_module(id, parent, name);
}

std::shared_ptr<module> netlist::create_module(const std::string& name, std::shared_ptr<module> parent)
{
    return create_module(get_unique_module_id(), name, parent);
}

bool netlist::delete_module(const std::shared_ptr<module> module)
{
    return m_manager->delete_module(module);
}

std::shared_ptr<module> netlist::get_top_module()
{
    return m_top_module;
}

std::shared_ptr<module> netlist::get_module_by_id(u32 id) const
{
    auto it = m_modules.find(id);
    if (it == m_modules.end())
    {
        log_error("netlist", "there is no module with id = {}.", id);
        return nullptr;
    }
    return it->second;
}

std::set<std::shared_ptr<module>> netlist::get_modules() const
{
    std::set<std::shared_ptr<module>> res;
    for (const auto& it : m_modules)
    {
        res.insert(it.second);
    }
    return res;
}

bool netlist::is_module_in_netlist(const std::shared_ptr<module> module) const
{
    return (module != nullptr) && (module->get_netlist() == shared_from_this()) && (m_modules.find(module->get_id()) != m_modules.end());
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

u32 netlist::get_unique_gate_id()
{
    if (!m_free_gate_ids.empty())
    {
        return *(m_free_gate_ids.begin());
    }
    while (m_used_gate_ids.find(m_next_gate_id) != m_used_gate_ids.end())
    {
        m_next_gate_id++;
    }
    return m_next_gate_id;
}

std::shared_ptr<gate> netlist::create_gate(const u32 id, const std::string& gate_type, const std::string& name, float x, float y)
{
    return m_manager->create_gate(id, gate_type, name, x, y);
}

std::shared_ptr<gate> netlist::create_gate(const std::string& gate_type, const std::string& name, float x, float y)
{
    return create_gate(get_unique_gate_id(), gate_type, name, x, y);
}

bool netlist::delete_gate(std::shared_ptr<gate> gate)
{
    return m_manager->delete_gate(gate);
}

bool netlist::is_gate_in_netlist(std::shared_ptr<gate> const gate) const
{
    return m_top_module->contains_gate(gate, true);
}

std::shared_ptr<gate> netlist::get_gate_by_id(const u32 gate_id) const
{
    return m_top_module->get_gate_by_id(gate_id, true);
}

std::set<std::shared_ptr<gate>> netlist::get_gates(const std::string& gate_type_filter, const std::string& name_filter) const
{
    return m_top_module->get_gates(gate_type_filter, name_filter, true);
}

u32 netlist::get_number_of_gates()
{
    return m_top_module->get_gates(DONT_CARE, DONT_CARE, true).size();
}

bool netlist::mark_global_vcc_gate(const std::shared_ptr<gate> gate)
{
    if (!is_gate_in_netlist(gate))
    {
        return false;
    }
    if (m_global_vcc_gates.find(gate) != m_global_vcc_gates.end())
    {
        log_debug("netlist", "gate '{}' (id = {:08x}) is already registered as global vcc gate in netlist.", gate->get_name(), gate->get_id());
        return true;
    }
    m_global_vcc_gates.insert(gate);
    netlist_event_handler::notify(netlist_event_handler::event::marked_global_vcc, shared_from_this(), gate->get_id());
    return true;
}

bool netlist::mark_global_gnd_gate(const std::shared_ptr<gate> gate)
{
    if (!is_gate_in_netlist(gate))
    {
        return false;
    }
    if (m_global_gnd_gates.find(gate) != m_global_gnd_gates.end())
    {
        log_debug("netlist", "gate '{}' (id = {:08x}) is already registered as global gnd gate in netlist.", gate->get_name(), gate->get_id());
        return true;
    }
    m_global_gnd_gates.insert(gate);
    netlist_event_handler::notify(netlist_event_handler::event::marked_global_gnd, shared_from_this(), gate->get_id());
    return true;
}

bool netlist::unmark_global_vcc_gate(const std::shared_ptr<gate> gate)
{
    if (!is_gate_in_netlist(gate))
    {
        return false;
    }
    auto it = m_global_vcc_gates.find(gate);
    if (it == m_global_vcc_gates.end())
    {
        log_debug("netlist", "gate '{}' (id = {:08x}) is not registered as a global vcc gate in netlist.", gate->get_name(), gate->get_id());
        return false;
    }
    m_global_vcc_gates.erase(it);
    netlist_event_handler::notify(netlist_event_handler::event::unmarked_global_vcc, shared_from_this(), gate->get_id());
    return true;
}

bool netlist::unmark_global_gnd_gate(const std::shared_ptr<gate> gate)
{
    if (!is_gate_in_netlist(gate))
    {
        return false;
    }
    auto it = m_global_gnd_gates.find(gate);
    if (it == m_global_gnd_gates.end())
    {
        log_debug("netlist", "gate '{}' (id = {:08x}) is not registered as a global gnd gate in netlist.", gate->get_name(), gate->get_id());
        return false;
    }
    m_global_gnd_gates.erase(it);
    netlist_event_handler::notify(netlist_event_handler::event::unmarked_global_gnd, shared_from_this(), gate->get_id());
    return true;
}

bool netlist::is_global_vcc_gate(const std::shared_ptr<gate> gate) const
{
    return (m_global_vcc_gates.find(gate) != m_global_vcc_gates.end());
}

bool netlist::is_global_gnd_gate(const std::shared_ptr<gate> gate) const
{
    return (m_global_gnd_gates.find(gate) != m_global_gnd_gates.end());
}

std::set<std::shared_ptr<gate>> netlist::get_global_vcc_gates() const
{
    return m_global_vcc_gates;
}

std::set<std::shared_ptr<gate>> netlist::get_global_gnd_gates() const
{
    return m_global_gnd_gates;
}

std::vector<std::string> netlist::get_input_pin_types(const std::string& gate_type) const
{
    auto gate_type_to_input_pin_types = m_gate_library->get_gate_type_map_to_input_pin_types();
    auto it                           = gate_type_to_input_pin_types->find(gate_type);
    if (it == gate_type_to_input_pin_types->end())
    {
        log_debug("netlist", "parameter 'gate_type' (= {}) is invalid.", gate_type);
        return std::vector<std::string>();
    }
    return it->second;
}

std::vector<std::string> netlist::get_output_pin_types(const std::string& gate_type) const
{
    auto gate_type_to_output_pin_types = m_gate_library->get_gate_type_map_to_output_pin_types();
    auto it                            = gate_type_to_output_pin_types->find(gate_type);
    if (it == gate_type_to_output_pin_types->end())
    {
        log_debug("netlist", "parameter 'gate_type' (= {}) is invalid.", gate_type);
        return std::vector<std::string>();
    }
    return it->second;
}

std::vector<std::string> netlist::get_inout_pin_types(const std::string& gate_type) const
{
    auto gate_type_to_inout_pin_types = m_gate_library->get_gate_type_map_to_inout_pin_types();
    auto it                           = gate_type_to_inout_pin_types->find(gate_type);
    if (it == gate_type_to_inout_pin_types->end())
    {
        log_debug("netlist", "parameter 'gate_type' (= {}) is invalid.", gate_type);
        return std::vector<std::string>();
    }
    return it->second;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

u32 netlist::get_unique_net_id()
{
    if (!m_free_net_ids.empty())
    {
        return *(m_free_net_ids.begin());
    }
    while (m_used_net_ids.find(m_next_net_id) != m_used_net_ids.end())
    {
        m_next_net_id++;
    }
    return m_next_net_id;
}

u32 netlist::get_number_of_nets()
{
    return m_nets_set.size();
}

std::shared_ptr<net> netlist::create_net(const u32 id, const std::string& name)
{
    return m_manager->create_net(id, name);
}

std::shared_ptr<net> netlist::create_net(const std::string& name)
{
    return m_manager->create_net(get_unique_net_id(), name);
}

bool netlist::delete_net(std::shared_ptr<net> n)
{
    return m_manager->delete_net(n);
}

bool netlist::is_net_in_netlist(const std::shared_ptr<net> n) const
{
    return m_nets_set.find(n) != m_nets_set.end();
}

std::shared_ptr<net> netlist::get_net_by_id(u32 net_id) const
{
    auto it = m_nets_map.find(net_id);
    if (it == m_nets_map.end())
    {
        log_error("netlist", "no net with id {:08x} registered in netlist.", net_id);
        return nullptr;
    }
    return it->second;
}

std::set<std::shared_ptr<net>> netlist::get_nets(const std::string& name_filter) const
{
    if (name_filter == DONT_CARE)
    {
        return m_nets_set;
    }
    std::set<std::shared_ptr<net>> res;
    for (const auto& x : m_nets_set)
    {
        if (x->get_name() != name_filter)
        {
            continue;
        }
        res.insert(x);
    }
    return res;
}

bool netlist::mark_global_input_net(std::shared_ptr<net> const n)
{
    if (!is_net_in_netlist(n))
    {
        return false;
    }
    if (m_global_input_nets.find(n) != m_global_input_nets.end())
    {
        log_debug("netlist", "net '{}' (id = {:08x}) is already registered as global input net in netlist.", n->get_name(), n->get_id());
        return true;
    }
    m_global_input_nets.insert(n);

    netlist_event_handler::notify(netlist_event_handler::event::marked_global_input, shared_from_this(), n->get_id());
    return true;
}

bool netlist::mark_global_output_net(std::shared_ptr<net> const n)
{
    if (!is_net_in_netlist(n))
    {
        return false;
    }
    if (m_global_output_nets.find(n) != m_global_output_nets.end())
    {
        log_debug("netlist", "net '{}' (id = {:08x}) is already registered as global output net in netlist", n->get_name(), n->get_id());
        return true;
    }
    m_global_output_nets.insert(n);

    netlist_event_handler::notify(netlist_event_handler::event::marked_global_output, shared_from_this(), n->get_id());
    return true;
}

bool netlist::mark_global_inout_net(std::shared_ptr<net> const n)
{
    if (!is_net_in_netlist(n))
    {
        return false;
    }
    if (m_global_inout_nets.find(n) != m_global_inout_nets.end())
    {
        log_debug("netlist", "net '{}' (id = {:08x}) is already registered as global inout net in netlist", n->get_name(), n->get_id());
        return true;
    }
    m_global_inout_nets.insert(n);

    netlist_event_handler::notify(netlist_event_handler::event::marked_global_inout, shared_from_this(), n->get_id());
    return true;
}

bool netlist::unmark_global_input_net(std::shared_ptr<net> const n)
{
    if (!is_net_in_netlist(n))
    {
        return false;
    }
    auto it = m_global_input_nets.find(n);
    if (it == m_global_input_nets.end())
    {
        log_debug("netlist", "net '{}' (id = {:08x}) is not registered as global input net in netlist.", n->get_name(), n->get_id());
        return false;
    }
    m_global_input_nets.erase(it);

    netlist_event_handler::notify(netlist_event_handler::event::unmarked_global_input, shared_from_this(), n->get_id());
    return true;
}

bool netlist::unmark_global_output_net(std::shared_ptr<net> const n)
{
    if (!is_net_in_netlist(n))
    {
        return false;
    }
    auto it = m_global_output_nets.find(n);
    if (it == m_global_output_nets.end())
    {
        log_debug("netlist", "net '{}' (id = {:08x}) is not registered as global output net in netlist.", n->get_name(), n->get_id());
        return false;
    }
    m_global_output_nets.erase(it);

    netlist_event_handler::notify(netlist_event_handler::event::unmarked_global_output, shared_from_this(), n->get_id());
    return true;
}

bool netlist::unmark_global_inout_net(std::shared_ptr<net> const n)
{
    if (!is_net_in_netlist(n))
    {
        return false;
    }
    auto it = m_global_inout_nets.find(n);
    if (it == m_global_inout_nets.end())
    {
        log_debug("netlist", "net '{}' (id = {:08x}) is not registered as global inout net in netlist.", n->get_name(), n->get_id());
        return false;
    }
    m_global_inout_nets.erase(it);

    netlist_event_handler::notify(netlist_event_handler::event::unmarked_global_inout, shared_from_this(), n->get_id());
    return true;
}

bool netlist::is_global_input_net(std::shared_ptr<net> const n) const
{
    return (m_global_input_nets.find(n) != m_global_input_nets.end());
}

bool netlist::is_global_output_net(std::shared_ptr<net> const n) const
{
    return (m_global_output_nets.find(n) != m_global_output_nets.end());
}

bool netlist::is_global_inout_net(std::shared_ptr<net> const n) const
{
    return (m_global_inout_nets.find(n) != m_global_inout_nets.end());
}

std::set<std::shared_ptr<net>> netlist::get_global_input_nets() const
{
    return m_global_input_nets;
}

std::set<std::shared_ptr<net>> netlist::get_global_output_nets() const
{
    return m_global_output_nets;
}

std::set<std::shared_ptr<net>> netlist::get_global_inout_nets() const
{
    return m_global_inout_nets;
}
