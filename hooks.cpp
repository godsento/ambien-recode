#include "includes.h"
#include "minhook/minhook.h"
#include "shifting.h"

Hooks                g_hooks{ };;
CustomEntityListener g_custom_entity_listener{ };;

void Pitch_proxy( CRecvProxyData *data, Address ptr, Address out ) {
	// normalize this fucker.
	math::NormalizeAngle( data->m_Value.m_Float );

	// clamp to remove retardedness.
	math::clamp( data->m_Value.m_Float, -90.f, 90.f );

	// call original netvar proxy.
	if ( g_hooks.m_Pitch_original )
		g_hooks.m_Pitch_original( data, ptr, out );
}

void Body_proxy( CRecvProxyData *data, Address ptr, Address out ) {

	// call original proxy.
	if ( g_hooks.m_Body_original )
		g_hooks.m_Body_original( data, ptr, out );
}

void AbsYaw_proxy( CRecvProxyData *data, Address ptr, Address out ) {

	// call original netvar proxy.
	if ( g_hooks.m_AbsYaw_original )
		g_hooks.m_AbsYaw_original( data, ptr, out );
}


void Force_proxy( CRecvProxyData *data, Address ptr, Address out ) {
	// convert to ragdoll.
	Ragdoll *ragdoll = ptr.as< Ragdoll * >( );

	// get ragdoll owner.
	Player *player = ragdoll->GetPlayer( );

	// we only want this happening to noobs we kill.
	if ( g_menu.main.misc.ragdoll_force.get( ) && g_cl.m_local && player && player->enemy( g_cl.m_local ) ) {
		// get m_vecForce.
		vec3_t vel = { data->m_Value.m_Vector[ 0 ], data->m_Value.m_Vector[ 1 ], data->m_Value.m_Vector[ 2 ] };

		// give some speed to all directions.
		vel *= 1000.f;

		// boost z up a bit.
		if ( vel.z <= 1.f )
			vel.z = 2.f;

		vel.z *= 2.f;

		// don't want crazy values for this... probably unlikely though?
		math::clamp( vel.x, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.y, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.z, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );

		// set new velocity.
		data->m_Value.m_Vector[ 0 ] = vel.x;
		data->m_Value.m_Vector[ 1 ] = vel.y;
		data->m_Value.m_Vector[ 2 ] = vel.z;
	}

	if ( g_hooks.m_Force_original )
		g_hooks.m_Force_original( data, ptr, out );
}

auto __fastcall ShouldSkipAnimationFrame(void* ecx, void* edx) -> bool {
	return false;
}

bool ShouldDrawViewModel() {

	if (!g_cl.m_processing || !g_csgo.m_engine->IsInGame() || !g_csgo.m_engine->IsConnected())
		return false;

	if (!g_menu.main.skins.draw_model_scope.get() && g_cl.m_local->m_bIsScoped())
		return false;

	return true;
}

bool Hooks::IsPaused() {
	static DWORD* return_to_extrapolation = (DWORD*)(pattern::find(g_csgo.m_client_dll,
		XOR("FF D0 A1 ?? ?? ?? ?? B9 ?? ?? ?? ?? D9 1D ?? ?? ?? ?? FF 50 34 85 C0 74 22 8B 0D ?? ?? ?? ??")) + 0x29);

	if (_ReturnAddress() == (void*)return_to_extrapolation)
		return true;

	return g_hooks.m_engine.GetOldMethod< IsPaused_t >(IVEngineClient::ISPAUSED)(this);
}

//



