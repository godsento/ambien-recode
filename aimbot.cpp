#include "includes.h"
#include "shifting.h"

Aimbot g_aimbot{ };;

void AimPlayer::OnRoundStart( Player *player ) {
	m_player = player;
	m_walk_record = LagRecord{ };
	m_shots = 0;
	m_missed_shots = 0;

	// reset stand and body index.
	m_stand_index  = 0;
	m_stand_index2 = 0;
	m_body_index   = 0;

	m_records.clear( );
	m_hitboxes.clear( );
	m_prefer_body = false;

	// IMPORTANT: DO NOT CLEAR LAST HIT SHIT.
}

void AimPlayer::SetupHitboxes(LagRecord* record) {

	// reset hitboxes & prefer states.
	m_hitboxes.clear();
	m_prefer_body = false;

	if (g_cl.m_weapon_id == ZEUS) {
		// hitboxes for the zeus.
		m_hitboxes.push_back({ HITBOX_BODY });
		m_hitboxes.push_back({ HITBOX_PELVIS });
		m_hitboxes.push_back({ HITBOX_THORAX });
		m_hitboxes.push_back({ HITBOX_CHEST });
		return;
	}

	bool m_resolved = record->m_mode == g_resolver.RESOLVE_WALK || record->m_mode == g_resolver.RESOLVE_BODY;
	// prefer, always.
	if (g_menu.main.aimbot.baim1.get(0))
		m_prefer_body = true;

	// prefer, fake.
	if (g_menu.main.aimbot.baim1.get(1) && !m_resolved)
		m_prefer_body = true;

	// prefer, in air.
	if (g_menu.main.aimbot.baim1.get(2) && !(record->m_pred_flags & FL_ONGROUND))
		m_prefer_body = true;

	bool dt = g_aimbot.dt_aim || g_tickshift.m_charged_ticks;

	// prefer, in air.
	if (g_menu.main.aimbot.baim1.get(3) && dt)
		m_prefer_body = true;

	bool only{ false };

	// only, always.
	if (g_menu.main.aimbot.baim2.get(0))
		only = true;

	// only, health.
	if (g_menu.main.aimbot.baim2.get(1) && m_player->m_iHealth() <= (int)g_menu.main.aimbot.baim_hp.get())
		only = true;

	// only, fake.
	if (g_menu.main.aimbot.baim2.get(2) && !m_resolved)
		only = true;

	// only, in air.
	if (g_menu.main.aimbot.baim2.get(3) && !(record->m_pred_flags & FL_ONGROUND))
		only = true;

	// only, in air.
	if (g_menu.main.aimbot.baim2.get(4) && dt)
		only = true;

	// only, on key.
	if (g_input.GetKeyState(g_menu.main.aimbot.baim_key.get()))
		only = true;

	if (g_menu.main.aimbot.hitbox.get(0) && !only)
		m_hitboxes.push_back({ HITBOX_HEAD });

	// stomach.
	if (g_menu.main.aimbot.hitbox.get(2)) {
		m_hitboxes.push_back({ HITBOX_PELVIS });
		m_hitboxes.push_back({ HITBOX_BODY });
	}

	// chest.
	if (g_menu.main.aimbot.hitbox.get(1)) {
		m_hitboxes.push_back({ HITBOX_THORAX });
		m_hitboxes.push_back({ HITBOX_CHEST });
		m_hitboxes.push_back({ HITBOX_UPPER_CHEST });
	}

	// legs.
	if (g_menu.main.aimbot.hitbox.get(4)) {
		m_hitboxes.push_back({ HITBOX_L_THIGH });
		m_hitboxes.push_back({ HITBOX_R_THIGH });
		m_hitboxes.push_back({ HITBOX_L_CALF });
		m_hitboxes.push_back({ HITBOX_R_CALF });
		m_hitboxes.push_back({ HITBOX_L_FOOT });
		m_hitboxes.push_back({ HITBOX_R_FOOT });
	}

	// arms.
	if (g_menu.main.aimbot.hitbox.get(3)) {
		m_hitboxes.push_back({ HITBOX_L_UPPER_ARM });
		m_hitboxes.push_back({ HITBOX_R_UPPER_ARM });
	}
}

