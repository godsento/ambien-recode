#include "includes.h"

bool Hooks::InPrediction( ) {
	Stack stack;
	ang_t *angles;

	// note - dex; first 2 'test al, al' instructions in C_BasePlayer::CalcPlayerView.
	static Address CalcPlayerView_ret1{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 0B 8B 0D ? ? ? ? 8B 01 FF 50 4C" ) ) };
	static Address CalcPlayerView_ret2{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 08 57 8B CE E8 ? ? ? ? 8B 06" ) ) };

	if( g_cl.m_local && g_menu.main.visuals.removals.get(0)) {
		// note - dex; apparently this calls 'view->DriftPitch()'.
		//             i don't know if this function is crucial for normal gameplay, if it causes issues then comment it out.
		if( stack.ReturnAddress( ) == CalcPlayerView_ret1 )
			return true;

		if( stack.ReturnAddress( ) == CalcPlayerView_ret2 ) {
			// at this point, angles are copied into the CalcPlayerView's eyeAngles argument.
			// (ebp) InPrediction -> (ebp) CalcPlayerView + 0xC = eyeAngles.
			angles = stack.next( ).arg( 0xC ).to< ang_t* >( );

			if( angles ) {
				*angles -= g_cl.m_local->m_viewPunchAngle( )
					     + ( g_cl.m_local->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( ) )
					     * g_csgo.view_recoil_tracking->GetFloat( );
			}

			return true;
		}
	}

	return g_hooks.m_prediction.GetOldMethod< InPrediction_t >( CPrediction::INPREDICTION )( this );
}

void Hooks::RunCommand(Entity* ent, CUserCmd* cmd, IMoveHelper* movehelper) {
	if (!ent || ent->index() != g_csgo.m_engine->GetLocalPlayer())
		return g_hooks.m_prediction.GetOldMethod< RunCommand_t >(CPrediction::RUNCOMMAND)(this, ent, cmd, movehelper);

	if (cmd->m_tick >= (g_cl.m_tick + int(1 / g_csgo.m_globals->m_interval) + 8)) {
		cmd->m_predicted = true;
		return;
	}

	// get player pointer.
	Player* player = (Player*)ent;

	// backup variables.
	int backup_tickbase = player->m_nTickBase();
	float backup_curtime = g_csgo.m_globals->m_curtime;

	// fix tickbase when shifting.
	if (cmd->m_command_number == g_tickshift.m_shift_cmd) {
		player->m_nTickBase() = game::TIME_TO_TICKS(player->m_flSimulationTime());
		++player->m_nTickBase();

		g_csgo.m_globals->m_curtime = game::TICKS_TO_TIME(player->m_nTickBase());
	}
	g_hooks.m_prediction.GetOldMethod< RunCommand_t >(CPrediction::RUNCOMMAND)(this, ent, cmd, movehelper);

	// store non compressed netvars.
	g_netdata.store();
}