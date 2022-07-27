#include "includes.h"
/*
void Client::UpdateAnimations() {
	if (!g_cl.m_local || !g_cl.m_processing)
		return;

	// set the interp flag on
	g_cl.m_local->m_fEffects() &= ~EF_NOINTERP;

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState();
	if (!state)
		return;

	// prevent model sway on player.
	g_cl.m_local->m_AnimOverlay()[12].m_weight = 0.f;

	// update animations with last networked data.
	g_cl.m_local->SetPoseParameters(g_cl.m_poses);

	// update abs yaw with last networked abs yaw.
	g_cl.m_local->SetAbsAngles(ang_t(0.f, g_cl.m_abs_yaw, 0.f));
}

void Client::ApplyUpdatedAnimation() {
	if (!g_cl.m_local || !g_cl.m_processing || !g_cl.m_local->alive() || !g_cl.m_local->m_iHealth())
		return;

//	g_cl.m_local->m_fEffects() |= EF_NOINTERP;

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState();
	if (!state)
		return;

	// set radar angles.
	if (g_csgo.m_input->CAM_IsThirdPerson())
		g_csgo.m_prediction->SetLocalViewAngles(m_radar);

	// update abs yaw with last networked abs yaw.
	g_cl.m_local->SetAbsAngles(ang_t(0.f, g_cl.m_abs_yaw, 0.f));
}

void Client::UpdateLocalAnimations() {
	if (!g_cl.m_local || !g_cl.m_processing || !g_cl.m_local->alive() || !g_cl.m_local->m_iHealth() )
		return;

	g_cl.m_local->m_fEffects() |= EF_NOINTERP;

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState();
	if (!state)
		return;

	static float m_spawn_time = 0.f;
	static C_AnimationLayer m_layers[13]{};

	// local respawned.
	if (g_cl.m_local->m_flSpawnTime() != m_spawn_time) {
		// reset animation state.
		game::ResetAnimationState(state);

		// note new spawn time.
		m_spawn_time = g_cl.m_local->m_flSpawnTime();
	}

	auto ApplyLocalPlayerModifications = [&]() -> void {
		// havent got updated layers and poses.
		if (!m_layers || !g_cl.m_poses)
			return;

		// note - vio; related to fixing animations on high ping.
		auto delta = math::NormalizedAngle(state->m_cur_feet_yaw - state->m_goal_feet_yaw);
		if ((delta / state->m_eye_pitch) > 120.f) {
			// set this shit.
			m_layers[3].m_cycle = 0.0f;
			m_layers[3].m_weight = 0.0f;
		}

		if (g_cl.m_poses)
			g_cl.m_poses[6] = 1.f;

		// prevent model sway on player.
		if (m_layers)
			m_layers[12].m_weight = 0.f;
	};

	// backup curtime and frametime.
	const float v1 = g_csgo.m_globals->m_curtime;
	const float v2 = g_csgo.m_globals->m_frametime;

	// get tickbase in time and modify it.
	const float v3 = game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase());
	const float v4 = (v3 / g_csgo.m_globals->m_interval) + .5f;

	// set curtime and frametime.
	g_csgo.m_globals->m_curtime = v3;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	// llama does it.
	state->m_cur_feet_yaw = 0.f;

	// CCSGOPlayerAnimState::Update, bypass already animated checks.
	if (state->m_frame >= v4)
		state->m_frame = v4 - 1;

	// update anim update delta like the server does.
	state->m_update_delta = std::max(0.0f, g_csgo.m_globals->m_curtime - state->m_frame);

	// is it time to update?
	if (g_csgo.m_globals->m_curtime != state->m_time) {
		// allow the game to update animations this tick.
		g_cl.m_update_local_animation = true;

		// update our animation state.
		game::UpdateAnimationState(state, ang_t(m_angle.x, m_angle.y, m_angle.z));

		// update animations.
		g_cl.m_local->UpdateClientSideAnimationEx();

		// stop the game from updating animations this tick.
		g_cl.m_update_local_animation = false;

		// store updated abs yaw.
		g_cl.m_abs_yaw = state->m_goal_feet_yaw;

		g_cl.m_local->m_flPoseParameter()[6] = 1.f;

		// grab updated layers & poses.
		g_cl.m_local->GetPoseParameters(g_cl.m_poses);
		g_cl.m_local->GetAnimLayers(m_layers);
	}

	// modify our animations to look steezy.
	ApplyLocalPlayerModifications();

	// send poses and layers to server.
	g_cl.m_local->SetPoseParameters(g_cl.m_poses);
	g_cl.m_local->SetAnimLayers(m_layers);

	// update rotation.
	g_cl.m_local->SetAbsAngles(g_cl.m_rotation);

	// restore globals.
	g_csgo.m_globals->m_curtime = v1;
	g_csgo.m_globals->m_frametime = v2;

}

void Client::update_shit() {

	if (!g_cl.m_local || !g_cl.m_processing || !g_cl.m_local->alive() || !g_cl.m_local->m_iHealth())
		return;

	g_cl.m_local->m_fEffects() |= EF_NOINTERP;

	if (g_cl.m_lag > 0)
		return;



	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState();
	if (!state)
		return;




	// update time.
	g_cl.m_anim_frame = g_csgo.m_globals->m_curtime - g_cl.m_anim_time;
	g_cl.m_anim_time = g_csgo.m_globals->m_curtime;

	// current angle will be animated.
	g_cl.m_angle = g_cl.m_cmd->m_view_angles;

	// fix landing anim.
	if (state->m_land && !state->m_dip_air && state->m_dip_cycle > 0.f && g_cl.m_flags & FL_ONGROUND)
		g_cl.m_angle.x = -12.f;

	math::clamp(g_cl.m_angle.x, -90.f, 90.f);
	g_cl.m_angle.normalize();

	// set lby to predicted value.
	//g_cl.m_local->m_flLowerBodyYawTarget() = g_cl.m_body;

	// pull the lower body direction towards the eye direction, but only when the player is moving
	if (state->m_ground) {
		// from csgo src sdk.
		const float CSGO_ANIM_LOWER_REALIGN_DELAY = 1.1f;

		// we are moving.
		if (state->m_speed > 0.1f || fabsf(state->m_fall_velocity) > 100.f) {
			g_cl.m_body_pred = g_cl.m_anim_time + (CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f);
			g_cl.m_body = g_cl.m_angle.y;
		}

		// we arent moving.
		else {
			// time for an update.
			if (g_cl.m_anim_time >= g_cl.m_body_pred) {
				g_cl.m_body_pred = g_cl.m_anim_time + CSGO_ANIM_LOWER_REALIGN_DELAY;
				g_cl.m_body = g_cl.m_angle.y;
			}
		}
	}

	// save updated data.
	g_cl.m_rotation = g_cl.m_local->m_angAbsRotation();
	g_cl.m_speed = state->m_speed;
	g_cl.m_ground = state->m_ground;
}*/


