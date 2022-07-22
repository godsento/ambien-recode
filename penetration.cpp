#include "includes.h"

bool penetration::IsArmored(Player* player, int nHitgroup) {
	const bool bHasHelmet = player->m_bHasHelmet();
	const bool bHasHeavyArmor = player->m_bHasHeavyArmor();
	const float flArmorValue = player->m_ArmorValue();

	if (flArmorValue > 0) {
		switch (nHitgroup) {
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			return true;
			break;
		case HITGROUP_HEAD:
			return bHasHelmet || bHasHeavyArmor;
			break;
		default:
			return bHasHeavyArmor;
			break;
		}
	}

	return false;
}

bool CGameTrace::DidHitWorld()  {
	return m_entity == g_csgo.m_entlist->GetClientEntity(0);
}
bool CGameTrace::DidHitNonWorldEntity() {
	return m_entity != nullptr && !DidHitWorld();
}

float penetration::scale( Player* player, float damage, float armor_ratio, int hitgroup ) {

	if (!player)
		return -1.f;

	if (!g_cl.m_processing)
		return -1.f;

	if (!g_cl.m_weapon)
		return -1.f;

	if (!g_cl.m_weapon_info)
		return -1.f;

	float flDamage = damage;
	const int nTeamNum = player->m_iTeamNum();
	float flHeadDamageScale = nTeamNum == 3 ? g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_ct_head"))->GetFloat() : g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_t_head"))->GetFloat();
	const float flBodyDamageScale = nTeamNum == 3 ? g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_ct_body"))->GetFloat() : g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_t_body"))->GetFloat();

	const bool bIsArmored = IsArmored(player, hitgroup);
	const bool bHasHeavyArmor = player->m_bHasHeavyArmor();
	const bool bIsZeus = g_cl.m_weapon_id == ZEUS;

	const float flArmorValue = static_cast<float>(player->m_ArmorValue());

	if (bHasHeavyArmor)
		flHeadDamageScale /= 2.f;

	if (!bIsZeus) {
		switch (hitgroup) {
		case HITGROUP_HEAD:
			flDamage = (flDamage * 4.f) * flHeadDamageScale;
			break;
		case HITGROUP_STOMACH:
			flDamage = (flDamage * 1.25f) * flBodyDamageScale;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage = (flDamage * 0.75f) * flBodyDamageScale;
			break;
		default:
			break;
		}
	}

	// enemy have armor
	if (bIsArmored) {
		float flArmorScale = 1.f;
		float flArmorBonusRatio = 0.5f;
		float flArmorRatioCalculated = armor_ratio * 0.5f;
		float fDamageToHealth = 0.f;

		if (bHasHeavyArmor) {
			flArmorRatioCalculated = armor_ratio * 0.25f;
			flArmorBonusRatio = 0.33f;

			flArmorScale = 0.33f;

			fDamageToHealth = (flDamage * flArmorRatioCalculated) * 0.85f;
		}
		else {
			fDamageToHealth = flDamage * flArmorRatioCalculated;
		}

		float fDamageToArmor = (flDamage - fDamageToHealth) * (flArmorScale * flArmorBonusRatio);

		// Does this use more armor than we have?
		if (fDamageToArmor > flArmorValue)
			fDamageToHealth = flDamage - (flArmorValue / flArmorBonusRatio);

		flDamage = fDamageToHealth;
	}

	return std::floor(flDamage);
}

void penetration::TraceLine(const vec3_t& start, const vec3_t& end, uint32_t mask, ITraceFilter* ignore, CGameTrace* ptr) {
	g_csgo.m_engine_trace->TraceRay(Ray(start, end), mask, ignore, ptr);
}