void Aimbot::init( ) {
	// clear old targets.
	m_targets.clear( );

	m_target = nullptr;
	m_aim = vec3_t{ };
	m_angle = ang_t{ };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;

	m_best_dist = std::numeric_limits< float >::max( );
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max( );
	m_best_height = std::numeric_limits< float >::max( );
}


void Aimbot::handle_targets() {

	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		if (!IsValidTarget(player))
			continue;

		AimPlayer* data = &m_players[i - 1];
		if (!data)
			continue;

		if (data->m_records.empty())
			continue;

		m_targets.emplace_back(data);
	}

	std::sort(m_targets.begin(), m_targets.end(), [&](const AimPlayer* a, const AimPlayer* b) {

		if (g_menu.main.aimbot.selection.get() == 0)
			return (a->m_records.front().get()->m_pred_origin.dist_to(g_cl.m_shoot_pos) < b->m_records.front().get()->m_pred_origin.dist_to(g_cl.m_shoot_pos));
		else if (g_menu.main.aimbot.selection.get() == 1)
			return (math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, a->m_player->WorldSpaceCenter()) < math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, b->m_player->WorldSpaceCenter()));
		else
			return (a->m_player->m_iHealth() < b->m_player->m_iHealth());

		return false;

		});
}


void Aimbot::StripAttack( ) {
	if ( g_cl.m_weapon_id == REVOLVER )
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK2;

	else
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK;
}

bool Aimbot::should_shoot() {

	bool tick_shift = g_cl.m_weapon->m_flNextPrimaryAttack() <= g_csgo.m_globals->m_curtime - game::TICKS_TO_TIME(14);
	bool tick_shift2 = g_cl.m_weapon->m_flNextPrimaryAttack() <= game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase() - 14);

	bool can_shoot = g_cl.m_weapon_fire; //|| (g_cl.m_weapon->m_flNextPrimaryAttack() <= g_csgo.m_globals->m_curtime - game::TICKS_TO_TIME(14) && g_tickshift.m_double_tap);
	bool auto_ = g_cl.m_weapon_id == G3SG1 || g_cl.m_weapon_id == SCAR20 || g_cl.m_weapon_info->m_is_full_auto;
	// no point in aimbotting if we cannot fire this tick.

	bool single = g_cl.m_weapon_id == SSG08 || g_cl.m_weapon_id == AWP;

	if (single && !tick_shift && !tick_shift2 && g_tickshift.m_double_tap)
		return false;

	bool between_shots = g_menu.main.aimbot.auto_stop_mode.get(1);

	if (!can_shoot && !auto_)
		return false;

	if (auto_ && !can_shoot && !g_tickshift.m_charged && !tick_shift && !between_shots)
		return false;

	return true;
}


void Aimbot::think( ) {
	// do all startup routines.
	init( );

	// sanity.
	if ( !g_cl.m_weapon )
		return;

	bool auto_ = g_cl.m_weapon_id == G3SG1 || g_cl.m_weapon_id == SCAR20 || g_cl.m_weapon_info->m_is_full_auto;

	bool should_strip = !g_cl.m_weapon_fire;

	if (should_strip && !auto_)
		StripAttack();

	// no grenades or bomb.
	if ( g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_C4 )
		return;

	// we have no aimbot enabled.
	if ( !g_menu.main.aimbot.enable.get( ) )
		return;

	// animation silent aim, prevent the ticks with the shot in it to become the tick that gets processed.
	// we can do this by always choking the tick before we are able to shoot.
	bool revolver = g_cl.m_weapon_id == REVOLVER && g_cl.m_revolver_cock != 0;
#ifndef _DEBUG

	// one tick before being able to shoot.
	if ( revolver && g_cl.m_revolver_cock > 0 && g_cl.m_revolver_cock == g_cl.m_revolver_query ) {
		*g_cl.m_packet = false;
		return;
	}

	// we have a normal weapon or a non cocking revolver
	// choke if its the processing tick.
	/*
	if ( g_cl.m_weapon_fire && !g_cl.m_lag && !revolver && !g_tickshift.m_double_tap) {
		*g_cl.m_packet = false;
		StripAttack( );
		return;
	}*/
#endif

	if (!should_shoot())
		return;

	handle_targets();

	// run knifebot.
	if ( g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS ) {
		knife( );
		return;
	}

	// scan available targets... if we even have any.
	find( );

	if (should_strip)
		StripAttack();

	// finally set data when shooting.
	apply( );
}

