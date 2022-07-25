#include "includes.h"

Chams g_chams{ };;

Chams::model_type_t Chams::GetModelType(const ModelRenderInfo_t& info) {
	// model name.
	//const char* mdl = info.m_model->m_name;

	std::string mdl{ info.m_model->m_name };

	//static auto int_from_chars = [ mdl ]( size_t index ) {
	//	return *( int* )( mdl + index );
	//};

	// little endian.
	//if( int_from_chars( 7 ) == 'paew' ) { // weap
	//	if( int_from_chars( 15 ) == 'om_v' && int_from_chars( 19 ) == 'sled' )
	//		return model_type_t::arms;
	//
	//	if( mdl[ 15 ] == 'v' )
	//		return model_type_t::view_weapon;
	//}

	//else if( int_from_chars( 7 ) == 'yalp' ) // play
	//	return model_type_t::player;

	if (mdl.find(XOR("player")) != std::string::npos && info.m_index >= 1 && info.m_index <= 64)
		return model_type_t::player;

	return model_type_t::invalid;
}

bool Chams::IsInViewPlane(const vec3_t& world) {
	float w;

	const VMatrix& matrix = g_csgo.m_engine->WorldToScreenMatrix();

	w = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];

	return w > 0.001f;
}

void Chams::SetColor(Color col, IMaterial* mat) {

	if (mat) {
		mat->ColorModulate(col);

		auto pVar = mat->find_var("$envmaptint");
		if (pVar)
			(*(void(__thiscall**)(int, float, float, float))(*(DWORD*)pVar + 44))((uintptr_t)pVar, col.r() / 255.f, col.g() / 255.f, col.b() / 255.f);
	}
	else
		g_csgo.m_render_view->SetColorModulation(col);
}


void Chams::SetAlpha(float alpha, IMaterial* mat) {
	if (mat)
		mat->AlphaModulate(alpha);

	else
		g_csgo.m_render_view->SetBlend(alpha);
}

void Chams::SetupMaterial(IMaterial* mat, Color col, bool z_flag) {
	SetColor(col, mat);

	/*
	if (mat == debugdrawflat) {
		mat->SetFlag(MATERIAL_VAR_WIREFRAME, true);
	}*/

	// mat->SetFlag( MATERIAL_VAR_HALFLAMBERT, true );

	if (mat != animated_shit3) {
		mat->SetFlag(MATERIAL_VAR_ZNEARER, z_flag);
		mat->SetFlag(MATERIAL_VAR_NOFOG, z_flag);
		mat->SetFlag(MATERIAL_VAR_IGNOREZ, z_flag);
	}

	g_csgo.m_studio_render->ForcedMaterialOverride(mat);
}

