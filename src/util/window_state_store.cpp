#include "window_state_store.hpp"
#include "util.hpp"
#include "sqlite.hpp"
#include "automatic_prefs.hpp"
#include <glibmm/miscutils.h>
#include <gtkmm.h>

namespace horizon {

WindowStateStore::WindowStateStore(Gtk::Window *w, const std::string &wn)
    : db(AutomaticPreferences::get().db), window_name(wn), win(w)
{
    {
        SQLite::Query q(db,
                        "SELECT width, height, x, y, maximized FROM "
                        "window_state WHERE window=?");
        q.bind(1, window_name);
        if (q.step()) {
            window_state.width = q.get<int>(0);
            window_state.height = q.get<int>(1);
            window_state.x = q.get<int>(2);
            window_state.y = q.get<int>(3);
            window_state.maximized = q.get<int>(4);
            apply(window_state);
            default_set = true;
        }
    }
    win->signal_size_allocate().connect([this](Gtk::Allocation &alloc) {
        if (win->is_maximized()) {
            window_state.maximized = true;
        }
        else {
            window_state.maximized = false;
            win->get_size(window_state.width, window_state.height);
            win->get_position(window_state.x, window_state.y);
        }
    });
    win->signal_delete_event().connect([this](GdkEventAny *ev) {
        save(window_name, window_state);
        return false;
    });
}

bool WindowStateStore::get_default_set() const
{
    return default_set;
}

void WindowStateStore::apply(const WindowState &ws)
{
    win->set_default_size(ws.width, ws.height);
    win->move(ws.x, ws.y);
    if (ws.maximized)
        win->maximize();
}

void WindowStateStore::save(const std::string &w, const WindowState &ws)
{
    SQLite::Query q(db,
                    "INSERT OR REPLACE INTO window_state (window, width, "
                    "height, x, y, maximized) VALUES (?, ?, ?, ?, ?, ?)");
    q.bind(1, w);
    q.bind(2, ws.width);
    q.bind(3, ws.height);
    q.bind(4, ws.x);
    q.bind(5, ws.y);
    q.bind(6, ws.maximized);
    q.step();
}
} // namespace horizon
