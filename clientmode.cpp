#include "includes.h"
#include "shifting.h"

bool Hooks::ShouldDrawParticles( ) {
	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawParticles_t >( IClientMode::SHOULDDRAWPARTICLES )( this );
}

bool Hooks::ShouldDrawFog( ) {
	// remove fog.
	if( g_menu.main.visuals.nofog.get( ) )
		return false;

	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawFog_t >( IClientMode::SHOULDDRAWFOG )( this );
}

void Hooks::OverrideView( CViewSetup* view ) {
	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	// g_grenades.think( );
	g_visuals.ThirdpersonThink( );

    // call original.
	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >( IClientMode::OVERRIDEVIEW )( this, view );

    // remove scope edge blur.
	if( g_menu.main.visuals.noscope.get( ) ) {
		if( g_cl.m_local && g_cl.m_local->m_bIsScoped( ) )
            view->m_edge_blur = 0;
	}
}

bool Hooks::CreateMove( float time, CUserCmd* cmd ) {
	Stack   stack;
	bool    ret;

	// let original run first.
	ret = g_hooks.m_client_mode.GetOldMethod< CreateMove_t >( IClientMode::CREATEMOVE )( this, time, cmd );

	// called from CInput::ExtraMouseSample -> return original.
	if( !cmd->m_command_number )
		return ret;

	// if we arrived here, called from -> CInput::CreateMove
	// call EngineClient::SetViewAngles according to what the original returns.
	if( ret )
		g_csgo.m_engine->SetViewAngles( cmd->m_view_angles );

	// random_seed isn't generated in ClientMode::CreateMove yet, we must set generate it ourselves.
	cmd->m_random_seed = g_csgo.MD5_PseudoRandom( cmd->m_command_number ) & 0x7fffffff;

	// get bSendPacket off the stack.
	g_cl.m_packet = stack.next( ).local( 0x1c ).as< bool* >( );

	// get bFinalTick off the stack.
	g_cl.m_final_packet = stack.next( ).local( 0x1b ).as< bool* >( );

	// invoke move function.
	g_cl.OnTick( cmd );

	return false;
}

bool Hooks::DoPostScreenSpaceEffects( CViewSetup* setup ) {
	g_visuals.RenderGlow( );

	return g_hooks.m_client_mode.GetOldMethod< DoPostScreenSpaceEffects_t >( IClientMode::DOPOSTSPACESCREENEFFECTS )( this, setup );
}

void WriteUsercmd(bf_write* buf, CUserCmd* in, CUserCmd* out) {
	static auto WriteUsercmdF = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));

	__asm
	{
		mov ecx, buf
		mov edx, in
		push out
		call WriteUsercmdF
		add esp, 4
	}
}

bool Hooks::WriteUsercmdDeltaToBuffer(int slot, bf_write* buf, int from, int to, bool isnewcommand) {
	if (g_cl.m_processing && g_csgo.m_engine->IsConnected() && g_csgo.m_engine->IsInGame()) {
		uintptr_t stackbase;
		__asm mov stackbase, ebp;
		CCLCMsg_Move_t* msg = reinterpret_cast<CCLCMsg_Move_t*>(stackbase + 0xFCC);

		if (g_tickshift.m_tick_to_shift_alternate > 0) {
			if (from != -1)
				return true;

			int32_t new_commands = msg->new_commands;
			int Next_Command = g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands + 1;
			int CommandsToAdd = std::min(g_tickshift.m_tick_to_shift_alternate, 16);

			g_tickshift.m_tick_to_shift_alternate = 0;
			msg->new_commands = CommandsToAdd;
			msg->backup_commands = 0;
			from = -1;

			for (to = Next_Command - new_commands + 1; to <= Next_Command; to++) {
				if (!g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >(23)(this, slot, buf, from, to, isnewcommand))
					return false;
				from = to;
			}

			CUserCmd* last_command = g_csgo.m_input->GetUserCmd(slot, from);
			CUserCmd nullcmd;
			CUserCmd ShiftCommand;

			if (last_command)
				nullcmd = *last_command;

			ShiftCommand = nullcmd;
			ShiftCommand.m_command_number++;
			ShiftCommand.m_tick += 100;

			for (int i = new_commands; i <= CommandsToAdd; i++) {
				WriteUsercmd(buf, &ShiftCommand, &nullcmd);
				nullcmd = ShiftCommand;
				ShiftCommand.m_command_number++;
				ShiftCommand.m_tick++;
			}
		}
	}

	return g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >(23)(this, slot, buf, from, to, isnewcommand);
}