void Chams::init() {
	std::ofstream("csgo\\materials\\simple_flat.vmt") << R"#("UnlitGeneric"
{
  "$basetexture" "vgui/white_additive"
  "$ignorez"      "1"
  "$envmap"       ""
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";
	std::ofstream("csgo\\materials\\simple_ignorez_reflective.vmt") << R"#("VertexLitGeneric"
{

  "$basetexture" "vgui/white_additive"
  "$ignorez"      "1"
  "$envmap"       "env_cubemap"
  "$normalmapalphaenvmapmask"  "1"
  "$envmapcontrast"             "1"
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";

	std::ofstream("csgo\\materials\\simple_regular_reflective.vmt") << R"#("VertexLitGeneric"
{

  "$basetexture" "vgui/white_additive"
  "$ignorez"      "0"
  "$envmap"       "env_cubemap"
  "$normalmapalphaenvmapmask"  "1"
  "$envmapcontrast"             "1"
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";

	std::ofstream("csgo/materials/chams.vmt") << R"#("VertexLitGeneric"
{
	  "$basetexture" "vgui/white_additive"
	  "$ignorez" "0"
	  "$additive" "0"
	  "$envmap"  "models/effects/cube_white"
	  "$normalmapalphaenvmapmask" "1"
	  "$envmaptint" "[0.37 0.68 0.89]"
	  "$envmapfresnel" "1"
	  "$envmapfresnelminmaxexp" "[0 1 2]"

	  "$envmapcontrast" "1"
	  "$nofog" "1"
	  "$model" "1"
	  "$nocull" "0"
	  "$selfillum" "1"
	  "$halflambert" "1"
	  "$znearer" "0"
	  "$flat" "1"
}
)#";

	std::ofstream("csgo/materials/Overlay.vmt") << R"#(" VertexLitGeneric "{

			"$additive" "1"
	        "$envmap"  "models/effects/cube_white"
			"$envmaptint" "[0 0 0]"
			"$envmapfresnel" "1"
			"$envmapfresnelminmaxexp" "[0 16 12]"
			"$alpha" "0.8"
})#";

	std::ofstream("csgo/materials/glowOverlay.vmt") << R"#("VertexLitGeneric" {
				"$additive" "1"
				"$envmap" "models/effects/cube_white"
				"$envmaptint" "[0 0.1 0.2]"
				"$envmapfresnel" "1"
				"$envmapfresnelminmaxexp" "[0 1 2]"
				"$ignorez" "1"
				"$alpha" "1"
			})#";

	std::ofstream("csgo/materials/cubemap.vmt") << R"#("VertexLitGeneric" { 

  "$basetexture" "vgui/white_additive"
  "$ignorez"      "1"
  "$envmap"       "env_cubemap"
  "$envmaptint"   "[0.6 0.6 0.6]"
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
	})#";

	std::ofstream("csgo\\materials\\pdr_animated_shit.vmt") <<
		R"#("VertexLitGeneric"
			{
			  "$additive" "1"
			  "$envmaptint" "[1 1 1]"
			  "$basetexture" "sprites/light_glow04"
			  "$wireframe" "1"
    "$selfillum"    "1"
    "$znearer"      "0"
    "$nofog"        "1"
    "$model"        "1"
    "$nocull"       "0"
              "$flat"         "1"
			  "Proxies"
				{
						"TextureScroll"
						{
								"texturescrollvar" "$BasetextureTransform"
								"texturescrollrate" "0.8"
								"texturescrollangle" "70"
						}
				}
			}
			)#";

	std::ofstream("csgo\\materials\\pdr_animated_shit2.vmt") <<
		R"#("VertexLitGeneric"
			{
			 	"$basetexture" "models/inventory_items/dreamhack_trophies/dreamhack_star_blur"
	"$wireframe" "1"
	"$envmaptint" "[1 1 1]"
	"$alpha" "1"
	"$additive" "1"
    "$selfillum"    "1"
    "$znearer"      "0"
    "$flat"         "1"
    "$nofog"        "1"
    "$model"        "1"
    "$nocull"       "0"
	"proxies"
     {
		"texturescroll"
        {
            "texturescrollvar" "$basetexturetransform"
            "texturescrollrate" "0.2"
            "texturescrollangle" "90"
        }
    }
			}
			)#";

	std::ofstream("csgo\\materials\\pdr_animated_shit3.vmt") <<
		R"#("VertexLitGeneric"
			{
			 	"$basetexture" "models/inventory_items/dreamhack_trophies/dreamhack_star_blur"
	"$additive" "1"
    "$selfillum"    "1"
	"$wireframe" "1"
	"$alpha" "2"


	"proxies"
     {
		"texturescroll"
        {
            "texturescrollvar" "$basetexturetransform"
            "texturescrollrate" "0.4"
            "texturescrollangle" "90"
        }
    }
			}
			)#";

	std::ofstream("csgo\\materials\\promethea_glowoverlay.vmt") << R"#("VertexLitGeneric" 
		{
		"$additive" "1"
		"$envmap" "models/effects/cube_white"
		"$envmaptint" "[1 1 1]"
		"$envmapfresnel" "1"
		"$envmapfresnelminmaxexp" "[0 1 2]"
		"$alpha" "0.8"
		})#";

	std::ofstream("csgo\\materials\\promethea_glowoverlay1.vmt") << R"#("VertexLitGeneric" 
		{
		"$additive" "1"
		"$envmap" "models/effects/cube_white"
		"$envmaptint" "[1 1 1]"
		"$envmapfresnel" "1"
		"$envmapfresnelminmaxexp" "[0 1 16]"
		"$alpha" "0.8"
		})#";

	materialMetall = g_csgo.m_material_system->FindMaterial("simple_ignorez_reflective", "Model textures");
	materialMetall->IncrementReferenceCount();

	materialMetallnZ = g_csgo.m_material_system->FindMaterial("simple_regular_reflective", "Model textures");
	materialMetallnZ->IncrementReferenceCount();

	// find stupid materials.
	debugambientcube = g_csgo.m_material_system->FindMaterial(XOR("debug/debugambientcube"), XOR("Model textures"));
	debugambientcube->IncrementReferenceCount();

	debugdrawflat = g_csgo.m_material_system->FindMaterial(XOR("debug/debugdrawflat"), XOR("Model textures"));
	debugdrawflat->IncrementReferenceCount();

	materialMetall3 = g_csgo.m_material_system->FindMaterial(XOR("cubemap"), XOR("Model textures"));
	materialMetall3->IncrementReferenceCount();

	onetap = g_csgo.m_material_system->FindMaterial(XOR("Overlay"), XOR("Model textures"));
	onetap->IncrementReferenceCount();

	animated_shit = g_csgo.m_material_system->FindMaterial(XOR("pdr_animated_shit"), XOR("Model textures"));

	animated_shit->IncrementReferenceCount();

	animated_shit2 = g_csgo.m_material_system->FindMaterial(XOR("pdr_animated_shit2"), XOR("Model textures"));
	animated_shit2->IncrementReferenceCount();

	animated_shit3 = g_csgo.m_material_system->FindMaterial(XOR("pdr_animated_shit3"), XOR("Model textures"));

	animated_shit3->IncrementReferenceCount();

	promethea_glow = g_csgo.m_material_system->FindMaterial(XOR("promethea_glowoverlay1"), nullptr);
	promethea_glow->IncrementReferenceCount();


	skeet = g_csgo.m_material_system->FindMaterial(XOR("promethea_glowoverlay"), nullptr);
	skeet->IncrementReferenceCount();

}




