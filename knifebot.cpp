#include "includes.h"

float constexpr max_dist = 128.f;
void Aimbot::knife() {
	struct KnifeTarget_t { bool stab; ang_t angle; LagRecord* record; };
	KnifeTarget_t target{};

	// we have no targets.
	if (m_targets.empty())
		return;



	// iterate all targets.
	for (const auto& t : m_targets) {
		// this target has no records
		// should never happen since it wouldnt be a target then.
		if (t->m_records.empty())
			continue;

		const vec3_t backup_origin = t->m_player->m_vecOrigin();
		const vec3_t  backup_abs_origin = t->m_player->GetAbsOrigin();
		const ang_t backup_abs_angles = t->m_player->GetAbsAngles();
		const vec3_t  backup_obb_mins = t->m_player->m_vecMins();
		const vec3_t  backup_obb_maxs = t->m_player->m_vecMaxs();
		const auto backup_cache = t->m_player->m_BoneCache2();

		auto restore = [&]() -> void {
			t->m_player->m_vecOrigin() = backup_origin;
			t->m_player->SetAbsOrigin(backup_abs_origin);
			t->m_player->SetAbsAngles(backup_abs_angles);
			t->m_player->m_vecMins() = backup_obb_mins;
			t->m_player->m_vecMaxs() = backup_obb_maxs;
			t->m_player->m_BoneCache2() = backup_cache;
		};



		// see if target broke lagcomp.
		if (g_menu.main.aimbot.lagfix.get() && g_lagcomp.StartPrediction(t)) {
			LagRecord* front = t->m_records.front().get();


			if (front->m_pred_origin.dist_to(g_cl.m_shoot_pos) > max_dist)
				continue;

			t->m_player->m_vecOrigin() = front->m_pred_origin;
			t->m_player->SetAbsOrigin(front->m_pred_origin);
			t->m_player->SetAbsAngles(front->m_abs_ang);
			t->m_player->m_vecMins() = front->m_mins;
			t->m_player->m_vecMaxs() = front->m_maxs;
			t->m_player->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(front->m_bones);

			// trace with front.
			for (const auto& a : m_knife_ang) {

				// check if we can knife.
				if (!CanKnife(front, a, target.stab)) {
					restore();
					continue;
				}

				// set target data.
				target.angle = a;
				target.record = front;
				restore();
				break;
			}
		}

		// we can history aim.
		else {

			LagRecord* best = g_resolver.FindIdealRecord(t);
			if (!best)
				continue;


			if (best->m_pred_origin.dist_to(g_cl.m_shoot_pos) <= max_dist) {

				t->m_player->m_vecOrigin() = best->m_pred_origin;
				t->m_player->SetAbsOrigin(best->m_pred_origin);
				t->m_player->SetAbsAngles(best->m_abs_ang);
				t->m_player->m_vecMins() = best->m_mins;
				t->m_player->m_vecMaxs() = best->m_maxs;
				t->m_player->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(best->m_bones);

				// trace with best.
				for (const auto& a : m_knife_ang) {

					// check if we can knife.
					if (!CanKnife(best, a, target.stab)) {
						restore();
						continue;
					}

					// set target data.
					target.angle = a;
					target.record = best;
					restore();
					break;
				}
			}

			LagRecord* last = t->m_records.back().get();
			if (last && last != best) {

				if (last->m_pred_origin.dist_to(g_cl.m_shoot_pos) <= max_dist) {

					t->m_player->m_vecOrigin() = last->m_pred_origin;
					t->m_player->SetAbsOrigin(last->m_pred_origin);
					t->m_player->SetAbsAngles(last->m_abs_ang);
					t->m_player->m_vecMins() = last->m_mins;
					t->m_player->m_vecMaxs() = last->m_maxs;
					t->m_player->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(last->m_bones);


					// trace with last.
					for (const auto& a : m_knife_ang) {

						// check if we can knife.
						if (!CanKnife(last, a, target.stab)) {
							restore();
							continue;
						}

						// set target data.
						target.angle = a;
						target.record = last;
						restore();
						break;
					}
				}
			}
		}

		// target player has been found already.
		if (target.record)
			break;
	}

	// we found a target.
	// set out data and choke.
	if (target.record) {
		// set target tick.

		if (target.record && !target.record)
			g_cl.m_cmd->m_tick = game::TIME_TO_TICKS(target.record->m_pred_time + g_cl.m_lerp);

		// set view angles.
		g_cl.m_cmd->m_view_angles = target.angle;

		// set attack1 or attack2.
		g_cl.m_cmd->m_buttons |= (target.stab ? button_flags_t::IN_ATTACK2 : button_flags_t::IN_ATTACK);

		// choke. 
		*g_cl.m_packet = false;
	}
}

bool Aimbot::CanKnife(LagRecord* record, ang_t angle, bool& stab) {
	// convert target angle to direction.
	vec3_t forward;
	math::AngleVectors(angle, &forward);

	// see if we can hit the player with full range
	// this means no stab.
	CGameTrace trace;
	KnifeTrace(forward, false, &trace);

	// we hit smthing else than we were looking for.
	if (!trace.m_entity || trace.m_entity->as< Player* >()->index() != record->m_player->index())
		return false;

	bool armor = record->m_player->m_ArmorValue() > 0;
	bool first = g_cl.m_weapon->m_flNextPrimaryAttack() + 0.4f < g_csgo.m_globals->m_curtime;
	bool back = KnifeIsBehind(record);

	int stab_dmg = m_knife_dmg.stab[armor][back];
	int slash_dmg = m_knife_dmg.swing[first][armor][back];
	int swing_dmg = m_knife_dmg.swing[false][armor][back];

	// smart knifebot.
	int health = record->m_player->m_iHealth();
	if (health <= slash_dmg || record->m_pred_origin.dist_to(g_cl.m_shoot_pos) > 33.f)
		stab = false;

	else if (health <= stab_dmg)
		stab = true;

	else if (health > (slash_dmg + swing_dmg + stab_dmg))
		stab = true;

	else
		stab = false;

	// damage wise a stab would be sufficient here.
	if (stab && !KnifeTrace(forward, true, &trace))
		return false;

	return true;
}

bool Aimbot::KnifeTrace(vec3_t dir, bool stab, CGameTrace* trace) {
	float range = stab ? 32.f : 48.f;

	vec3_t start = g_cl.m_shoot_pos;
	vec3_t end = start + (dir * range);

	CTraceFilterSimple filter;
	filter.SetPassEntity(g_cl.m_local);
	g_csgo.m_engine_trace->TraceRay(Ray(start, end), MASK_SOLID, &filter, trace);

	// if the above failed try a hull trace.
	if (trace->m_fraction >= 1.f) {
		g_csgo.m_engine_trace->TraceRay(Ray(start, end, { -16.f, -16.f, -18.f }, { 16.f, 16.f, 18.f }), MASK_SOLID, &filter, trace);
		return trace->m_fraction < 1.f;
	}

	return true;
}

bool Aimbot::KnifeIsBehind(LagRecord* record) {
	vec3_t delta{ record->m_pred_origin - g_cl.m_shoot_pos };
	delta.z = 0.f;
	delta.normalize();

	vec3_t target;
	math::AngleVectors(record->m_abs_ang, &target);
	target.z = 0.f;

	return delta.dot(target) > 0.475f;
}