void penetration::ClipTraceToPlayer(const vec3_t vecAbsStart, const vec3_t& vecAbsEnd, uint32_t iMask, ITraceFilter* pFilter, CGameTrace* pGameTrace, Player* player) {
	constexpr float flMaxRange = 60.0f, flMinRange = 0.0f;

	// get bounding box
	const vec3_t vecObbMins = player->m_vecMins();
	const vec3_t vecObbMaxs = player->m_vecMaxs();
	const vec3_t vecObbCenter = (vecObbMaxs + vecObbMins) / 2.f;

	// calculate world space center
	const vec3_t vecPosition = vecObbCenter + player->m_vecOrigin();

	Ray Ray(vecAbsStart, vecAbsEnd);

	const vec3_t vecTo = vecPosition - vecAbsStart;
	vec3_t vecDirection = vecAbsEnd - vecAbsStart;
	const float flLength = vecDirection.normalize();

	// YOU DONT NEED TO MAKE THIS A TRANNY CODED FUNCTION!!!!!!
	const float flRangeAlong = vecDirection.dot(vecTo);
	float flRange = 0.0f;

	// calculate distance to ray
	if (flRangeAlong < 0.0f)
		// off start point
		flRange = -vecTo.length();
	else if (flRangeAlong > flLength)
		// off end point
		flRange = -(vecPosition - vecAbsEnd).length();
	else
		// within ray bounds
		flRange = (vecPosition - (vecDirection * flRangeAlong + vecAbsStart)).length();

	if (flRange < 0.0f || flRange > 60.0f)
		return;

	CGameTrace playerTrace;
	g_csgo.m_engine_trace->ClipRayToEntity(Ray, iMask | CONTENTS_HITBOX, player, &playerTrace);

	if (pGameTrace->m_fraction > playerTrace.m_fraction)
		*pGameTrace = playerTrace;
}

void penetration::ClipTraceToPlayers(const vec3_t& vecAbsStart, const vec3_t& vecAbsEnd, uint32_t iMask, ITraceFilter* pFilter, CGameTrace* pGameTrace, float flMaxRange, float flMinRange) {
	float flSmallestFraction = pGameTrace->m_fraction;

	vec3_t vecDelta(vecAbsEnd - vecAbsStart);
	const float flDelta = vecDelta.normalize();

	Ray Ray(vecAbsStart, vecAbsEnd);

	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* pPlayer = g_csgo.m_entlist->GetClientEntity< Player* >(i);
		if (!pPlayer || pPlayer->dormant() || !pPlayer->alive())
			continue;

		if (pFilter && !pFilter->ShouldHitEntity(pPlayer, iMask))
			continue;

		// get bounding box
		const vec3_t vecObbMins = pPlayer->m_vecMins();
		const vec3_t vecObbMaxs = pPlayer->m_vecMaxs();
		const vec3_t vecObbCenter = (vecObbMaxs + vecObbMins) / 2.f;

		// calculate world space center
		const vec3_t vecPosition = vecObbCenter + pPlayer->m_vecOrigin();

		const vec3_t vecTo = vecPosition - vecAbsStart;
		vec3_t vecDirection = vecAbsEnd - vecAbsStart;
		const float flLength = vecDirection.normalize();

		// YOU DONT NEED TO MAKE THIS A TRANNY CODED FUNCTION!!!!!!
		const float flRangeAlong = vecDirection.dot(vecTo);
		float flRange = 0.0f;

		// calculate distance to ray
		if (flRangeAlong < 0.0f)
			// off start point
			flRange = -vecTo.length();
		else if (flRangeAlong > flLength)
			// off end point
			flRange = -(vecPosition - vecAbsEnd).length();
		else
			// within ray bounds
			flRange = (vecPosition - (vecDirection * flRangeAlong + vecAbsStart)).length();

		if (flRange < flMinRange || flRange > flMaxRange)
			return;

		CGameTrace playerTrace;
		g_csgo.m_engine_trace->ClipRayToEntity(Ray, iMask | CONTENTS_HITBOX, pPlayer, &playerTrace);
		if (playerTrace.m_fraction < flSmallestFraction) {
			// we shortened the ray - save off the trace
			*pGameTrace = playerTrace;
			flSmallestFraction = playerTrace.m_fraction;
		}
	}
}

