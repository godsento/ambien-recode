#include "includes.h"

void Form::draw() {
	m_alpha = 0xff;

	float alpha = m_open ? 1.f : 0.f;

	if (alpha != m_fast_anim_step)
		m_fast_anim_step = math::lerp(g_csgo.m_globals->m_frametime * 8, m_fast_anim_step, alpha);

	math::clamp(m_fast_anim_step, 0.f, 1.f);


	m_alpha = m_fast_anim_step * 255.f;

	if (!m_alpha)
		return;

	// get gui color.
	Color color = g_gui.m_color;
	color.a() = m_fast_anim_step * 255.f;
	Color color2 = g_gui.m_color;
	Color color3 = g_gui.m_color;
	Color color4 = g_gui.m_color;
	Color color5 = g_gui.m_color;

	color2.a() = 50 * m_fast_anim_step;
	color3.a() = 120 * m_fast_anim_step;
	color4.a() = 255 * m_fast_anim_step;

	// background.
	int top_bar_size = 22;
	render::rect_filled(m_x, m_y, m_width, m_height, { 18,18,18, m_alpha });
	render::rect_filled(m_x, m_y, m_width, top_bar_size, { 16,16,16, m_alpha });
	render::gradient(m_x, m_y + (top_bar_size / 2) + 8, m_width / 2, 1, { 0,0,0,0 }, color, true);
	render::gradient(m_x + m_width / 2, m_y + (top_bar_size / 2) + 8, m_width / 2, 1, color, { 0,0,0,0 }, true);
	render::FontSize_t text_size = render::bold2.size("ambien");

	// watch the fps drop
	for (int i{ 1 }; i <= 3; ++i) {

		color5.a() = (17.f / i) * m_fast_anim_step;

		render::bold2.string(m_x + m_width / 2 + i, m_y + (top_bar_size / 2) - (text_size.m_height / 2), color5, "ambien", render::ALIGN_CENTER);
		render::bold2.string(m_x + m_width / 2 - i, m_y + (top_bar_size / 2) - (text_size.m_height / 2), color5, "ambien", render::ALIGN_CENTER);

		render::bold2.string(m_x + m_width / 2, m_y + (top_bar_size / 2) - (text_size.m_height / 2) + i, color5, "ambien", render::ALIGN_CENTER);
		render::bold2.string(m_x + m_width / 2, m_y + (top_bar_size / 2) - (text_size.m_height / 2) - i, color5, "ambien", render::ALIGN_CENTER);

		render::bold2.string(m_x + m_width / 2 - i, m_y + (top_bar_size / 2) - (text_size.m_height / 2) - i, color5, "ambien", render::ALIGN_CENTER);
		render::bold2.string(m_x + m_width / 2 + i, m_y + (top_bar_size / 2) - (text_size.m_height / 2) + i, color5, "ambien", render::ALIGN_CENTER);


		render::bold2.string(m_x + m_width / 2 - i, m_y + (top_bar_size / 2) - (text_size.m_height / 2) + i, color5, "ambien", render::ALIGN_CENTER);
		render::bold2.string(m_x + m_width / 2 + i, m_y + (top_bar_size / 2) - (text_size.m_height / 2) - i, color5, "ambien", render::ALIGN_CENTER);
	}

	render::bold2.string(m_x + m_width / 2, m_y + (top_bar_size / 2) - (text_size.m_height / 2), color4, "ambien", render::ALIGN_CENTER);

	//	render::bold.string(m_x + m_width / 2 + 1, m_y + (top_bar_size / 2) - (text_size.m_height / 2) - 1, color5, "ambien", render::ALIGN_CENTER);
	//	render::bold.string(m_x + m_width / 2 - 1, m_y + (top_bar_size / 2) - (text_size.m_height / 2) + 1, color5, "ambien", render::ALIGN_CENTER);

	render::rect(m_x + 1, m_y + 1, m_width - 2, m_height - 2, { 26,26,26, m_alpha });
	// border.
	render::rect(m_x, m_y, m_width, m_height, { 5, 5, 5, m_alpha });
	render::gradient(m_x, m_y + m_height - 40, m_width / 2, 1, { 0,0,0,0 }, color, true);
	render::gradient(m_x + m_width / 2, m_y + m_height - 40, m_width / 2, 1, color, { 0,0,0,0 }, true);
	Rect tabs_area = GetTabsRect();
	render::menu_shade.string(tabs_area.x + tabs_area.w - 8, tabs_area.y + 9, { 255,255,255, (int)std::floor(50 * m_fast_anim_step) }, "debug | v4", render::ALIGN_RIGHT);
	// draw tabs if we have any.
	if (!m_tabs.empty()) {
		// tabs background and border.
		size_t font_w = 8;
		for (size_t i{}; i < m_tabs.size(); ++i) {
			const auto& t = m_tabs[i];
			if (t == m_active_tab) {
				render::menu_shade.string(tabs_area.x + font_w, tabs_area.y + 5, color, t->m_title);
				render::menu_shade.string(tabs_area.x + font_w, tabs_area.y + 15, { 255,255,255,  (int)std::floor(50 * m_fast_anim_step) }, t->m_desc);
			}
			else
				render::menu_shade.string(tabs_area.x + font_w, tabs_area.y + 9, color4, t->m_title);
			int descwidth = render::menu_shade.size(t->m_desc).m_width;
			int titlewidth = render::menu_shade.size(t->m_title).m_width;

			font_w += ((descwidth > titlewidth && t == m_active_tab) ? descwidth : titlewidth) + 10;
		}

		// this tab has elements.
		if (!m_active_tab->m_elements.empty()) {
			// elements background and border.
			Rect el = GetElementsRect();

			render::rect_filled(el.x, el.y, el.w, el.h, { 18,18,18, m_alpha });
			render::rect(el.x, el.y, el.w, el.h, { 5,5,5, m_alpha });
			render::rect(el.x + 1, el.y + 1, el.w - 2, el.h - 2, { 26,26,26, m_alpha });

			// iterate elements to display.
			for (const auto& e : m_active_tab->m_elements) {

				// draw the active element last.
				if (!e || (m_active_element && e == m_active_element))
					continue;

				if (!e->m_show)
					continue;

				// this element we dont draw.
				if (!(e->m_flags & ElementFlags::DRAW))
					continue;

				e->draw();
			}

			// we still have to draw one last fucker.
			if (m_active_element && m_active_element->m_show)
				m_active_element->draw();
		}
	}
}