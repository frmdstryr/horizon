#pragma once
#include "core/clipboard.hpp"
#include "core/core.hpp"
#include "core/cores.hpp"
#include "imp_interface.hpp"
#include "keyseq_dialog.hpp"
#include "main_window.hpp"
#include "pool/pool.hpp"
#include "preferences/preferences.hpp"
#include "selection_filter_dialog.hpp"
#include "util/window_state_store.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/warnings_box.hpp"
#include "action.hpp"
#include <zmq.hpp>

#ifdef G_OS_WIN32
#undef DELETE
#undef DUPLICATE
#endif

namespace horizon {

class PoolParams {
public:
    PoolParams(const std::string &bp, const std::string &cp = "") : base_path(bp), cache_path(cp)
    {
    }
    std::string base_path;
    std::string cache_path;
};

std::unique_ptr<Pool> make_pool(const PoolParams &params);

class ImpBase : public sigc::trackable {
    friend class ImpInterface;

public:
    ImpBase(const PoolParams &params);
    void run(int argc, char *argv[]);
    virtual void handle_tool_change(ToolID id);
    virtual void construct() = 0;
    void canvas_update_from_pp();
    virtual ~ImpBase()
    {
    }
    void set_read_only(bool v);

    std::set<ObjectRef> highlights;
    virtual void update_highlights(){};

protected:
    MainWindow *main_window;
    class CanvasGL *canvas;
    class PropertyPanels *panels;
    WarningsBox *warnings_box;
    class ToolPopover *tool_popover;
    Gtk::Menu *context_menu = nullptr;
    SpinButtonDim *grid_spin_button;
    std::unique_ptr<SelectionFilterDialog> selection_filter_dialog;

    std::unique_ptr<Pool> pool;
    Cores core;
    std::unique_ptr<ClipboardManager> clipboard = nullptr;
    std::unique_ptr<KeySequenceDialog> key_sequence_dialog = nullptr;
    std::unique_ptr<ImpInterface> imp_interface = nullptr;
    Glib::RefPtr<Glib::Binding> grid_spacing_binding;

    std::map<std::pair<ActionID, ToolID>, ActionConnection> action_connections;
    ActionConnection &connect_action(ToolID tool_id, std::function<void(const ActionConnection &)> cb);
    ActionConnection &connect_action(ToolID tool_id);
    ActionConnection &connect_action(ActionID action_id, std::function<void(const ActionConnection &)> cb);

    class RulesWindow *rules_window = nullptr;

    zmq::context_t zctx;
    zmq::socket_t sock_broadcast_rx;
    zmq::socket_t sock_project;
    bool sockets_connected = false;
    int mgr_pid = -1;
    bool no_update = false;

    virtual void canvas_update() = 0;
    void sc(void);
    bool handle_key_press(GdkEventKey *key_event);
    void handle_cursor_move(const Coordi &pos);
    bool handle_click(GdkEventButton *button_event);
    bool handle_click_release(GdkEventButton *button_event);
    bool handle_context_menu(GdkEventButton *button_event);
    void tool_process(const ToolResponse &resp);
    void tool_begin(ToolID id, bool override_selection = false, const std::set<SelectableRef> &sel = {});
    void add_tool_button(ToolID id, const std::string &label, bool left = true);
    void handle_warning_selected(const Coordi &pos);
    virtual bool handle_broadcast(const json &j);
    bool handle_close(GdkEventAny *ev);
    json send_json(const json &j);

    bool trigger_action(const std::pair<ActionID, ToolID> &action);
    bool trigger_action(ActionID aid);
    bool trigger_action(ToolID tid);

    void add_tool_action(ToolID tid, const std::string &action);
    Glib::RefPtr<Gio::Menu> add_hamburger_menu();

    Preferences preferences;

    virtual CanvasPreferences *get_canvas_preferences()
    {
        return &preferences.canvas_non_layer;
    }
    virtual void apply_preferences();

    std::unique_ptr<WindowStateStore> state_store = nullptr;

    virtual void handle_maybe_drag();

    virtual ActionCatalogItem::Availability get_editor_type_for_action() const = 0;
    virtual ObjectType get_editor_type() const = 0;

    void layer_up_down(bool up);
    void goto_layer(int layer);

    Gtk::Button *create_action_button(std::pair<ActionID, ToolID> action);

    void set_action_sensitive(std::pair<ActionID, ToolID>, bool v);
    bool get_action_sensitive(std::pair<ActionID, ToolID>) const;
    virtual void update_action_sensitivity();

    typedef sigc::signal<void> type_signal_action_sensitive;
    type_signal_action_sensitive signal_action_sensitive()
    {
        return s_signal_action_sensitive;
    }

    virtual std::string get_hud_text(std::set<SelectableRef> &sel);
    std::string get_hud_text_for_part(const Part *part);
    std::string get_hud_text_for_net(const Net *net);

    void set_monitor_files(const std::set<std::string> &files);
    void set_monitor_items(const std::set<std::pair<ObjectType, UUID>> &items);
    virtual void update_monitor()
    {
    }
    void edit_pool_item(ObjectType type, const UUID &uu);

    void parameter_window_add_polygon_expand(class ParameterWindow *parameter_window);

    bool read_only = false;

    void tool_update_data(std::unique_ptr<ToolData> &data);

    virtual void search_center(const Core::SearchResult &res);

private:
    void fix_cursor_pos();
    Glib::RefPtr<Gio::FileMonitor> preferences_monitor;
    void handle_drag();
    void update_selection_label();
    std::string get_tool_settings_filename(ToolID id);

    void hud_update();

    std::map<std::string, Glib::RefPtr<Gio::FileMonitor>> file_monitors;

    void handle_file_changed(const Glib::RefPtr<Gio::File> &file1, const Glib::RefPtr<Gio::File> &file2,
                             Gio::FileMonitorEvent ev);

    ActionConnection &connect_action(ActionID action_id, ToolID tool_id,
                                     std::function<void(const ActionConnection &)> cb);

    void create_context_menu(Gtk::Menu *parent, const std::set<SelectableRef> &sel);

    KeySequence2 keys_current;
    bool handle_action_key(GdkEventKey *ev);
    void handle_tool_action(const ActionConnection &conn);

    void handle_search();
    void search_go(int dir);
    std::list<Core::SearchResult> search_results;
    unsigned int search_result_current = 0;
    void update_search_markers();
    void update_search_types_label();
    void set_search_mode(bool enabled, bool focus = true);
    std::map<ObjectType, Gtk::CheckButton *> search_check_buttons;

    class LogWindow *log_window = nullptr;
    std::set<SelectableRef> selection_for_drag_move;
    Coordf cursor_pos_drag_begin;
    Coordi cursor_pos_grid_drag_begin;

    std::map<std::pair<ActionID, ToolID>, bool> action_sensitivity;
    type_signal_action_sensitive s_signal_action_sensitive;
};
} // namespace horizon
