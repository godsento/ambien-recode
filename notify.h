#pragma once

// modelled after the original valve 'developer 1' debug console
// https://github.com/LestaD/SourceEngine2007/blob/master/se2007/engine/console.cpp

class NotifyText {
public:
	std::string m_text;
	Color		m_color;
	float		m_time;

public:
	__forceinline NotifyText(const std::string& text, Color color, float time) : m_text{ text }, m_color{ color }, m_time{ time } {}
};

class Notify {
private:
	std::vector< std::shared_ptr< NotifyText > > m_notify_text;

public:
	__forceinline Notify() : m_notify_text{} {}

	__forceinline void add(const std::string& text, Color color = colors::white, float time = 6.f, bool console = true) {
		// modelled after 'CConPanel::AddToNotify'
		m_notify_text.push_back(std::make_shared< NotifyText >(text, color, time));

		if (console)
			g_cl.print(text);
	}

	// modelled after 'CConPanel::DrawNotify' and 'CConPanel::ShouldDraw'
	void think(Color nigga) {
		int		x{ 8 }, y{ 5 }, size{ render::esp.m_size.m_height + 1 };
		Color	color;
		float	left;

		// update lifetimes.
		for (size_t i{}; i < m_notify_text.size(); ++i) {
			auto notify = m_notify_text[i];

			notify->m_time -= g_csgo.m_globals->m_frametime;

			if (notify->m_time <= 0.f) {
				m_notify_text.erase(m_notify_text.begin() + i);
				continue;
			}
		}

		// we have nothing to draw.
		if (m_notify_text.empty())
			return;

		// iterate entries.
		for (size_t i{}; i < m_notify_text.size(); ++i) {
			auto notify = m_notify_text[i];

			left = notify->m_time;
			color = notify->m_color;
			float value_thing = 1;


			color.a() = 255;
			// start
			if (left <= 6.f && left >= 5.75f) {
				float f = 0.f;
				f = abs(left - 6.0) / 0.25f;
				value_thing = f * 1;
				color.a() = 255 * value_thing;

			}

			// ghetto
			float shit_x = x - 500 + 500 * value_thing;



			// end
			if (left <= .25f) {
				float f = left;
				math::clamp(f, 0.f, .25f);

				f /= .25f;


				value_thing = 1.f - f;

				color.a() = (int)(f * 255.f);

				if (i == 0 && f <= 0.125f)
					y -= size * (1.f - f / 0.125f);

				shit_x = x - 500 * value_thing;
			}




			nigga.a() = color.a();

			std::string prefix = ("[amb]");
			render::FontSize_t fontsize = render::esp.size(prefix);

			render::esp.string(shit_x, y, nigga, prefix);
			render::esp.string(shit_x + fontsize.m_width + 5, y, color, notify->m_text);

			y += size;
		}
	}
};

extern Notify g_notify;