// thanks for this and the function above sharklazer!
void ReceivedSimulationTime(CRecvProxyData* pData, void* ent, void* pOut) {

	if (g_hooks.m_SimulationTime_original) {

		//g_cl.print("boop?");
		g_hooks.m_SimulationTime_original(pData, ent, pOut);
	}

	/*
	float time = (uintptr_t)pOut;
	Entity* pent = (Entity*)ent;
	if (pOut && pent && pent->IsPlayer() && pent != g_cl.m_local) {

		AimPlayer* m_player = &g_aimbot.m_players[pent->index() - 1];

		if (m_player && g_aimbot.IsValidTarget(m_player->m_player)) {

			// we should have at least two records while doing this to compare this too
			if (m_player->m_records.size() >= 2 && !m_player->m_records.empty()) { 

				// don't do this on the local player, i mean we shouldn't be keeping records of the local player anyways but still.
				if (pent != g_cl.m_local) {

					// do this for every record.
					for (const auto& records : m_player->m_records) { 
						// extract exact simulation time to what it was before it was compressed.
						if (g_csgo.m_globals->m_interval == (1.0f / 64.0f))
							time = ExtractLostPrecisionForSimulationTime(time); 

					//	g_cl.print("bud we fixing that simtime ong");

						records->m_sim_time = time; // set our simtime records to this.
					}
				}
			}
		}
	}*/
}