bool penetration::TraceToExit2(CGameTrace* pEnterTrace, vec3_t vecStartPos, vec3_t vecDirection, CGameTrace* pExitTrace) {
	constexpr float flMaxDistance = 90.f, flStepSize = 4.f;
	static CTraceFilterSimple_game filter{};
	float flCurrentDistance = 0.f;

	int iFirstContents = 0;

	bool bIsWindow = 0;
	auto v23 = 0;

	do {
		// Add extra distance to our ray
		flCurrentDistance += flStepSize;

		// Multiply the direction vec3_t to the distance so we go outwards, add our position to it.
		vec3_t vecEnd = vecStartPos + (vecDirection * flCurrentDistance);

		if (!iFirstContents)
			iFirstContents = g_csgo.m_engine_trace->GetPointContents(vecEnd, (MASK_SHOT_HULL | CONTENTS_HITBOX));

		int iPointContents = g_csgo.m_engine_trace->GetPointContents(vecEnd, (MASK_SHOT_HULL | CONTENTS_HITBOX));

		if (!(iPointContents & MASK_SHOT_HULL) || ((iPointContents & CONTENTS_HITBOX) && iPointContents != iFirstContents)) {
			//Let's setup our end position by deducting the direction by the extra added distance
			vec3_t vecStart = vecEnd - (vecDirection * flStepSize);

			// this gets a bit more complicated and expensive when we have to deal with displacements
			TraceLine(vecEnd, vecStart, MASK_SHOT, nullptr, pExitTrace); // was MASK_SHOT_HULL should just be MASK_SHOT

			// we hit an ent's hitbox, do another trace.
			if (pExitTrace->m_startsolid && pExitTrace->m_surface.m_flags & SURF_HITBOX) {
				filter.SetPassEntity(pExitTrace->m_entity);

				// do another trace, but skip the player to get the actual exit surface 
				TraceLine(vecStartPos, vecStart, MASK_SHOT_HULL, (ITraceFilter*)&filter, pExitTrace);

				if (pExitTrace->hit() && !pExitTrace->m_startsolid) {
					vecEnd = pExitTrace->m_endpos;
					return true;
				}

				continue;
			}

			//Can we hit? Is the wall solid?
			if (pExitTrace->hit() && !pExitTrace->m_startsolid) {
				if (game::IsBreakable(pEnterTrace->m_entity) && game::IsBreakable(pEnterTrace->m_entity))
					return true;

				if (pEnterTrace->m_surface.m_flags & SURF_NODRAW ||
					(!(pExitTrace->m_surface.m_flags & SURF_NODRAW) && pExitTrace->m_plane.m_normal.dot(vecDirection) <= 1.f)) {
					const float flMultAmount = pExitTrace->m_fraction * 4.f;

					// get the real end pos
					vecStart -= vecDirection * flMultAmount;
					return true;
				}

				continue;
			}

			if (!pExitTrace->hit() || pExitTrace->m_startsolid) {
				if (pEnterTrace->DidHitNonWorldEntity() && game::IsBreakable(pEnterTrace->m_entity)) {
					// if we hit a breakable, make the assumption that we broke it if we can't find an exit (hopefully..)
					// fake the end pos
					pExitTrace = pEnterTrace;
					pExitTrace->m_endpos = vecStartPos + vecDirection;
					return true;
				}
			}
		}
		// max pen distance is 90 units.
	} while (flCurrentDistance <= flMaxDistance);

	return false;
}