bool Chams::OverridePlayer(int index) {
	Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(index);
	if (!player)
		return false;

	// always skip the local player in DrawModelExecute.
	// this is because if we want to make the local player have less alpha
	// the static props are drawn after the players and it looks like aids.
	// therefore always process the local player in scene end.
	if (player->m_bIsLocalPlayer())
		return true;

	// see if this player is an enemy to us.
	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local);

	// we have chams on enemies.
	if (enemy && g_menu.main.players.chams_enemy.get(0))
		return true;

	// we have chams on friendly.
	else if (!enemy && g_menu.main.players.chams_friendly.get(0))
		return true;

	return false;
}

bool Chams::GenerateLerpedMatrix(int index, BoneArray* out) {
	LagRecord* current_record;
	AimPlayer* data;

	return false;

	Player* ent = g_csgo.m_entlist->GetClientEntity< Player* >(index);
	if (!ent)
		return false;

	if (!g_aimbot.IsValidTarget(ent))
		return false;

	data = &g_aimbot.m_players[index - 1];
	if (!data || data->m_records.empty())
		return false;

	if (data->m_records.size() < 2)
		return false;

	auto* channel_info = g_csgo.m_engine->GetNetChannelInfo();
	if (!channel_info)
		return false;

	static float max_unlag = 0.2f;
	static auto sv_maxunlag = g_csgo.m_cvar->FindVar(HASH("sv_maxunlag"));
	if (sv_maxunlag) {
		max_unlag = sv_maxunlag->GetFloat();
	}

	for (auto it = data->m_records.rbegin(); it != data->m_records.rend(); it++) {
		current_record = it->get();

		bool end = it + 1 == data->m_records.rend();

		if (current_record && current_record->valid() && (!end && ((it + 1)->get()))) {


			auto LastInvalid = (it + 1)->get();
			auto FirstInvalid = current_record;


			if (!LastInvalid || !FirstInvalid)
				continue;

			if (LastInvalid->m_sim_time - FirstInvalid->m_sim_time >= 1.f)
				continue;




			const auto NextOrigin = LastInvalid->m_origin;
			const auto m_curtime = g_csgo.m_globals->m_curtime;

			auto flDelta = 1.f - (m_curtime - LastInvalid->m_interp_time) / (LastInvalid->m_sim_time - current_record->m_sim_time);
			if (flDelta < 0.f || flDelta > 1.f)
				LastInvalid->m_interp_time = m_curtime;

			flDelta = 1.f - (m_curtime - LastInvalid->m_interp_time) / (LastInvalid->m_sim_time - current_record->m_sim_time);

			vec3_t lerp = math::Interpolate(NextOrigin, current_record->m_origin, std::clamp(flDelta, 0.f, 1.f));

			matrix3x4_t ret[128];

			std::memcpy(ret,
				current_record->m_bones,
				sizeof(ret));

			for (size_t i{ 0 }; i < 128; ++i) {
				vec3_t matrix_delta = current_record->m_bones[i].GetOrigin() - current_record->m_origin;
				ret[i].SetOrigin(matrix_delta + lerp);
			}

			std::memcpy(out,
				ret,
				sizeof(ret));

			return true;
		}
	}

	return false;
}

