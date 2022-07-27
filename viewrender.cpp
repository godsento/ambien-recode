#include "includes.h"

void Hooks::OnRenderStart( ) {
	// call og.

	if (!g_cl.m_processing || !g_csgo.m_engine->IsConnected() || !g_csgo.m_engine->IsInGame())
		return g_hooks.m_view_render.GetOldMethod< OnRenderStart_t >(CViewRender::ONRENDERSTART)(this);

	g_hooks.m_view_render.GetOldMethod< OnRenderStart_t >( CViewRender::ONRENDERSTART )( this );

	if (g_cl.m_local->GetActiveWeapon()->m_zoomLevel() == 2)
		g_csgo.m_view_render->m_view.m_fov = g_menu.main.skins.fov_amt.get() * (g_menu.main.skins.second_scoped_fov.get() / 100);
	else if (g_cl.m_local->GetActiveWeapon()->m_zoomLevel() == 1)
		g_csgo.m_view_render->m_view.m_fov = g_menu.main.skins.fov_amt.get() * (g_menu.main.skins.scoped_fov.get() / 100);
	else
		g_csgo.m_view_render->m_view.m_fov = g_menu.main.skins.fov_amt.get();

	g_csgo.m_view_render->m_view.m_viewmodel_fov = g_menu.main.skins.viewmodel_fov_amt.get();
}

void Hooks::RenderView( const CViewSetup &view, const CViewSetup &hud_view, int clear_flags, int what_to_draw ) {
	// ...

	g_hooks.m_view_render.GetOldMethod< RenderView_t >( CViewRender::RENDERVIEW )( this, view, hud_view, clear_flags, what_to_draw );
}

void Hooks::Render2DEffectsPostHUD( const CViewSetup &setup ) {
	if( !g_menu.main.visuals.removals.get(3) )
		g_hooks.m_view_render.GetOldMethod< Render2DEffectsPostHUD_t >( CViewRender::RENDER2DEFFECTSPOSTHUD )( this, setup );
}

void Hooks::RenderSmokeOverlay( bool unk ) {
	// do not render smoke overlay.
	if( !g_menu.main.visuals.removals.get(1) )
		g_hooks.m_view_render.GetOldMethod< RenderSmokeOverlay_t >( CViewRender::RENDERSMOKEOVERLAY )( this, unk );
}