void Aimbot::find( ) {
	struct BestTarget_t { Player *player; vec3_t pos; float damage; LagRecord *record; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t{ };
	best.record = nullptr;

	if ( m_targets.empty( ) )
		return;

	// iterate all targets.
	for ( const auto &t : m_targets ) {
		if ( t->m_records.empty( ) )
			continue;

		// this player broke lagcomp.
		// his bones have been resetup by our lagcomp.
		// therfore now only the front record is valid.
		if ( g_menu.main.aimbot.lagfix.get( ) && g_lagcomp.StartPrediction( t ) ) {
			LagRecord *front = t->m_records.front( ).get( );

			t->SetupHitboxes( front );
			if ( t->m_hitboxes.empty( ) )
				continue;

			// rip something went wrong..
			if ( t->GetBestAimPosition( tmp_pos, tmp_damage, front ) ) {

				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = front;
				break;
			}
		}

		// player did not break lagcomp.
		// history aim is possible at this point.
		else {
			
			if (t->m_records.front().get()->m_broke_lc)
				continue;

			LagRecord *ideal = g_resolver.FindIdealRecord( t );

			if ( !ideal )
				ideal = t->m_records.front().get();

			if ( !ideal )
				continue;

			t->SetupHitboxes( ideal );
			if ( t->m_hitboxes.empty( ) )
				continue;

			bool hit_ideal = t->GetBestAimPosition(tmp_pos, tmp_damage, ideal);

			// try to select best record as target.
			if ( hit_ideal ) {
				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = ideal;
				break;
			}
			
			LagRecord *last = g_resolver.FindLastRecord( t );
			/*
			if (!last || last == ideal) {
				if (ideal != t->m_records.front().get())
					last = t->m_records.front().get();
			}*/

			if (!last || last == ideal)
				continue;

			t->SetupHitboxes( last );
			if ( t->m_hitboxes.empty( ) )
				continue;

			// rip something went wrong..
			if ( t->GetBestAimPosition( tmp_pos, tmp_damage, last ) ) {
				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = last;
				break;
			}
		}
	}


	// set autostop shit.
	m_stop = (g_cl.m_flags & FL_ONGROUND) && g_cl.m_ground && best.player && g_menu.main.aimbot.auto_stop.get();



	// verify our target and set needed data.
	if ( best.player && best.record ) {
		// calculate aim angle.
		math::VectorAngles( best.pos - g_cl.m_shoot_pos, m_angle );

		// set member vars.
		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_record = best.record;

		bool on = g_menu.main.aimbot.hitchance_amount.get( ) && g_menu.main.config.mode.get( ) == 0;
		bool hit = on && CheckHitchance( m_target, m_angle );

		// if we can scope.
		bool can_scope = !g_cl.m_local->m_bIsScoped( ) && ( g_cl.m_weapon_id == AUG || g_cl.m_weapon_id == SG553 || g_cl.m_weapon_type == WEAPONTYPE_SNIPER_RIFLE );

		float flMaxSpeed = g_cl.m_local->m_bIsScoped() > 0 ? g_cl.m_weapon_info->m_max_player_speed_alt : g_cl.m_weapon_info->m_max_player_speed;


		bool accurate_speed = (g_cl.m_unpredicted_vel.length() <= flMaxSpeed / 3) || (g_cl.m_local->m_vecVelocity().length() <= flMaxSpeed / 3);

		if (!accurate_speed && !hit && g_menu.main.aimbot.auto_stop_mode.get(2))
			return;

		if ( can_scope ) {
			// always.
			if ( g_menu.main.aimbot.zoom.get( ) == 1 ) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}

			// hitchance fail.
			else if ( g_menu.main.aimbot.zoom.get( ) == 2 && on && !hit ) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}
		}

		if ( hit || !on ) {
			// right click attack.
			if ( g_menu.main.config.mode.get( ) == 1 && g_cl.m_weapon_id == REVOLVER )
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;

			// left click attack.
			else
				g_cl.m_cmd->m_buttons |= IN_ATTACK;
		}
	}
}