void Chams::RenderHistoryChams(int index) {

	Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(index);
	if (!player)
		return;

	if (!g_aimbot.IsValidTarget(player))
		return;

	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local);
	if (enemy) {
		auto data = &g_aimbot.m_players[index - 1];
		if (!data)
			return;

		if (data->m_records.empty())
			return;


		if (data->m_records.front().get()->dormant())
			return;


		if (data->m_records.front().get()->m_broke_lc)
			return;

		if (data->m_records.size() <= 1)
			return;

		// override blend.
		SetAlpha(g_menu.main.players.chams_enemy_history_blend.get() / 100.f);


		auto matematerial = debugambientcube;

		switch (g_menu.main.players.chams_enemy_history_type.get()) {
		case 0:
			break;
		case 1:
			matematerial = debugdrawflat;
			break;
		case 2:
			matematerial = materialMetallnZ;
			break;
		case 3:
			matematerial = skeet;
			break;
		case 4:
			matematerial = onetap;
			break;
		case 5:
			matematerial = materialMetall3;
			break;
		case 6:
			matematerial = promethea_glow;
			break;

		}

		// set material and color.
		SetupMaterial(matematerial, g_menu.main.players.chams_enemy_history_col.get(), true);


		auto last = data->m_records.back().get();//g_resolver.FindLastRecord(data);

		// was the matrix properly setup?
		if (last && !last->dormant()) {


			// backup the bone cache before we fuck with it.
			auto backup_bones = player->m_BoneCache().m_pCachedBones;

			// replace their bone cache with our custom one.
			player->m_BoneCache().m_pCachedBones = last->m_bones;

			// manually draw the model.
			player->DrawModel();

			// reset their bone cache to the previous one.
			player->m_BoneCache().m_pCachedBones = backup_bones;
		}
	}

}

bool Chams::DrawModel(uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone) {
	// store and validate model type.
	model_type_t type = GetModelType(info);
	if (type == model_type_t::invalid)
		return true;

	// is a valid player.
	if (type == model_type_t::player) {
		// do not cancel out our own calls from SceneEnd
		// also do not cancel out calls from the glow.
		if (!m_running && !g_csgo.m_studio_render->m_pForcedMaterial && OverridePlayer(info.m_index))
			return false;
	}

	return true;
}

void Chams::SceneEnd() {
	// store and sort ents by distance.
	if (SortPlayers()) {
		// iterate each player and render them.
		for (const auto& p : m_players)
			RenderPlayer(p);
	}

	// restore.
	g_csgo.m_studio_render->ForcedMaterialOverride(nullptr);
	g_csgo.m_render_view->SetColorModulation(colors::white);
	g_csgo.m_render_view->SetBlend(1.f);
}

