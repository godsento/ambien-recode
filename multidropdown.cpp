#include "includes.h"

void MultiDropdown::arrow1(Point p) {
	render::rect_filled(p.x + m_w - 11, p.y + m_offset + 9 + 2, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(p.x + m_w - 10, p.y + m_offset + 9 + 2, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(p.x + m_w - 9, p.y + m_offset + 9 + 2, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(p.x + m_w - 8, p.y + m_offset + 9 + 2, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(p.x + m_w - 7, p.y + m_offset + 9 + 2, 1, 1, { 152, 152, 152, m_parent->m_alpha });

	render::rect_filled(p.x + m_w - 10, p.y + m_offset + 9 + 1, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(p.x + m_w - 9, p.y + m_offset + 9 + 1, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(p.x + m_w - 8, p.y + m_offset + 9 + 1, 1, 1, { 152, 152, 152, m_parent->m_alpha });

	render::rect_filled(p.x + m_w - 9, p.y + m_offset + 9, 1, 1, { 152, 152, 152, m_parent->m_alpha });
}

void MultiDropdown::arrow2(Point l) {
	render::rect_filled(l.x + m_w - 11, l.y + m_offset + 9, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(l.x + m_w - 10, l.y + m_offset + 9, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(l.x + m_w - 9, l.y + m_offset + 9, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(l.x + m_w - 8, l.y + m_offset + 9, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(l.x + m_w - 7, l.y + m_offset + 9, 1, 1, { 152, 152, 152, m_parent->m_alpha });

	render::rect_filled(l.x + m_w - 10, l.y + m_offset + 9 + 1, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(l.x + m_w - 9, l.y + m_offset + 9 + 1, 1, 1, { 152, 152, 152, m_parent->m_alpha });
	render::rect_filled(l.x + m_w - 8, l.y + m_offset + 9 + 1, 1, 1, { 152, 152, 152, m_parent->m_alpha });

	render::rect_filled(l.x + m_w - 9, l.y + m_offset + 9 + 2, 1, 1, { 152, 152, 152, m_parent->m_alpha });
}

void MultiDropdown::draw() {
	Rect	area{ m_parent->GetElementsRect() };
	Point	p{ area.x + m_pos.x, area.y + m_pos.y };

	// get gui color.
	Color color = g_gui.m_color;
	color.a() = m_parent->m_alpha;

	// draw label.
	if (m_use_label)
		render::menu.string(p.x, p.y - 2, { 205, 205, 205, m_parent->m_alpha }, m_label);


	// draw border.
	render::rect(p.x + DROPDOWN_X_OFFSET, p.y + m_offset, m_w - DROPDOWN_X_OFFSET, DROPDOWN_BOX_HEIGHT, { 40, 40, 40, m_parent->m_alpha });

	// draw inside.
	render::rect_filled(p.x + DROPDOWN_X_OFFSET + 1, p.y + m_offset + 1, m_w - DROPDOWN_X_OFFSET - 2, DROPDOWN_BOX_HEIGHT - 2, m_open ? Color(12, 12, 12, m_parent->m_alpha) : Color(18, 18, 18, m_parent->m_alpha));


	// does this dropdown have any items?
	if (!m_items.empty()) {

		// is this dropdown open?
		if (m_open) {

			// draw items outline.
			render::rect(p.x + DROPDOWN_X_OFFSET, p.y + m_offset + DROPDOWN_BOX_HEIGHT + DROPDOWN_SEPARATOR, m_w - DROPDOWN_X_OFFSET, m_anim_height + 1, { 40, 40, 40, m_parent->m_alpha });

			// draw items inside.
			render::rect_filled(p.x + DROPDOWN_X_OFFSET + 1, p.y + m_offset + DROPDOWN_BOX_HEIGHT + DROPDOWN_SEPARATOR + 1, m_w - DROPDOWN_X_OFFSET - 2, m_anim_height - 1, { 22, 22, 22, m_parent->m_alpha });

			// iterate items.
			for (size_t i{}; i < m_items.size(); ++i) {

				// get offset to current item.
				int item_offset = (i * DROPDOWN_ITEM_HEIGHT);


				// check.
				bool active = std::find(m_active_items.begin(), m_active_items.end(), i) != m_active_items.end();

				m_anim_per_items[i] = (active) ? 1.f : 0.f;

				if (m_anim_per_items[i] != m_anim_per_items_last_frame[i]) {

					if (m_anim_per_items[i] > m_anim_per_items_last_frame[i])
						m_anim_per_items_last_frame[i] += g_csgo.m_globals->m_frametime * 4;
					else if (m_anim_per_items[i] < m_anim_per_items_last_frame[i])
						m_anim_per_items_last_frame[i] -= g_csgo.m_globals->m_frametime * 4;
				}

				// yet again, it won't use list init inside the ternary conditional.
				render::menu.string(p.x + 8 + std::round(8 * m_anim_per_items_last_frame[i]), p.y + m_offset + DROPDOWN_BOX_HEIGHT + DROPDOWN_ITEM_Y_OFFSET + item_offset,
					active ? color : Color{ 152, 152, 152, m_parent->m_alpha },
					m_items[i]);
			}
		}

		// no items.
		if (!m_active_items.size()) {
			render::menu.string(p.x + 8, p.y + m_offset + 2, { 152, 152, 152, m_parent->m_alpha }, XOR("none"));

		}
		// one item.
		else if (m_active_items.size() == 1) {
			render::menu.string(p.x + 8, p.y + m_offset + 2, { 152, 152, 152, m_parent->m_alpha }, m_items[m_active_items.front()]);

		}

		// more then one item.
		else {
			std::stringstream tmp, ss;
			auto			  items = GetActiveItems();

			for (size_t i{}; i < items.size(); ++i) {
				std::string item = items[i];

				// first item should not have a comma infront.
				if (i >= 1)
					tmp << XOR(", ");

				tmp << item;

				// does it still fit in?
				if (render::menu.size(tmp.str() + XOR("...")).m_width >= (m_w - DROPDOWN_X_OFFSET - DROPDOWN_ITEM_X_OFFSET - 10)) {
					ss << XOR("...");
					break;
				}

				if (i >= 1)
					ss << XOR(", ");

				ss << item;
			}

			render::menu.string(p.x + DROPDOWN_X_OFFSET + 5, p.y + m_offset + 2, { 152, 152, 152, m_parent->m_alpha }, ss.str());
		}
	}
}

void MultiDropdown::think() {
	// fucker can be opened.
	if (!m_items.empty()) {
		int total_size = DROPDOWN_ITEM_HEIGHT * m_items.size() + 3;

		// we need to travel 'total_size' in 300 ms.
		float frequency = total_size / 0.3f;

		// the increment / decrement per frame.
		float step = frequency * .04;

		// if open		-> increment
		// if closed	-> decrement
		m_open ? m_anim_height = total_size : m_anim_height = 0;

		// clamp the size.
		math::clamp< float >(m_anim_height, 0, total_size);

		// open
		if (m_open) {
			// clicky height.
			m_h = m_offset + DROPDOWN_BOX_HEIGHT + DROPDOWN_SEPARATOR + total_size;

			// another element is in focus.
			if (m_parent->m_active_element != this)
				m_open = false;
		}

		// closed.
		else
			m_h = m_offset + DROPDOWN_BOX_HEIGHT;
	}

	// no items, no open.
	else
		m_h = m_offset + DROPDOWN_BOX_HEIGHT;
}

void MultiDropdown::click() {
	Rect  area{ m_parent->GetElementsRect() };
	Point p{ area.x + m_pos.x, area.y + m_pos.y };

	// bar.
	Rect bar{ p.x + DROPDOWN_X_OFFSET, p.y + m_offset, m_w - DROPDOWN_X_OFFSET, DROPDOWN_BOX_HEIGHT };

	// toggle if bar pressed.
	if (g_input.IsCursorInRect(bar))
		m_open = !m_open;

	// check item clicks.
	if (m_open) {
		if (!g_input.IsCursorInRect(bar)) {
			// iterate items.
			for (size_t i{}; i < m_items.size(); ++i) {
				Rect item{ p.x + DROPDOWN_X_OFFSET, p.y + m_offset + DROPDOWN_BOX_HEIGHT + DROPDOWN_SEPARATOR + (i * DROPDOWN_ITEM_HEIGHT), m_w - DROPDOWN_X_OFFSET, DROPDOWN_ITEM_HEIGHT };

				// click was in context of current item.
				if (g_input.IsCursorInRect(item)) {
					bool active = std::find(m_active_items.begin(), m_active_items.end(), i) != m_active_items.end();

					if (active) {
						m_active_items.erase(std::remove(m_active_items.begin(), m_active_items.end(), i), m_active_items.end());

						if (m_callback)
							m_callback();
					}


					else {
						m_active_items.push_back(i);

						if (m_callback)
							m_callback();
					}
				}
			}
		}
	}
}