bool Aimbot::CheckHitchance(Player* player, const ang_t& angle) {
	constexpr float HITCHANCE_MAX = 100.f;
	int   SEED_MAX = 256;

	if (g_cl.get_fps() <= 75)
		SEED_MAX = 128.f;

	Player* m_player = player;
	LagRecord* record = m_record;



	const vec3_t backup_origin = m_player->m_vecOrigin();
	const vec3_t backup_abs_origin = m_player->GetAbsOrigin();
	const ang_t backup_abs_angles = m_player->GetAbsAngles();
	const vec3_t backup_obb_mins = m_player->m_vecMins();
	const vec3_t backup_obb_maxs = m_player->m_vecMaxs();
	const auto backup_cache = m_player->m_BoneCache2();

	// quick function
	auto restore = [&]() -> void {
		m_player->m_vecOrigin() = backup_origin;
		m_player->SetAbsOrigin(backup_abs_origin);
		m_player->SetAbsAngles(backup_abs_angles);
		m_player->m_vecMins() = backup_obb_mins;
		m_player->m_vecMaxs() = backup_obb_maxs;
		m_player->m_BoneCache2() = backup_cache;
	};

	auto apply = [&]() -> void {
		m_player->m_vecOrigin() = record->m_pred_origin;
		m_player->SetAbsOrigin(record->m_pred_origin);
		m_player->SetAbsAngles(record->m_abs_ang);
		m_player->m_vecMins() = record->m_mins;
		m_player->m_vecMaxs() = record->m_maxs;
		m_player->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(record->m_bones);

	};

	float hc = g_menu.main.aimbot.hitchance_amount.get();

	if (dt_aim)
		hc = g_menu.main.aimbot.double_tap_hc.get();

	if (g_cl.m_weapon_id == ZEUS)
		hc = 75.f;

	goal_hc = hc;

	vec3_t     start{ g_cl.m_shoot_pos }, end, fwd, right, up, dir, wep_spread;
	float      inaccuracy, spread;
	CGameTrace tr;
	size_t     total_hits{ }, needed_hits{ (size_t)std::ceil((hc * SEED_MAX) / HITCHANCE_MAX)};

	// get needed directional vectors.
	math::AngleVectors(angle, &fwd, &right, &up);

	// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
	inaccuracy = g_cl.m_weapon->GetInaccuracy();
	spread = g_cl.m_weapon->GetSpread();
	float recoil_index = g_cl.m_weapon->m_flRecoilIndex();


	apply();


	// iterate all possible seeds.
	for (int i{ 0 }; i <= SEED_MAX; ++i) {
		float a = g_csgo.RandomFloat(0.f, 1.f);
		float b = g_csgo.RandomFloat(0.f, math::pi * 2.f);
		float c = g_csgo.RandomFloat(0.f, 1.f);
		float d = g_csgo.RandomFloat(0.f, math::pi * 2.f);


		if (g_cl.m_weapon_id == REVOLVER) {
			a = 1.f - a * a;
			a = 1.f - c * c;
		}
		else if (g_cl.m_weapon_id == NEGEV && recoil_index < 3.0f) {
			for (int i = 3; i > recoil_index; i--) {
				a *= a;
				c *= c;
			}

			a = 1.0f - a;
			c = 1.0f - c;
		}

		float inac = a * inaccuracy;
		float sir = c * spread;

		vec3_t sirVec((cos(b) * inac) + (cos(d) * sir), (sin(b) * inac) + (sin(d) * sir), 0), direction;

		direction.x = fwd.x + (sirVec.x * right.x) + (sirVec.y * up.x);
		direction.y = fwd.y + (sirVec.x * right.y) + (sirVec.y * up.y);
		direction.z = fwd.z + (sirVec.x * right.z) + (sirVec.y * up.z);
		direction.normalize();

		ang_t viewAnglesSpread;
		math::VectorAngles(direction, viewAnglesSpread, &up);
		viewAnglesSpread.normalize();

		vec3_t viewForward;
		math::AngleVectors(viewAnglesSpread, &viewForward);
		viewForward.normalized();

		end = start + (viewForward * g_cl.m_weapon_info->m_range);

		// setup ray and trace.
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT, player, &tr);



		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if (tr.m_entity == player && game::IsValidHitgroup(tr.m_hitgroup))
			++total_hits;

		// we made it.
		if (total_hits >= needed_hits) {
			restore();
			return true;
		}

		// we cant make it anymore.
		if ((SEED_MAX - i + total_hits) < needed_hits) {
			restore();
			return false;
		}
	}

	restore();
	return false;
}