void Chams::RenderPlayer(Player* player) {
	// prevent recruisive model cancelation.
	m_running = true;

	// restore.
	g_csgo.m_studio_render->ForcedMaterialOverride(nullptr);
	g_csgo.m_render_view->SetColorModulation(colors::white);
	g_csgo.m_render_view->SetBlend(1.f);

	// this is the local player.
	// we always draw the local player manually in drawmodel.
	if (player->m_bIsLocalPlayer()) {
		if (g_menu.main.players.chams_local_scope.get() && player->m_bIsScoped())
			SetAlpha(0.25f);

		else if (g_menu.main.players.chams_local.get()) {
			// override blend.
			SetAlpha(g_menu.main.players.chams_local_blend.get() / 100.f);

			auto matematerial = debugambientcube;

			switch (g_menu.main.players.localchamstype.get()) {
			case 0:
				matematerial = debugambientcube;
				break;
			case 1:
				matematerial = debugdrawflat;
				break;
			case 2:
				matematerial = materialMetallnZ;
				break;
			case 3:
				matematerial = skeet;
				break;
			case 4:
				matematerial = onetap;
				break;
			case 5:
				matematerial = materialMetall3;
				break;
			case 6:
				matematerial = promethea_glow;
				break;

			}


			SetupMaterial(matematerial, g_menu.main.players.chams_local_col.get(), false);


		}

		// manually draw the model.
		player->DrawModel();

		if (g_menu.main.players.chams_local_overlay.get()) {

			auto matematerial = animated_shit;

			switch (g_menu.main.players.overlaychamstype.get()) {
			case 0:
				break;
			case 1:
				matematerial = animated_shit2;
				break;
			case 2:
				matematerial = animated_shit3;
				break;
			case 3:
				matematerial = promethea_glow;
				break;
			case 4:
				matematerial = skeet;
				break;

			}

			SetupMaterial(matematerial, g_menu.main.players.chams_local_overlay_color.get(), false);

			player->DrawModel();
		}

	}

	// check if is an enemy.
	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local) && player->index() != g_cl.m_local->index();

	if (enemy && g_menu.main.players.chams_enemy_history.get()) {
		RenderHistoryChams(player->index());
	}

	if (enemy && g_menu.main.players.chams_enemy.get(0)) {

		auto matematerial = debugambientcube;

		switch (g_menu.main.players.enemychamstype.get()) {
		case 0:
			matematerial = debugambientcube;
			break;
		case 1:
			matematerial = debugdrawflat;
			break;
		case 2:
			matematerial = materialMetallnZ;
			break;
		case 3:
			matematerial = skeet;
			break;
		case 4:
			matematerial = onetap;
			break;
		case 5:
			matematerial = materialMetall3;
			break;
		case 6:
			matematerial = promethea_glow;
			break;
		}

		if (g_menu.main.players.chams_enemy.get(1)) {

			SetAlpha(g_menu.main.players.chams_enemy_blend.get() / 100.f);

			SetupMaterial(matematerial, g_menu.main.players.chams_enemy_invis.get(), true);

			player->DrawModel();
		}

		SetAlpha(g_menu.main.players.chams_enemy_blend.get() / 100.f);

		SetupMaterial(matematerial, g_menu.main.players.chams_enemy_vis.get(), false);

		player->DrawModel();
	}

	else if (!enemy && g_menu.main.players.chams_friendly.get(0)) {


		auto matematerial = debugambientcube;

		switch (g_menu.main.players.friendchamstype.get()) {
		case 0:
			matematerial = debugambientcube;
			break;
		case 1:
			matematerial = debugdrawflat;
			break;
		case 2:
			matematerial = materialMetallnZ;
			break;
		case 3:
			matematerial = skeet;
			break;
		case 4:
			matematerial = onetap;
			break;
		case 5:
			matematerial = materialMetall3;
			break;
		case 6:
			matematerial = promethea_glow;
			break;
		}

		if (g_menu.main.players.chams_friendly.get(1)) {

			SetAlpha(g_menu.main.players.chams_friendly_blend.get() / 100.f);

			SetupMaterial(matematerial, g_menu.main.players.chams_friendly_invis.get(), true);

			player->DrawModel();
		}

		SetAlpha(g_menu.main.players.chams_friendly_blend.get() / 100.f);

		SetupMaterial(matematerial, g_menu.main.players.chams_friendly_vis.get(), false);

		player->DrawModel();
	}

	m_running = false;
}

bool Chams::SortPlayers() {
	// lambda-callback for std::sort.
	// to sort the players based on distance to the local-player.
	static auto distance_predicate = [](Entity* a, Entity* b) {
		vec3_t local = g_cl.m_local->GetAbsOrigin();

		// note - dex; using squared length to save out on sqrt calls, we don't care about it anyway.
		float len1 = (a->GetAbsOrigin() - local).length_sqr();
		float len2 = (b->GetAbsOrigin() - local).length_sqr();

		return len1 < len2;
	};

	// reset player container.
	m_players.clear();

	// find all players that should be rendered.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		// get player ptr by idx.
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		// validate.
		if (!player || !player->IsPlayer() || !player->alive())
			continue;

		// do not draw players occluded by view plane.
		if (!IsInViewPlane(player->WorldSpaceCenter()))
			continue;

		// this player was not skipped to draw later.
		// so do not add it to our render list.
		if (!OverridePlayer(i))
			continue;

		m_players.push_back(player);
	}

	// any players?
	if (m_players.empty())
		return false;

	// sorting fixes the weird weapon on back flickers.
	// and all the other problems regarding Z-layering in this shit game.
	std::sort(m_players.begin(), m_players.end(), distance_predicate);

	return true;
}