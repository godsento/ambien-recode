#include "includes.h"

void AimPlayer::UpdateAnimations(LagRecord* record) {
	CCSGOPlayerAnimState* state = m_player->m_PlayerAnimState();
	if (!state)
		return;

	// player respawned.
	if (m_player->m_flSpawnTime() != m_spawn) {
		// reset animation state.
		game::ResetAnimationState(state);

		// note new spawn time.
		m_spawn = m_player->m_flSpawnTime();
	}

	// backup curtime.
	float curtime = g_csgo.m_globals->m_curtime;
	float frametime = g_csgo.m_globals->m_frametime;

	// set curtime to animtime.
	// set frametime to ipt just like on the server during simulation.
	g_csgo.m_globals->m_curtime = record->m_sim_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	// backup stuff that we do not want to fuck with.
	AnimationBackup_t backup;

	backup.m_origin = m_player->m_vecOrigin();
	backup.m_abs_origin = m_player->GetAbsOrigin();
	backup.m_velocity = m_player->m_vecVelocity();
	backup.m_abs_velocity = m_player->m_vecAbsVelocity();
	backup.m_flags = m_player->m_fFlags();
	backup.m_eflags = m_player->m_iEFlags();
	backup.m_duck = m_player->m_flDuckAmount();
	backup.m_body = m_player->m_flLowerBodyYawTarget();
	m_player->GetAnimLayers(backup.m_layers);

	// is player a bot?
	bool bot = game::IsFakePlayer(m_player->index());

	// reset fakewalk state.
	record->m_fake_walk = false;
	record->m_mode = Resolver::Modes::RESOLVE_NONE;
	record->m_resolver_mode = "none";

	if (m_records.size() >= 2) {
		// get pointer to previous record.
		LagRecord* previous = m_records[1].get();

		if (previous && !previous->dormant()) {

			auto e = m_player;
			vec3_t velocity = m_player->m_vecVelocity();
			bool was_in_air = (m_player->m_fFlags() & FL_ONGROUND) && (previous->m_flags & FL_ONGROUND);

			auto time_difference = std::max(g_csgo.m_globals->m_interval, m_player->m_flSimulationTime() - previous->m_sim_time);
			auto origin_delta = m_player->m_vecOrigin() - previous->m_origin;

			auto animation_speed = 0.0f;

			if (!origin_delta.IsZero() && game::TIME_TO_TICKS(time_difference) > 0 && game::TIME_TO_TICKS(time_difference) < 16)
			{

				record->m_velocity = origin_delta * (1.0f / time_difference);

				// it will get overriden
				record->m_anim_velocity = record->m_velocity;

				if (m_player->m_fFlags() & FL_ONGROUND && record->m_layers[11].m_weight > 0.0f && record->m_layers[11].m_weight < 1.0f && record->m_layers[11].m_cycle > previous->m_layers[11].m_cycle)
				{
					auto weapon = m_player->m_hActiveWeapon().Get();

					if (weapon)
					{
						auto max_speed = 260.0f;
						auto weapon_info = e->GetActiveWeapon()->GetWpnData();

						if (weapon_info)
							max_speed = m_player->m_bIsScoped() ? weapon_info->m_max_player_speed_alt : weapon_info->m_max_player_speed;

						auto modifier = 0.35f * (1.0f - record->m_layers[11].m_weight);

						if (modifier > 0.0f && modifier < 1.0f)
							animation_speed = max_speed * (modifier + 0.55f);
					}
				}

				if (animation_speed > 0.0f)
				{
					animation_speed /= record->m_anim_velocity.length_2d();

					record->m_anim_velocity.x *= animation_speed;
					record->m_anim_velocity.y *= animation_speed;
				}

				if (m_records.size() >= 3 && time_difference > g_csgo.m_globals->m_interval && !m_records[2].get()->dormant())
				{
					auto previous_velocity = (previous->m_origin - m_records[2].get()->m_origin) * (1.0f / time_difference);

					if (!previous_velocity.IsZero() && !was_in_air)
					{
						auto current_direction = math::NormalizedAngle(math::rad_to_deg(atan2(record->m_anim_velocity.y, record->m_anim_velocity.x)));
						auto previous_direction = math::NormalizedAngle(math::rad_to_deg(atan2(previous_velocity.y, previous_velocity.x)));

						auto average_direction = current_direction - previous_direction;
						average_direction = math::deg_to_rad(math::NormalizedAngle(current_direction + average_direction * 0.5f));

						auto direction_cos = cos(average_direction);
						auto dirrection_sin = sin(average_direction);

						auto velocity_speed = record->m_anim_velocity.length_2d();

						record->m_anim_velocity.x = direction_cos * velocity_speed;
						record->m_anim_velocity.y = dirrection_sin * velocity_speed;
					}
				}

				if (!(m_player->m_fFlags() & FL_ONGROUND))
				{
					auto fixed_timing = std::clamp(time_difference, g_csgo.m_globals->m_interval, 1.0f);
					record->m_anim_velocity.z -= g_csgo.sv_gravity->GetFloat() * fixed_timing * 0.5f;
				}
				else
					record->m_anim_velocity.z = 0.0f;
			}

			m_player->m_iEFlags() &= ~0x1800;

			if (m_player->m_fFlags() & FL_ONGROUND && record->m_anim_velocity.length() > 0.0f && record->m_layers[6].m_weight <= 0.0f)
				record->m_anim_velocity = vec3_t(0, 0, 0);

			bool two_tick_ground = (m_player->m_fFlags() & FL_ONGROUND) && (previous->m_flags & FL_ONGROUND);

			//https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/player_lagcompensation.cpp#L462-L467
			record->m_broke_lc = (record->m_origin - previous->m_origin).length_2d_sqr() > 4096.f && !two_tick_ground;

			// lagcompensation time
			// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/player_lagcompensation.cpp#L504
			record->m_lag_time = record->m_sim_time - previous->m_sim_time;
		}
	}

	// fix various issues with the game eW91dHViZS5jb20vZHlsYW5ob29r
	// these issues can only occur when a player is choking data.
	if (record->m_lag > 1) {
		// detect fakewalk.
		float speed = record->m_velocity.length();

		if (record->m_flags & FL_ONGROUND
			&& record->m_layers[4].m_weight == 0.0f
			&& record->m_layers[5].m_weight == 0.0f
			&& record->m_layers[6].m_playback_rate < 0.0001f
			&& record->m_anim_velocity.length_2d() > 0.f && record->m_lag > 7)
			record->m_fake_walk = true;

		if (record->m_fake_walk)
			record->m_velocity = record->m_anim_velocity = { 0.f, 0.f, 0.f };

		// we need atleast 2 updates/records
		// to fix these issues.
		if (m_records.size() >= 2) {
			// get pointer to previous record.
			LagRecord* previous = m_records[1].get();

			if (previous && !previous->dormant()) {

				// jumpfall.
				bool bOnGround = record->m_flags & FL_ONGROUND;
				bool bJumped = false;
				bool bLandedOnServer = false;
				float flLandTime = 0.f;

				// magic llama code.
				if (record->m_layers[4].m_cycle < 0.5f && (!(record->m_flags & FL_ONGROUND) || !(previous->m_flags & FL_ONGROUND))) {
					flLandTime = record->m_sim_time - float(record->m_layers[4].m_playback_rate / record->m_layers[4].m_cycle);
					bLandedOnServer = flLandTime >= previous->m_sim_time;
				}

				// jump_fall fix
				if (bLandedOnServer && !bJumped) {
					if (flLandTime <= record->m_anim_time) {
						bJumped = true;
						bOnGround = true;
					}
					else {
						bOnGround = previous->m_flags & FL_ONGROUND;
					}
				}

				// do the fix. hahaha
				if (bOnGround) {
					m_player->m_fFlags() |= FL_ONGROUND;
				}
				else {
					m_player->m_fFlags() &= ~FL_ONGROUND;
				}

				// fix crouching players.
				// the duck amount we receive when people choke is of the last simulation.
				// if a player chokes packets the issue here is that we will always receive the last duckamount.
				// but we need the one that was animated.
				// therefore we need to compute what the duckamount was at animtime.

				// delta in duckamt and delta in time..
				float duck = record->m_duck - previous->m_duck;

				// get the duckamt change per tick.
				float change = (duck / record->m_lag_time) * g_csgo.m_globals->m_interval;

				// fix crouching players.
				m_player->m_flDuckAmount() = previous->m_duck + change;
			}
		}
	}


	bool fake = !bot && record->m_lag > 0;
	// if using fake angles, correct angles.
	if (fake)
		g_resolver.ResolveAngles(m_player, record);

	// set stuff before animating.
	m_player->m_vecOrigin() = record->m_origin;
	m_player->m_vecVelocity() = m_player->m_vecAbsVelocity() = record->m_anim_velocity;
	m_player->m_flLowerBodyYawTarget() = record->m_body;

	// EFL_DIRTY_ABSVELOCITY
	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	m_player->m_iEFlags() &= ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY);

	// write potentially resolved angles.
	m_player->m_angEyeAngles() = record->m_eye_angles;

	// fix animating in same frame.
	if (state->m_frame >= g_csgo.m_globals->m_frame)
		state->m_frame -= 1;

	// 'm_animating' returns true if being called from SetupVelocity, passes raw velocity to animstate.
	m_player->m_bClientSideAnimation() = true;
	m_player->UpdateClientSideAnimation();
	m_player->m_bClientSideAnimation() = false;

	// store updated/animated poses and rotation in lagrecord.
	m_player->GetPoseParameters(record->m_poses);
	record->m_abs_ang = m_player->GetAbsAngles();

	g_bones.BuildBones(m_player, 0x7FF00, record->m_bones, record);
	record->m_setup = record->m_bones;

	// restore backup data.
	m_player->m_vecOrigin() = backup.m_origin;
	m_player->m_vecVelocity() = backup.m_velocity;
	m_player->m_vecAbsVelocity() = backup.m_abs_velocity;
	m_player->m_fFlags() = backup.m_flags;
	m_player->m_iEFlags() = backup.m_eflags;
	m_player->m_flDuckAmount() = backup.m_duck;
	m_player->m_flLowerBodyYawTarget() = backup.m_body;
	m_player->SetAbsOrigin(backup.m_abs_origin);
	m_player->SetAnimLayers(backup.m_layers);

	// IMPORTANT: do not restore poses here, since we want to preserve them for rendering.
	// also dont restore the render angles which indicate the model rotation.

	// restore globals.
	g_csgo.m_globals->m_curtime = curtime;
	g_csgo.m_globals->m_frametime = frametime;
}