void Hooks::init( ) {

	MH_Initialize();
	MH_CreateHook(pattern::find(PE::GetModule(HASH("engine.dll")), XOR("55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D ? ? ? ? 8A")), &CL_Move, reinterpret_cast<void**>(&o_CLMove));
	MH_CreateHook(pattern::find(PE::GetModule(HASH("client.dll")), XOR("57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 02")), &ShouldSkipAnimationFrame, NULL);
	MH_CreateHook(pattern::find(PE::GetModule(HASH("client.dll")), XOR("55 8B EC 51 57 E8 ")), &ShouldDrawViewModel, NULL);
	MH_EnableHook(MH_ALL_HOOKS);

	// hook wndproc.
	m_old_wndproc = ( WNDPROC )g_winapi.SetWindowLongA( g_csgo.m_game->m_hWindow, GWL_WNDPROC, util::force_cast< LONG >( Hooks::WndProc ) );

	// setup normal VMT hooks.
	m_panel.init( g_csgo.m_panel );
	m_panel.add( IPanel::PAINTTRAVERSE, util::force_cast( &Hooks::PaintTraverse ) );

	m_client.init( g_csgo.m_client );
	m_client.add( CHLClient::LEVELINITPREENTITY, util::force_cast( &Hooks::LevelInitPreEntity ) );
	m_client.add( CHLClient::LEVELINITPOSTENTITY, util::force_cast( &Hooks::LevelInitPostEntity ) );
	m_client.add( CHLClient::LEVELSHUTDOWN, util::force_cast( &Hooks::LevelShutdown ) );
	m_client.add( CHLClient::WRITEUSERCMDDELTATOBUFFER, util::force_cast(&Hooks::WriteUsercmdDeltaToBuffer));
	m_client.add( CHLClient::FRAMESTAGENOTIFY, util::force_cast( &Hooks::FrameStageNotify ) );

	m_engine.init( g_csgo.m_engine );
	m_engine.add( IVEngineClient::ISCONNECTED, util::force_cast( &Hooks::IsConnected ) );
	m_engine.add( IVEngineClient::ISHLTV, util::force_cast( &Hooks::IsHLTV ) );
	m_engine.add( IVEngineClient::ISPAUSED, util::force_cast( &Hooks::IsPaused ) );

//	m_engine_sound.init( g_csgo.m_sound );
//	m_engine_sound.add( IEngineSound::EMITSOUND, util::force_cast( &Hooks::EmitSound ) );

	m_prediction.init( g_csgo.m_prediction );
	m_prediction.add( CPrediction::INPREDICTION, util::force_cast( &Hooks::InPrediction ) );
	m_prediction.add( CPrediction::RUNCOMMAND, util::force_cast( &Hooks::RunCommand ) );

	m_client_mode.init( g_csgo.m_client_mode );
	m_client_mode.add( IClientMode::SHOULDDRAWPARTICLES, util::force_cast( &Hooks::ShouldDrawParticles ) );
	m_client_mode.add( IClientMode::SHOULDDRAWFOG, util::force_cast( &Hooks::ShouldDrawFog ) );
	m_client_mode.add( IClientMode::OVERRIDEVIEW, util::force_cast( &Hooks::OverrideView ) );
	m_client_mode.add( IClientMode::CREATEMOVE, util::force_cast( &Hooks::CreateMove ) );
	m_client_mode.add( IClientMode::DOPOSTSPACESCREENEFFECTS, util::force_cast( &Hooks::DoPostScreenSpaceEffects ) );

	m_surface.init( g_csgo.m_surface );
	//m_surface.add( ISurface::GETSCREENSIZE, util::force_cast( &Hooks::GetScreenSize ) );
	m_surface.add( ISurface::LOCKCURSOR, util::force_cast( &Hooks::LockCursor ) );
	m_surface.add( ISurface::PLAYSOUND, util::force_cast( &Hooks::PlaySound ) );
	m_surface.add( ISurface::ONSCREENSIZECHANGED, util::force_cast( &Hooks::OnScreenSizeChanged ) );

	m_model_render.init( g_csgo.m_model_render );
	m_model_render.add( IVModelRender::DRAWMODELEXECUTE, util::force_cast( &Hooks::DrawModelExecute ) );

	m_render_view.init( g_csgo.m_render_view );
	m_render_view.add( IVRenderView::SCENEEND, util::force_cast( &Hooks::SceneEnd ) );

	m_shadow_mgr.init( g_csgo.m_shadow_mgr );
	m_shadow_mgr.add( IClientShadowMgr::COMPUTESHADOWDEPTHTEXTURES, util::force_cast( &Hooks::ComputeShadowDepthTextures ) );

	m_view_render.init( g_csgo.m_view_render );
	m_view_render.add( CViewRender::ONRENDERSTART, util::force_cast( &Hooks::OnRenderStart ) );
	m_view_render.add( CViewRender::RENDERVIEW, util::force_cast( &Hooks::RenderView ) );
	m_view_render.add( CViewRender::RENDER2DEFFECTSPOSTHUD, util::force_cast( &Hooks::Render2DEffectsPostHUD ) );
	m_view_render.add( CViewRender::RENDERSMOKEOVERLAY, util::force_cast( &Hooks::RenderSmokeOverlay ) );

	m_match_framework.init( g_csgo.m_match_framework );
	m_match_framework.add( CMatchFramework::GETMATCHSESSION, util::force_cast( &Hooks::GetMatchSession ) );

	m_material_system.init( g_csgo.m_material_system );
	m_material_system.add( IMaterialSystem::OVERRIDECONFIG, util::force_cast( &Hooks::OverrideConfig ) );

	m_fire_bullets.init( g_csgo.TEFireBullets );
	m_fire_bullets.add( 7, util::force_cast( &Hooks::PostDataUpdate ) );

	m_client_state.init( g_csgo.m_hookable_cl );
	m_client_state.add( CClientState::TEMPENTITIES, util::force_cast( &Hooks::TempEntities ) );

	// register our custom entity listener.
	// todo - dex; should we push our listeners first? should be fine like this.
	g_custom_entity_listener.init( );

	// cvar hooks.
	m_debug_spread.init( g_csgo.weapon_debug_spread_show );
	m_debug_spread.add( ConVar::GETINT, util::force_cast( &Hooks::DebugSpreadGetInt ) );

	m_net_show_fragments.init( g_csgo.net_showfragments );
	m_net_show_fragments.add( ConVar::GETBOOL, util::force_cast( &Hooks::NetShowFragmentsGetBool ) );

	// set netvar proxies.
	g_netvars.SetProxy(HASH("DT_CSPlayer"), HASH("m_flSimulationTime"), ReceivedSimulationTime, m_SimulationTime_original);
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_angEyeAngles[0]" ), Pitch_proxy, m_Pitch_original );
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_flLowerBodyYawTarget" ), Body_proxy, m_Body_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_vecForce" ), Force_proxy, m_Force_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_flAbsYaw" ), AbsYaw_proxy, m_AbsYaw_original );
}