bool penetration::run( PenetrationInput_t* in, PenetrationOutput_t* out ) {
    static CTraceFilterSkipTwoEntities_game filter{};

	int			  pen{ 4 }, enter_material, exit_material;
	float		  damage, penetration, penetration_mod, player_damage, remaining, trace_len{}, total_pen_mod, damage_mod, modifier, damage_lost;
	surfacedata_t *enter_surface, *exit_surface;
	bool		  nodraw, grate;
	vec3_t		  start, dir, end, pen_end;
	CGameTrace	  trace, exit_trace;
	Weapon		  *weapon;
	WeaponInfo    *weapon_info;

	// if we are tracing from our local player perspective.
	if( in->m_from->m_bIsLocalPlayer( ) ) {
		weapon      = g_cl.m_weapon;
		weapon_info = g_cl.m_weapon_info;
		start       = g_cl.m_shoot_pos;
	}

	// not local player.
	else {
		weapon = in->m_from->GetActiveWeapon( );
		if( !weapon )
			return false;

		// get weapon info.
		weapon_info = weapon->GetWpnData( );
		if( !weapon_info )
			return false;

		// set trace start.
		start = in->m_from->GetShootPosition( );
	}

	// get some weapon data.
	damage      = static_cast<float>(weapon_info->m_damage);
	penetration = weapon_info->m_penetration;

    // used later in calculations.
    penetration_mod = std::max( 0.f, ( 3.f / penetration ) * 1.25f );

	// get direction to end point.
	dir = ( in->m_pos - start ).normalized( );

    // setup trace filter for later.
    filter.SetPassEntity( in->m_from );
    filter.SetPassEntity2( nullptr );

    while( damage > 0.f && pen > 0 ) {
		// calculating remaining len.
		remaining = weapon_info->m_range - trace_len;

		// set trace end.
		end = start + ( dir * remaining );

		// setup ray and trace.
		TraceLine( start, end, MASK_SHOT, ( ITraceFilter* )&filter, &trace );

		// check for player hitboxes extending outside their collision bounds.
		// if no target is passed we clip the trace to a specific player, otherwise we clip the trace to any player.
		if (in->m_target)
			ClipTraceToPlayer( start, end + ( dir * 40.f ), MASK_SHOT_HULL | CONTENTS_HITBOX, ( ITraceFilter* )&filter, &trace, in->m_target );
		else
			ClipTraceToPlayers( start, end + ( dir * 40.f ), MASK_SHOT_HULL | CONTENTS_HITBOX, ( ITraceFilter* )&filter, &trace, 60.f, 0 );

		// we didn't hit anything.
		if( trace.m_fraction == 1.f )
			return false;

		// calculate damage based on the distance the bullet traveled.
		trace_len += trace.m_fraction * remaining;
		damage *= powf(weapon_info->m_range_modifier, trace_len * 0.002f);

		// if a target was passed.
		if( in->m_target ) {

			// validate that we hit the target we aimed for.
			if( trace.m_entity && trace.m_entity == in->m_target && game::IsValidHitgroup( trace.m_hitgroup ) ) {
				int group = ( weapon->m_iItemDefinitionIndex( ) == ZEUS ) ? HITGROUP_GENERIC : trace.m_hitgroup;

				// scale damage based on the hitgroup we hit.
				player_damage = scale( in->m_target, damage, weapon_info->m_armor_ratio, group );

				if (player_damage < 1.f)
					return false;

				// set result data for when we hit a player.
			    out->m_pen      = pen != 4;
			    out->m_hitgroup = group;
			    out->m_damage   = (int)std::round(player_damage);
			    out->m_target   = in->m_target;
				

				bool lethal = (g_menu.main.aimbot.scale_dmg.get() || weapon->m_iItemDefinitionIndex() == ZEUS) && player_damage >= in->m_target->m_iHealth();

				// non-penetrate damage.
				if( pen == 4 )
					return lethal || player_damage >= in->m_damage;
					
				// penetration damage.
				return lethal || player_damage >= in->m_damage_pen;
			}
		}

		// no target was passed, check for any player hit or just get final damage done.
		else {
			out->m_pen = pen != 4;

			// todo - dex; team checks / other checks / etc.
			if( trace.m_entity && trace.m_entity->IsPlayer( ) && game::IsValidHitgroup( trace.m_hitgroup ) ) {
				int group = ( weapon->m_iItemDefinitionIndex( ) == ZEUS ) ? HITGROUP_GENERIC : trace.m_hitgroup;

				player_damage = scale( trace.m_entity->as< Player* >( ), damage, weapon_info->m_armor_ratio, group );

				// set result data for when we hit a player.
				out->m_hitgroup = group;
				out->m_damage   = player_damage;
				out->m_target   = trace.m_entity->as< Player* >( );

				// non-penetrate damage.
				if (pen == 4)
					return player_damage >= trace.m_entity->as< Player* >()->m_iHealth() || player_damage >= in->m_damage;

				// penetration damage.
				return player_damage >= trace.m_entity->as< Player* >()->m_iHealth() || player_damage >= in->m_damage_pen;
			}

            // if we've reached here then we didn't hit a player yet, set damage and hitgroup.
            out->m_damage = damage;
		}

		// don't run pen code if it's not wanted.
		if( !in->m_can_pen )
			return false;

		// get surface at entry point.
		enter_surface = g_csgo.m_phys_props->GetSurfaceData( trace.m_surface.m_surface_props );

		// this happens when we're too far away from a surface and can penetrate walls or the surface's pen modifier is too low.
		if ((trace_len > 3000.f && penetration) || enter_surface->m_game.m_penetration_modifier < 0.1f) {
			pen = 0;
			return false;
		}

		// store data about surface flags / contents.
		nodraw = ( trace.m_surface.m_flags & SURF_NODRAW );
		grate  = ( trace.m_contents & CONTENTS_GRATE );

		// get material at entry point.
		enter_material = enter_surface->m_game.m_material;

		// note - dex; some extra stuff the game does.
		if( !pen && !nodraw && !grate && enter_material != CHAR_TEX_GRATE && enter_material != CHAR_TEX_GLASS )
			return false;

		// no more pen.
		if( penetration <= 0.f || pen <= 0 )
			return false;

		// try to penetrate object.
		if ( !TraceToExit2( &trace, trace.m_endpos, dir, &exit_trace ) ) {
			if( !( g_csgo.m_engine_trace->GetPointContents( pen_end, MASK_SHOT_HULL ) & MASK_SHOT_HULL ) )
				return false;
		}

		// get surface / material at exit point.
		// note, i tried to fill up the "TODO" from dex but it just rapes fps so i removed it
		exit_surface  = g_csgo.m_phys_props->GetSurfaceData( exit_trace.m_surface.m_surface_props );
        exit_material = exit_surface->m_game.m_material;

		if (enter_material == CHAR_TEX_GRATE || enter_material == CHAR_TEX_GLASS) {
			total_pen_mod = 3.f;
			damage_mod = 0.05f;
		}

		else if (nodraw || grate) {
			total_pen_mod = 1.f;
			damage_mod = 0.16f;
		}

		else {
			total_pen_mod = (enter_surface->m_game.m_penetration_modifier + exit_surface->m_game.m_penetration_modifier) * 0.5f;
			damage_mod = 0.16f;
		}

		// thin metals, wood and plastic get a penetration bonus.
		if (enter_material == exit_material) {
			if (exit_material == CHAR_TEX_CARDBOARD || exit_material == CHAR_TEX_WOOD)
				total_pen_mod = 3.f;

			else if (exit_material == CHAR_TEX_PLASTIC)
				total_pen_mod = 2.f;
		}

		const float flTraceDistance = (exit_trace.m_endpos - trace.m_endpos).length();
		const float flPenetrationMod = fmaxf(1.0 / total_pen_mod, 0.0f);
		const float flTotalLostDamage = (fmaxf(3.f / penetration, 0.f) *
			(flPenetrationMod * 3.f) + (damage * damage_mod)) +
			(((flTraceDistance * flTraceDistance) * flPenetrationMod) / 24);

		const float flClampedLostDamage = fmaxf(flTotalLostDamage, 0.f);

		if (flClampedLostDamage > damage)
			return false;

		// reduce damage power each time we hit something other than a grate
		if (flClampedLostDamage > 0.0f)
			damage -= flClampedLostDamage;

		// do we still have enough damage to deal?
		if (damage < 2.0f)
			return false;

		// penetration was successful
		// setup new start end parameters for successive trace
		start = exit_trace.m_endpos;

		// decrement pen.
		--pen;
	}

	return false;
}