inline const char* dir[]{
	"models/player/custom_player/legacy/tm_phoenix.mdl",
	"models/player/custom_player/eminem/css/t_arctic.mdl",
	"models/player/custom_player/owston/amongus"

};

void Client::SetAngles() {

	if (!g_cl.m_local || !g_cl.m_processing || !g_cl.m_local->alive() || !g_cl.m_local->m_iHealth())
		return;



	// set the nointerp flag.
	// g_cl.m_local->m_fEffects( ) &= ~EF_NOINTERP;
	g_cl.m_local->m_fEffects() |= EF_NOINTERP;

	// apply the rotation.
	g_cl.m_local->SetAbsAngles(m_rotation);
	g_cl.m_local->m_angRotation() = m_rotation;
	g_cl.m_local->m_angNetworkAngles() = m_rotation;

	// set radar angles.
	if (g_csgo.m_input->CAM_IsThirdPerson())
		g_csgo.m_prediction->SetLocalViewAngles(m_radar);
}

void Client::UpdateAnimations() {

	if (!g_cl.m_local || !g_cl.m_processing || !g_cl.m_local->alive() || !g_cl.m_local->m_iHealth())
		return;

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState();
	if (!state)
		return;

	// prevent model sway on player.
	g_cl.m_local->m_AnimOverlay()[12].m_weight = 0.f;

	// update animations with last networked data.
	g_cl.m_local->SetPoseParameters(g_cl.m_poses);


	//if (g_hvh.m_desync)
		//g_cl.m_abs_yaw += (bool)g_csgo.RandomInt(0, 1) ? g_csgo.RandomFloat(90.f, 145.f) : -g_csgo.RandomFloat(90.f, 145.f);

	// update abs yaw with last networked abs yaw.
	g_cl.m_local->SetAbsAngles(ang_t(0.f, g_cl.m_abs_yaw, 0.f));
}

void Client::UpdateInformation() {

	if (!g_cl.m_local || !g_cl.m_processing || !g_cl.m_local->alive() || !g_cl.m_local->m_iHealth())
		return;

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState();
	if (!state)
		return;


	if (g_cl.m_lag > 0) {
		m_goal_feet_yaw_fake = state->m_goal_feet_yaw;
		return;
	}



	// update time.
	m_anim_frame = g_csgo.m_globals->m_curtime - m_anim_time;
	m_anim_time = g_csgo.m_globals->m_curtime;

	// current angle will be animated.
	m_angle = g_cl.m_cmd->m_view_angles;

	// fix landing anim.
	if (state->m_land && !state->m_dip_air && state->m_dip_cycle > 0.f && g_cl.m_flags & FL_ONGROUND)
		m_angle.x = -12.f;

	math::clamp(m_angle.x, -90.f, 90.f);
	m_angle.normalize();


	//	if (g_hvh.m_desync)
	//		m_angle.y = g_hvh.m_direction;

		// write angles to model.
	//g_csgo.m_prediction->SetLocalViewAngles(m_angle);
	game::UpdateAnimationState(state, m_angle);

	if (state->m_speed > 0.1f) {
		if (state->m_ground)
			m_body = m_angle.y;

		m_body_pred = m_anim_time + 0.22f;
	}

	// standing update every 1.1s
	else if (m_anim_time >= m_body_pred) {
		m_body = m_angle.y;
		m_body_pred = m_anim_time + 1.1f;
	}

	// set lby to predicted value.
//	if (!g_hvh.m_desync)
	g_cl.m_local->m_flLowerBodyYawTarget() = m_body;
	//	else
	//		g_cl.m_local->m_flLowerBodyYawTarget() = math::NormalizedAngle(g_csgo.RandomFloat(360, -360));

		// CCSGOPlayerAnimState::Update, bypass already animated checks.
	if (state->m_frame >= g_csgo.m_globals->m_frame)
		state->m_frame = g_csgo.m_globals->m_frame - 1;

	// call original, bypass hook.
	g_hooks.m_UpdateClientSideAnimation(g_cl.m_local);

	// static legs bs
	g_cl.m_local->m_flPoseParameter()[6] = 1.f;

	// get last networked poses.
	g_cl.m_local->GetPoseParameters(g_cl.m_poses);

	//	if (g_hvh.m_desync)
	//		state->m_goal_feet_yaw = m_angle.y - pick_random;

		// store updated abs yaw.
	g_cl.m_abs_yaw = state->m_goal_feet_yaw;

	// save updated data.
	m_rotation = g_cl.m_local->m_angAbsRotation();
	m_speed = state->m_speed;
	m_ground = state->m_ground;
}
