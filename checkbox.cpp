#include "includes.h"

void Checkbox::draw() {
	Rect area{ m_parent->GetElementsRect() };
	Point p{ area.x + m_pos.x, area.y + m_pos.y };

	// get gui color.
	Color color = g_gui.m_color;
	color.a() = m_parent->m_alpha;

	Color color1 = Color(18, 18, 18);
	Color color2 = Color(18, 18, 18);

	color1.a() = 50 * m_parent->m_fast_anim_step;
	color2.a() = 255 * m_parent->m_fast_anim_step;


	// render black outline on checkbox.
	render::rect(p.x, p.y, CHECKBOX_SIZE, CHECKBOX_SIZE, { 40, 40, 40, m_parent->m_alpha });

	// render checkbox title.
	if (m_use_label)
		render::menu.string(p.x + LABEL_OFFSET, p.y - 2, { 205, 205, 205, m_parent->m_alpha }, m_label);

	// render checked.
	if (m_checked) {
		Color wtf = color;
		wtf.r() = std::clamp(wtf.r() * .8, (double)0, (double)255);
		wtf.g() = std::clamp(wtf.g() * .8, (double)0, (double)255);
		wtf.b() = std::clamp(wtf.b() * .8, (double)0, (double)255);
		render::rect_filled(p.x + 1, p.y + 1, CHECKBOX_SIZE - 2, CHECKBOX_SIZE - 2, wtf);
		render::rect(p.x + 1, p.y + 1, CHECKBOX_SIZE - 2, CHECKBOX_SIZE - 2, color);
	}

	//render::rect( el.x + m_pos.x, el.y + m_pos.y, m_w, m_pos.h, { 255, 0, 0 } );
}

void Checkbox::think() {
	// set the click area to the length of the string, so we can also press the string to toggle.
	if (m_use_label)
		m_w = LABEL_OFFSET + render::menu_shade.size(m_label).m_width;
}

void Checkbox::click() {
	// toggle.
	m_checked = !m_checked;

	if (m_callback)
		m_callback();
}