bool AimPlayer::SetupHitboxPoints(LagRecord* record, BoneArray* bones, int index, std::vector< HitscanPoint_t >& points) {


	const model_t* model = m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(m_player->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(index);
	if (!bbox)
		return false;

	// get hitbox scales.
	float scale = g_menu.main.aimbot.scale.get() / 100.f;

	// big inair fix.
	if (!(record->m_pred_flags & FL_ONGROUND))
		scale = 0.78f;

	float bscale = g_menu.main.aimbot.body_scale.get() / 100.f;


	// compute raw center point.
	vec3_t center = (bbox->m_mins + bbox->m_maxs) / 2.f;

	// these indexes represent boxes.
	if (bbox->m_radius <= 0.f) {

		points.push_back(HitscanPoint_t{ index, center, true, true });

		// the feet hiboxes have a side, heel and the toe.
		if (index == HITBOX_R_FOOT || index == HITBOX_L_FOOT) {

			if (g_menu.main.aimbot.multipoint.get(4)) {
				// get point offset relative to center point
				// and factor in hitbox scale.
				float d2 = (bbox->m_mins.x - center.x) * scale;
				float d3 = (bbox->m_maxs.x - center.x) * scale;


				// heel.
				vec3_t heel{ center.x + d2, center.y, center.z };
				points.push_back(HitscanPoint_t{ index, heel, false, true });

				// toe.
				vec3_t toe{ center.x + d3, center.y, center.z };
				points.push_back(HitscanPoint_t{ index, toe, false, true });
			}
		}


	}

	// these hitboxes are capsules.
	else {

		// center.
		points.push_back(HitscanPoint_t{ index, center, true, false });


		// factor in the pointscale.
		float r = bbox->m_radius * scale;
		float br = bbox->m_radius * bscale;

		// head has 5 points.
		// center + back + left + right + topback
		if (index == HITBOX_HEAD) {
			if (g_menu.main.aimbot.multipoint.get(0)) {
				// rotation matrix 45 degrees.
				// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
				// std::cos( deg_to_rad( 45.f ) )
				constexpr float rotation = 0.70710678f;

				// top/back 45 deg.
				// this is the best spot to shoot at.
				vec3_t top{ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z };
				points.push_back(HitscanPoint_t{ index, top, false, false });

				// right.
				vec3_t right{ center.x, center.y, bbox->m_maxs.z + r };
				points.push_back(HitscanPoint_t{ index, right, false, false });

				// left.
				vec3_t left{ center.x, center.y, bbox->m_maxs.z - r };
				points.push_back(HitscanPoint_t{ index, left, false,false });

				// back.
				vec3_t back{ center.x, bbox->m_maxs.y - r, center.z };
				points.push_back(HitscanPoint_t{ index, back, false,false });
			}
		}
		// body has 4 points
		// center + back + left + right
		else if (index == HITBOX_PELVIS) {
			if (g_menu.main.aimbot.multipoint.get(2)) {
				vec3_t back{ center.x, bbox->m_maxs.y - br, center.z };
				vec3_t right{ center.x, center.y, bbox->m_maxs.z + br };
				vec3_t left{ center.x, center.y, bbox->m_mins.z - br };

				points.push_back(HitscanPoint_t{ index, right, false, false });
				points.push_back(HitscanPoint_t{ index, left, false, false });
				points.push_back(HitscanPoint_t{ index, back, false, false });
			}
		}
		// exact same as pelvis but inverted (god knows what theyre doing at valve)
		else if (index == HITBOX_BODY) {
			if (g_menu.main.aimbot.multipoint.get(2)) {

				vec3_t back{ center.x, bbox->m_maxs.y - br, center.z };
				vec3_t right{ center.x, center.y, bbox->m_maxs.z - br };
				vec3_t left{ center.x, center.y, bbox->m_mins.z + br };

				points.push_back(HitscanPoint_t{ index, right, false, false });
				points.push_back(HitscanPoint_t{ index, left, false, false });
				points.push_back(HitscanPoint_t{ index, back, false, false });
			}
		}

		// other stomach/chest hitboxes have 2 points.
		else if (index == HITBOX_THORAX || index == HITBOX_CHEST) {


			// add extra point on back.
			if (g_menu.main.aimbot.multipoint.get(1)) {
				vec3_t back{ center.x, bbox->m_maxs.y - br, center.z };
				points.push_back(HitscanPoint_t{ index, back, false, false });
			}
		}

		else if (index == HITBOX_R_CALF || index == HITBOX_L_CALF) {

			// half bottom.
			if (g_menu.main.aimbot.multipoint.get(4)) {
				vec3_t calf = { bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z };
				points.push_back(HitscanPoint_t{ index, calf, false, false });
			}
		}

		// arms get only one point.
		else if (index == HITBOX_R_UPPER_ARM || index == HITBOX_L_UPPER_ARM) {

			// elbow.
			if (g_menu.main.aimbot.multipoint.get(3)) {
				vec3_t arm = { bbox->m_maxs.x + (bbox->m_radius * 0.75f), center.y, center.z };
				points.push_back(HitscanPoint_t{ index, arm, false, false });
			}
		}
	}

	return true;
}

bool AimPlayer::GetBestAimPosition( vec3_t &aim, float &damage, LagRecord *record ) {
	bool                  pen { true };
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< HitscanPoint_t > points;


	const model_t* model = m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(m_player->m_nHitboxSet());
	if (!set)
		return false;

	// get player hp.
	int hp = std::min( 100, m_player->m_iHealth( ) );

	if ( g_cl.m_weapon_id == ZEUS ) {
		dmg = pendmg = hp;
	}

	else {
		dmg = g_menu.main.aimbot.minimal_damage.get( );
		pendmg = g_menu.main.aimbot.penetrate_minimal_damage.get();

		bool shdmg = (g_aimbot.damage_override && g_menu.main.aimbot.dmg_override_mode.get() == 1) || (g_menu.main.aimbot.dmg_override_mode.get() == 0 && g_input.GetKeyState(g_menu.main.aimbot.dmg_override.get()));


		if (shdmg)
			dmg = pendmg = g_menu.main.aimbot.dmg_override_amount.get();

		if (dmg >= 100)
			dmg = hp + (dmg - 100);


		if (pendmg >= 100)
			pendmg = hp + (pendmg - 100);

		if (g_menu.main.aimbot.scale_dmg.get() && hp < 20.f)
			dmg = pendmg = hp;
	}


	const vec3_t backup_origin = m_player->m_vecOrigin();
	const vec3_t backup_abs_origin = m_player->GetAbsOrigin();
	const ang_t backup_abs_angles = m_player->GetAbsAngles();
	const vec3_t backup_obb_mins = m_player->m_vecMins();
	const vec3_t backup_obb_maxs = m_player->m_vecMaxs();
	const auto backup_cache = m_player->m_BoneCache2();

	// quick function
	auto restore = [&]() -> void {
		m_player->m_vecOrigin() = backup_origin;
		m_player->SetAbsOrigin(backup_abs_origin);
		m_player->SetAbsAngles(backup_abs_angles);
		m_player->m_vecMins() = backup_obb_mins;
		m_player->m_vecMaxs() = backup_obb_maxs;
		m_player->m_BoneCache2() = backup_cache;
	};

	auto apply = [&]() -> void {
		m_player->m_vecOrigin() = record->m_pred_origin;
		m_player->SetAbsOrigin(record->m_pred_origin);
		m_player->SetAbsAngles(record->m_abs_ang);
		m_player->m_vecMins() = record->m_mins;
		m_player->m_vecMaxs() = record->m_maxs;
		m_player->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(record->m_bones);

	};

	// sup does that(?)
	// like it caches even before multipoints so i guess we have to do it aswell?
	apply();

	for (const auto& it : m_hitboxes) {

		// setup points on hitbox.
		SetupHitboxPoints(record, record->m_bones, it.m_index, points);
	}

	// restore matrixes
	restore();

	// nothing to do here we are done.
	if (points.empty())
		return false;

	// transform capsule points.
	for (auto& p : points) {
		mstudiobbox_t* bbox = set->GetHitbox(p.m_index);

		if (p.m_not_capsule) {

			matrix3x4_t rot_matrix;
			g_csgo.AngleMatrix(bbox->m_angle, rot_matrix);

			// apply the rotation to the entity input space (local).
			matrix3x4_t matrix;
			math::ConcatTransforms(record->m_bones[bbox->m_bone], rot_matrix, matrix);

			// extract origin from matrix.
			vec3_t origin = matrix.GetOrigin();

			// VectorRotate.
			// rotate point by angle stored in matrix.
			p.m_point = { p.m_point.dot(matrix[0]), p.m_point.dot(matrix[1]), p.m_point.dot(matrix[2]) };

			// transform point to world space.
			p.m_point += origin;
		}
		else
			math::VectorTransform(p.m_point, record->m_bones[bbox->m_bone], p.m_point);
	}

	/*
	for (const auto& point : points) {

		Color col{ 255, 0, 0 };

		if (point.m_center)
			col = { 0, 255, 0 };

		g_csgo.m_debug_overlay->AddBoxOverlay(point.m_point, vec3_t(-0.5, -0.5, -0.5), vec3_t(0.5, 0.5, 0.5), ang_t(0, 0, 0), col.r(), col.g(), col.b(), 175.f, 0.05);
	}*/

	// set player matrixes to custom matrixes
	apply();

	bool has_lethal{ false };

	// iterate aimpoints
	for (HitscanPoint_t& point : points) {

		penetration::PenetrationInput_t in;

		in.m_damage = dmg;
		in.m_damage_pen = pendmg;
		in.m_can_pen = pen;
		in.m_target = m_player;
		in.m_from = g_cl.m_local;
		in.m_pos = point.m_point;
		point.m_damage = -1;

		penetration::PenetrationOutput_t out;


		if (penetration::run(&in, &out)) {

			// nope we did not hit head..
			// that prevents from shooting hidden head apparently?
			// seems retarded but ok
			// actually im wrong that fixes head behind body situations 
			if (point.m_index <= 2 && out.m_hitgroup != HITGROUP_HEAD) {
				continue;
			}

			// faggots who are forward smh
			if (point.m_index == 7 && out.m_hitgroup == HITGROUP_HEAD) {
				continue;
			}

			// valid shot, set damage of that point
			point.m_damage = (int)out.m_damage;
		}
	}

	// restore matrixes after being done
	restore();

	// sort by center
	std::sort(points.begin(), points.end(), [&](const HitscanPoint_t a, const HitscanPoint_t b) { return (int)a.m_center > (int)b.m_center; });

	// sort by lethal
	std::sort(points.begin(), points.end(), [&](const HitscanPoint_t a, const HitscanPoint_t b) { 
		bool lethal_a = a.m_damage >= m_player->m_iHealth() && a.m_index > 2;
		bool lethal_b = b.m_damage >= m_player->m_iHealth() && b.m_index > 2;

		return (int)lethal_a > (int)lethal_b;
		});


	// woo, future bestpoint!!
	HitscanPoint_t* best_point = nullptr;
	bool m_resolved = record->m_mode == g_resolver.RESOLVE_WALK || record->m_mode == g_resolver.RESOLVE_BODY;

	// loop through all of our points
	for (HitscanPoint_t& point : points) {

		// we cant hit this point for enough damage / at all
		if (point.m_damage < 1)
			continue;

		// we got no point, so take first to come
		// and boom we can start comparing
		if (!best_point) {
			best_point = &point;
			continue;
		}

		// lethal body / resolved head
		if (point.m_damage >= m_player->m_iHealth() && (point.m_index > 2 || m_resolved)) {

			// bestpoint was head but we got lethal body
			if (best_point->m_index <= 2 && point.m_index > 2) {
				best_point = &point;
				continue;
			}

			// lethal + center, perfect
			if (point.m_center) {
				best_point = &point;
				break;
			}
		}

		bool best_point_lethal = best_point && best_point->m_damage >= m_player->m_iHealth() && (best_point->m_index > 2 || m_resolved);

		// bestpoint isnt lethal
		if (!best_point_lethal) {

			// go for prefered hitbox instead
			if (m_prefer_body) {
				int two_shot = (int)std::round(point.m_damage * 2.f);
				if (point.m_index > 2 && two_shot >= m_player->m_iHealth()) {
					best_point = &point;
					break;
				}
			}

			bool prefer_center = g_menu.main.aimbot.prefer_center.get() && (point.m_center && std::abs(best_point->m_damage - point.m_damage) <= 2 && best_point->m_damage < m_player->m_iHealth() && !best_point->m_center);

			// just take highest damage if no prefered hitbox shootable
			// or we're not prefering any hitbox
			if (point.m_damage >= best_point->m_damage || prefer_center)
				best_point = &point;
		}

		// this might seem retarded but at this point that means we got no prefered hitbox / resolved
		// so lets just take a lethal center of any hitbox
		if (best_point->m_center && best_point->m_damage >= m_player->m_iHealth())
			break;
	}

	// set data.
	if (best_point) {
		scan.m_pos = best_point->m_point;
		scan.m_damage = best_point->m_damage;
		scan.m_hitbox = best_point->m_index;
	}

	// we found something that we can damage.
	// set out vars.
	if (scan.m_damage > 0.f) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		//hitbox = scan.m_hitbox;
		return true;
	}

	return false;
}

