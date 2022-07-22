#include "includes.h"

void Button::draw() {
	Rect  area{ m_parent->GetElementsRect() };
	Point p{ area.x + m_pos.x, area.y + m_pos.y };


	// draw border.
	render::rect(p.x + BUTTON_X_OFFSET + 1, p.y, m_w - BUTTON_X_OFFSET - 1, BUTTON_BOX_HEIGHT, { 40, 40, 40, m_parent->m_alpha });

	// button name
	render::menu.string(p.x + (m_w / 2),
		p.y + 2, { 205, 205, 205, m_parent->m_alpha }, m_label, render::ALIGN_CENTER);
}

void Button::think() {}

void Button::click() {
	Rect  area{ m_parent->GetElementsRect() };
	Point p{ area.x + m_pos.x, area.y + m_pos.y };

	// area where user has to click.
	Rect btn = { p.x + BUTTON_X_OFFSET, p.y, m_w - BUTTON_X_OFFSET, BUTTON_BOX_HEIGHT };

	if (g_input.IsCursorInRect(btn) && m_callback)
		m_callback();
}