void AimPlayer::OnNetUpdate(Player* player) {
	bool reset = (!g_menu.main.aimbot.enable.get() || player->m_lifeState() == LIFE_DEAD || !player->enemy(g_cl.m_local));
	bool disable = (!reset && !g_cl.m_processing);

	// if this happens, delete all the lagrecords.
	if (reset) {
		player->m_bClientSideAnimation() = true;
		m_body_update = FLT_MAX;
		m_moved = false;
		m_records.clear();
		m_last_freestand_scan = 0;
		return;
	}

	// just disable anim if this is the case.
	if (disable) {
		player->m_bClientSideAnimation() = true;
		m_body_update = FLT_MAX;
		m_moved = false;
		m_last_freestand_scan = 0;
		m_records.clear();
		return;
	}

	// update player ptr if required.
	// reset player if changed.
	if (m_player != player)
		m_records.clear();

	// update player ptr.
	m_player = player;

	// indicate that this player has been out of pvs.
	// insert dummy record to separate records
	// to fix stuff like animation and prediction.
	if (player->dormant()) {
		bool insert = true;
		m_body_update = FLT_MAX;
		m_moved = false;
		m_last_freestand_scan = player->m_flSimulationTime() + 0.23f;

		// we have any records already?
		if (!m_records.empty()) {

			LagRecord* front = m_records.front().get();

			// we already have a dormancy separator.
			if (front->dormant())
				insert = false;
		}

		if (insert) {
			// add new record.
			m_records.emplace_front(std::make_shared< LagRecord >(player));

			// get reference to newly added record.
			LagRecord* current = m_records.front().get();

			// mark as dormant.
			current->m_dormant = true;
		}

		while (m_records.size() > 1)
			m_records.pop_back();

		return;
	}

	bool update = (m_records.empty() || player->m_flSimulationTime() > m_records.front().get()->m_sim_time);


	// this is the first data update we are receving
	// OR we received data with a newer simulation context.
	if (update) {
		// add new record.
		m_records.emplace_front(std::make_shared< LagRecord >(player));

		// get reference to newly added record.
		LagRecord* current = m_records.front().get();

		// mark as non dormant.
		current->m_dormant = false;

		// update animations on current record.
		// call resolver.
		UpdateAnimations(current);

		if (!m_records.back().get()->valid() && m_records.size() > 3)
			m_records.pop_back();
	}


	//while (m_records.size() > 2 && g_tickshift.m_charged)
	//	m_records.pop_back();

	while (m_records.size() > 3 && m_records.front().get() && m_records.front().get()->m_broke_lc)
		m_records.pop_back();

	while (m_records.size() > (int)std::round(1.f / g_csgo.m_globals->m_interval))
		m_records.pop_back();

}