#include "gui/graph_widget/contexts/module_context.h"

#include "gui/graph_widget/layouters/module_layouter.h"
#include "gui/gui_globals.h"

module_context::module_context(const std::shared_ptr<const module> m) : graph_context(context_type::module, g_graph_context_manager.get_default_layouter(this), g_graph_context_manager.get_default_shader(this)),
    m_id(m->get_id())
{
    for (const std::shared_ptr<module>& s : m->get_submodules())
        m_modules.insert(s->get_id());

    for (const std::shared_ptr<gate>& g : m->get_gates())
        m_gates.insert(g->get_id());

    for (const std::shared_ptr<net>& n: m->get_internal_nets())
        m_internal_nets.insert(n->get_id());

    for (const std::shared_ptr<net>& n : m->get_input_nets())
    {
        if (n->is_unrouted())
            m_global_io_nets.insert(n->get_id());
        else
            m_local_io_nets.insert(n->get_id());
    }

    for (const std::shared_ptr<net>& n : m->get_output_nets())
    {
        if (n->is_unrouted())
            m_global_io_nets.insert(n->get_id());
        else
            m_local_io_nets.insert(n->get_id());
    }

    static_cast<module_layouter*>(m_layouter)->add(m_modules, m_gates, m_internal_nets);
    m_shader->add(m_modules, m_gates, m_internal_nets);

    m_scene_update_required = true;
}

void module_context::add(const QSet<u32>& modules, const QSet<u32>& gates)
{
    QSet<u32> new_modules = modules - m_modules;
    QSet<u32> new_gates = gates - m_gates;

    QSet<u32> old_modules = m_removed_modules & new_modules;
    QSet<u32> old_gates = m_removed_gates & new_gates;

    m_removed_modules -= old_modules;
    m_removed_gates -= old_gates;

    new_modules -= old_modules;
    new_gates -= old_gates;

    m_added_modules += new_modules;
    m_added_gates += new_gates;

    // NET STUFF, CAN PROBABLY BE OPTIMIZED
    std::shared_ptr<module> m = g_netlist->get_module_by_id(m_id);
    assert(m);

    QSet<u32> current_internal_nets;
    for (const std::shared_ptr<net>& n: m->get_internal_nets())
        current_internal_nets.insert(n->get_id());

    m_added_internal_nets = current_internal_nets - m_internal_nets;
    m_removed_internal_nets -= current_internal_nets;

    QSet<u32> current_global_io_nets;
    QSet<u32> current_local_io_nets;

    for (const std::shared_ptr<net>& n : m->get_input_nets())
    {
        if (n->is_unrouted())
            current_global_io_nets.insert(n->get_id());
        else
            current_local_io_nets.insert(n->get_id());
    }

    for (const std::shared_ptr<net>& n : m->get_output_nets())
    {
        if (n->is_unrouted())
            current_global_io_nets.insert(n->get_id());
        else
            current_local_io_nets.insert(n->get_id());
    }

    m_added_global_io_nets = current_global_io_nets - m_global_io_nets;
    m_removed_global_io_nets -= current_global_io_nets;

    m_added_local_io_nets = current_local_io_nets - m_local_io_nets;
    m_removed_local_io_nets -= current_local_io_nets;

    evaluate_changes();
    update();
}

void module_context::remove(const QSet<u32>& modules, const QSet<u32>& gates)
{
    QSet<u32> old_modules = modules & m_modules;
    QSet<u32> old_gates = gates & m_gates;

    m_removed_modules += old_modules;
    m_removed_gates += old_gates;

    m_added_modules -= modules;
    m_added_gates -= gates;

    // NET STUFF, CAN PROBABLY BE OPTIMIZED
    std::shared_ptr<module> m = g_netlist->get_module_by_id(m_id);
    assert(m);

    QSet<u32> current_internal_nets;
    for (const std::shared_ptr<net>& n: m->get_internal_nets())
        current_internal_nets.insert(n->get_id());

    m_added_internal_nets = current_internal_nets - m_internal_nets;
    m_removed_internal_nets = m_internal_nets - current_internal_nets;

    QSet<u32> current_global_io_nets;
    QSet<u32> current_local_io_nets;

    for (const std::shared_ptr<net>& n : m->get_input_nets())
    {
        if (n->is_unrouted())
            current_global_io_nets.insert(n->get_id());
        else
            current_local_io_nets.insert(n->get_id());
    }

    for (const std::shared_ptr<net>& n : m->get_output_nets())
    {
        if (n->is_unrouted())
            current_global_io_nets.insert(n->get_id());
        else
            current_local_io_nets.insert(n->get_id());
    }

    m_added_global_io_nets = current_global_io_nets - m_global_io_nets;
    m_removed_global_io_nets = m_global_io_nets - current_global_io_nets;

    m_added_local_io_nets = current_local_io_nets - m_local_io_nets;
    m_removed_local_io_nets = m_local_io_nets - current_local_io_nets;

    evaluate_changes();
    update();
}

u32 module_context::get_id() const
{
    return m_id;
}

const QSet<u32>& module_context::modules() const
{
    return m_modules;
}

const QSet<u32>& module_context::gates() const
{
    return m_gates;
}

const QSet<u32>& module_context::internal_nets() const
{
    return m_internal_nets;
}

const QSet<u32>& module_context::global_io_nets() const
{
    return m_global_io_nets;
}

const QSet<u32>& module_context::local_io_nets() const
{
    return m_local_io_nets;
}

void module_context::evaluate_changesets()
{
    if (!m_added_modules.isEmpty()          ||
        !m_added_gates.isEmpty()            ||
        !m_added_internal_nets.isEmpty()    ||
        !m_added_global_io_nets.isEmpty()   ||
        !m_added_local_io_nets.isEmpty()    ||
        !m_removed_modules.isEmpty()        ||
        !m_removed_gates.isEmpty()          ||
        !m_removed_internal_nets.isEmpty()  ||
        !m_removed_global_io_nets.isEmpty() ||
        !m_removed_local_io_nets.isEmpty())

        m_unhandled_changes = true;
}

void module_context::apply_changesets()
{
    m_modules -= m_removed_modules;
    m_gates -= m_removed_gates;
    m_internal_nets -= m_removed_internal_nets;
    m_global_io_nets -= m_removed_global_io_nets;
    m_local_io_nets -= m_removed_local_io_nets;

    m_modules += m_added_modules;
    m_gates += m_added_gates;
    m_internal_nets += m_added_internal_nets;

    static_cast<module_layouter*>m_layouter->remove(m_removed_modules, m_removed_gates, m_removed_nets);
    static_cast<module_layouter*>m_layouter->add(m_added_modules, m_added_gates, m_added_nets);

    m_shader->remove(m_removed_modules, m_removed_gates, m_removed_nets);
    m_shader->add(m_added_modules, m_added_gates, m_added_nets);

    m_added_modules.clear();
    m_added_gates.clear();
    m_added_internal_nets.clear();
    m_added_global_io_nets.clear();
    m_added_local_io_nets.clear();

    m_removed_modules.clear();
    m_removed_gates.clear();
    m_removed_internal_nets.clear();
    m_removed_global_io_nets.clear();
    m_removed_local_io_nets.clear();

    m_unhandled_changes = false;
    m_scene_update_required = true;
}