bool Aimbot::SelectTarget( LagRecord *record, const vec3_t &aim, float damage ) {
	return false;
}

void Aimbot::apply( ) {
	bool attack, attack2;

	// attack states.
	attack = ( g_cl.m_cmd->m_buttons & IN_ATTACK );
	attack2 = ( g_cl.m_weapon_id == REVOLVER && g_cl.m_cmd->m_buttons & IN_ATTACK2 );

	// ensure we're attacking.
	if ( attack || attack2 ) {
		// choke every shot.
		*g_cl.m_packet = true;

		g_movement.fired_shot = true;



		if (g_aimbot.dt_aim)
			g_aimbot.dt_aim = false;

		if (g_tickshift.m_charged_ticks)
			dt_aim = true;


//		g_notify.add(tfm::format("hc on this tick is %i\n", goal_hc));

		if ( m_target ) {
			// make sure to aim at un-interpolated data.
			// do this so BacktrackEntity selects the exact record.
			if ( m_record && !m_record->m_broke_lc )
				g_cl.m_cmd->m_tick = game::TIME_TO_TICKS( m_record->m_sim_time + g_cl.m_lerp );

			// set angles to target.
			g_cl.m_cmd->m_view_angles = m_angle;

			// if not silent aim, apply the viewangles.
			if ( !g_menu.main.aimbot.silent.get( ) )
				g_csgo.m_engine->SetViewAngles( m_angle );

			g_visuals.DrawHitboxMatrix( m_record, colors::white, 1.f );
		}

		g_cl.m_cmd->m_view_angles -= g_cl.m_local->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( );

		// store fired shot.
		g_shots.StoreLastFireData( m_target ? m_target : nullptr, m_target ? m_damage : -1.f, g_cl.m_weapon_info->m_bullets, m_target ? m_record : nullptr );

		// set that we fired.
		g_cl.m_shot = true;
	}
}

void Aimbot::NoSpread( ) {
	
}