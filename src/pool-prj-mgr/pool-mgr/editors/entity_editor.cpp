#include "entity_editor.hpp"
#include <iostream>
#include "pool/entity.hpp"
#include <glibmm.h>
#include "dialogs/pool_browser_dialog.hpp"
#include "widgets/pool_browser.hpp"
#include "widgets/tag_entry.hpp"
#include "util/pool_completion.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

class GateEditor : public Gtk::Box {
public:
    GateEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Gate *g, EntityEditor *pa);
    static GateEditor *create(class Gate *g, EntityEditor *pa);
    class Gate *gate;
    EntityEditor *parent;
    void reload();

private:
    Gtk::Entry *name_entry = nullptr;
    Gtk::Entry *suffix_entry = nullptr;
    Gtk::SpinButton *swap_group_spin_button = nullptr;
    Gtk::Label *unit_label = nullptr;
};

GateEditor::GateEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Gate *g, EntityEditor *pa)
    : Gtk::Box(cobject), gate(g), parent(pa)
{
    x->get_widget("gate_name", name_entry);
    x->get_widget("gate_suffix", suffix_entry);
    x->get_widget("gate_swap_group", swap_group_spin_button);
    x->get_widget("gate_unit", unit_label);
    entry_add_sanitizer(name_entry);
    entry_add_sanitizer(suffix_entry);

    unit_label->set_track_visited_links(false);
    unit_label->signal_activate_link().connect(
            [this](const std::string &url) {
                parent->s_signal_goto.emit(ObjectType::UNIT, UUID(url));
                return true;
            },
            false);

    name_entry->set_text(gate->name);
    name_entry->signal_changed().connect([this] {
        gate->name = name_entry->get_text();
        parent->needs_save = true;
    });

    suffix_entry->set_text(gate->suffix);
    suffix_entry->signal_changed().connect([this] {
        gate->suffix = suffix_entry->get_text();
        parent->needs_save = true;
    });

    swap_group_spin_button->set_value(gate->swap_group);
    swap_group_spin_button->signal_value_changed().connect([this] {
        gate->swap_group = swap_group_spin_button->get_value_as_int();
        parent->needs_save = true;
    });

    reload();
}

void GateEditor::reload()
{
    unit_label->set_markup("<a href=\"" + (std::string)gate->unit->uuid + "\">"
                           + Glib::Markup::escape_text(gate->unit->name) + "</a>");
}

GateEditor *GateEditor::create(Gate *g, EntityEditor *pa)
{
    GateEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/pool-mgr/editors/entity_editor.ui");
    x->get_widget_derived("gate_editor", w, g, pa);
    w->reference();
    return w;
}

static void bind_entry(Gtk::Entry *e, std::string &s, bool &needs_save)
{
    e->set_text(s);
    e->signal_changed().connect([e, &s, &needs_save] {
        s = e->get_text();
        needs_save = true;
    });
}

EntityEditor::EntityEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Entity *e, Pool *p)
    : Gtk::Box(cobject), entity(e), pool(p)
{
    x->get_widget("entity_name", name_entry);
    x->get_widget("entity_manufacturer", manufacturer_entry);
    {
        Gtk::Box *tag_box;
        x->get_widget("entity_tags", tag_box);
        tag_entry = Gtk::manage(new TagEntry(*pool, ObjectType::ENTITY, true));
        tag_entry->show();
        tag_box->pack_start(*tag_entry, true, true, 0);
    }
    x->get_widget("entity_prefix", prefix_entry);
    x->get_widget("entity_gates", gates_listbox);
    x->get_widget("entity_gates_refresh", refresh_button);
    x->get_widget("gate_add", add_button);
    x->get_widget("gate_delete", delete_button);
    entry_add_sanitizer(name_entry);
    entry_add_sanitizer(manufacturer_entry);
    entry_add_sanitizer(prefix_entry);

    bind_entry(name_entry, entity->name, needs_save);
    bind_entry(manufacturer_entry, entity->manufacturer, needs_save);
    manufacturer_entry->set_completion(create_pool_manufacturer_completion(pool));
    bind_entry(prefix_entry, entity->prefix, needs_save);

    tag_entry->set_tags(entity->tags);
    tag_entry->signal_changed().connect([this] {
        entity->tags = tag_entry->get_tags();
        needs_save = true;
    });

    gates_listbox->set_sort_func([](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
        auto na = dynamic_cast<GateEditor *>(a->get_child())->gate->name;
        auto nb = dynamic_cast<GateEditor *>(b->get_child())->gate->name;
        if (na > nb) {
            return 1;
        }
        else if (na < nb) {
            return -1;
        }
        else {
            return 0;
        }
    });

    for (auto &it : entity->gates) {
        auto ed = GateEditor::create(&it.second, this);
        ed->show_all();
        gates_listbox->append(*ed);
        ed->unreference();
    }
    gates_listbox->invalidate_sort();

    add_button->signal_clicked().connect(sigc::mem_fun(this, &EntityEditor::handle_add));
    delete_button->signal_clicked().connect(sigc::mem_fun(this, &EntityEditor::handle_delete));
    refresh_button->signal_clicked().connect([this] { gates_listbox->invalidate_sort(); });
}

void EntityEditor::reload()
{
    entity->update_refs(*pool);
    auto children = gates_listbox->get_children();
    for (auto &ch : children) {
        auto row = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto ed = dynamic_cast<GateEditor *>(row->get_child());
        ed->reload();
    }
}

void EntityEditor::handle_delete()
{
    auto rows = gates_listbox->get_selected_rows();
    std::set<int> indices;
    std::set<UUID> uuids;
    for (auto &row : rows) {
        uuids.insert(dynamic_cast<GateEditor *>(row->get_child())->gate->uuid);
        indices.insert(row->get_index());
    }
    for (auto &row : rows) {
        delete row;
    }
    for (auto it = entity->gates.begin(); it != entity->gates.end();) {
        if (uuids.find(it->first) != uuids.end()) {
            entity->gates.erase(it++);
        }
        else {
            it++;
        }
    }
    for (auto index : indices) {
        auto row = gates_listbox->get_row_at_index(index);
        if (row)
            gates_listbox->select_row(*row);
    }
    needs_save = true;
}


void EntityEditor::handle_add()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    PoolBrowserDialog dia(top, ObjectType::UNIT, pool);
    if (dia.run() == Gtk::RESPONSE_OK) {
        auto unit_uu = dia.get_browser()->get_selected();
        if (unit_uu) {
            auto uu = UUID::random();
            auto gate = &entity->gates.emplace(uu, uu).first->second;
            gate->unit = pool->get_unit(unit_uu);

            auto ed = GateEditor::create(gate, this);
            ed->show_all();
            gates_listbox->append(*ed);
            ed->unreference();

            auto children = gates_listbox->get_children();
            for (auto &ch : children) {
                auto row = dynamic_cast<Gtk::ListBoxRow *>(ch);
                auto edi = dynamic_cast<GateEditor *>(row->get_child());
                if (edi->gate->uuid == uu) {
                    gates_listbox->unselect_all();
                    gates_listbox->select_row(*row);
                    break;
                }
            }

            gates_listbox->invalidate_sort();
        }
    }
    needs_save = true;
}

EntityEditor *EntityEditor::create(Entity *e, class Pool *p)
{
    EntityEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/pool-mgr/editors/entity_editor.ui");
    x->get_widget_derived("entity_editor", w, e, p);
    w->reference();
    return w;
}
} // namespace horizon
