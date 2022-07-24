#include "includes.h"

Resolver g_resolver{};;

LagRecord* Resolver::FindIdealRecord( AimPlayer* data ) {

	if( data->m_records.empty( ) )
		return nullptr;

    // iterate records.
	for( const auto &it : data->m_records ) {
		

		if( it->dormant( ) || it->immune( ) || !it->valid( ) )
			continue;

        // try to find a record with a shot, lby update, walking or no anti-aim.
		if(it->m_mode == Modes::RESOLVE_BODY || it->m_mode == Modes::RESOLVE_WALK)
            return it.get();
	}

	// none found above, return the first valid record if possible.
	return nullptr;
}

LagRecord* Resolver::FindLastRecord( AimPlayer* data ) {

	if( data->m_records.empty( ) )
		return nullptr;

	// iterate records in reverse.
	for( auto it = data->m_records.crbegin( ); it != data->m_records.crend( ); ++it ) {
		LagRecord* current = it->get( );

		// if this record is valid.
		// we are done since we iterated in reverse.
		if( current->valid( ) && !current->immune( ) && !current->dormant( ) )
			return current;
	}

	return nullptr;
}

void Resolver::OnBodyUpdate( Player* player, float value ) {

}

float Resolver::GetAwayAngle(LagRecord* record) {
	float  delta{ std::numeric_limits< float >::max() };
	vec3_t pos;
	ang_t  away;

	if (g_cl.m_net_pos.empty()) {
		math::VectorAngles(g_cl.m_local->m_vecOrigin() - record->m_pred_origin, away);
		return away.y;
	}

	float owd = (g_cl.m_latency / 2.f);

	float target = record->m_pred_time;

	// iterate all.
	for (const auto& net : g_cl.m_net_pos) {
		float dt = std::abs(target - net.m_time);

		// the best origin.
		if (dt < delta) {
			delta = dt;
			pos = net.m_pos;
		}
	}

	math::VectorAngles(pos - record->m_pred_origin, away);
	return away.y;
}


void Resolver::PredictBodyUpdates(Player* player, LagRecord* record) {

	if (!g_cl.m_processing)
		return;

	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	if (!data)
		return;

	if (data->m_records.empty())
		return;

	// inform esp that we're about to be the prediction process
	data->m_predict = true;

	//check if the player is walking
	// no need for the extra fakewalk check since we null velocity when they're fakewalking anyway
	if (record->m_anim_velocity.length_2d() > 0.1f && !record->m_fake_walk) {
		// predict the first flick they have to do after they stop moving
		data->m_body_update = player->m_flSimulationTime() + 0.22f;

		// since we are still not in the prediction process, inform the cheat that we arent predicting yet
		// this is only really used to determine if we should draw the lby timer bar on esp, no other real purpose
		data->m_predict = false;

		// stop here
		return;
	}

	if (data->m_body_index > 0)
		return;

	if (data->m_records.size() >= 2) {
		LagRecord* previous = data->m_records[1].get();

		if (previous && previous->valid()) { //&& !record->m_retard && !previous->m_retard) {
			// lby updated on this tick
			if (previous->m_body != player->m_flLowerBodyYawTarget() || player->m_flSimulationTime() >= data->m_body_update) {

				// for backtrack
				// record->m_resolved = true;

				// inform the cheat of the resolver method
				record->m_mode = RESOLVE_BODY;

				// predict the next body update
				data->m_body_update = player->m_flSimulationTime() + 1.1f;

				// set eyeangles to lby
				record->m_eye_angles.y = record->m_body;
				math::NormalizeAngle(record->m_eye_angles.y);

				player->m_angEyeAngles().y = record->m_eye_angles.y;

				record->m_resolver_mode = "flick";
			}
		}
	}
}


void Resolver::MatchShot( AimPlayer* data, LagRecord* record ) {
	// do not attempt to do this in nospread mode.
	if( g_menu.main.config.mode.get( ) == 1 )
		return;

	float shoot_time = -1.f;

	Weapon* weapon = data->m_player->GetActiveWeapon( );
	if( weapon ) {
		// with logging this time was always one tick behind.
		// so add one tick to the last shoot time.
		shoot_time = weapon->m_fLastShotTime( );
	}

	// this record has a shot on it.
	if( shoot_time != data->m_last_shoot_time && shoot_time != -1 ) {
		if( record->m_lag <= 2 )
			record->m_shot = true;
		
		// more then 1 choke, cant hit pitch, apply prev pitch.
		else if( data->m_records.size( ) >= 2 ) {
			LagRecord* previous = data->m_records[ 1 ].get( );

			if( previous && !previous->dormant( ) )
				record->m_eye_angles.x = previous->m_eye_angles.x;
		}
	}
}

void Resolver::SetMode( LagRecord* record ) {
	// the resolver has 3 modes to chose from.
	// these modes will vary more under the hood depending on what data we have about the player
	// and what kind of hack vs. hack we are playing (mm/nospread).

	float speed = record->m_anim_velocity.length( );

	// if on ground, moving, and not fakewalking.
	if( ( record->m_flags & FL_ONGROUND ) && speed > 0.1f && !record->m_fake_walk )
		record->m_mode = Modes::RESOLVE_WALK;

	// if on ground, not moving or fakewalking.
	if( ( record->m_flags & FL_ONGROUND ) && ( speed <= 0.1f || record->m_fake_walk ) )
		record->m_mode = Modes::RESOLVE_STAND;

	// if not on ground.
	else if( !( record->m_flags & FL_ONGROUND ) )
		record->m_mode = Modes::RESOLVE_AIR;
}

void Resolver::ResolveAngles( Player* player, LagRecord* record ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

	// mark this record if it contains a shot.
	MatchShot( data, record );

	// next up mark this record with a resolver mode that will be used.
	SetMode( record );

	// if we are in nospread mode, force all players pitches to down.
	// TODO; we should check thei actual pitch and up too, since those are the other 2 possible angles.
	// this should be somehow combined into some iteration that matches with the air angle iteration.
	if( g_menu.main.config.mode.get( ) == 1 )
		record->m_eye_angles.x = 90.f;

	// we arrived here we can do the acutal resolve.
	if( record->m_mode == Modes::RESOLVE_WALK ) 
		ResolveWalk( data, record );

	else if( record->m_mode == Modes::RESOLVE_STAND )
		ResolveStand( data, record );

	else if( record->m_mode == Modes::RESOLVE_AIR )
		ResolveAir( data, record );


	// fuck yall
	PredictBodyUpdates(player, record);

	// normalize the eye angles, doesn't really matter but its clean.
	math::NormalizeAngle( record->m_eye_angles.y );
}

void Resolver::ResolveWalk( AimPlayer* data, LagRecord* record ) {
	// apply lby to eyeangles.
	record->m_eye_angles.y = record->m_body;

	// reset stand and body index.		
	record->m_resolver_mode = "moving";

	if (record->m_anim_velocity.length() > 25.f) {
		data->m_last_freestand_scan = record->m_sim_time + 0.23f;
	}

	if (record->m_anim_velocity.length() > 30.f) { // niggers
		data->m_stand_index = 0;
		data->m_stand_index2 = 0;


		if (record->m_anim_velocity.length() > 100.f || record->m_lag > 7) {
			data->m_body_index = 0;
			data->m_moving_index = 0;
			record->m_resolver_mode = "moving (high)";
		}
	}

	if (data->m_moving_index > 0)
		ResolveStand(data, record);

	// copy the last record that this player was walking
	// we need it later on because it gives us crucial data.
	std::memcpy( &data->m_walk_record, record, sizeof( LagRecord ) );
}


void Resolver::anti_freestand(AimPlayer* data, LagRecord* record) {
	float away = GetAwayAngle(record);

	record->m_resolver_mode = "anti-freestand";

	if ((data->tr_right.m_fraction > 0.97f && data->tr_left.m_fraction > 0.97f) || (data->tr_left.m_fraction == data->tr_right.m_fraction) || std::abs(data->tr_left.m_fraction - data->tr_right.m_fraction) <= 0.1f) {
		record->m_eye_angles.y = away + 180.f;
		record->m_resolver_mode += "(back)";
	}
	else {
		record->m_eye_angles.y = (data->tr_left.m_fraction < data->tr_right.m_fraction) ? away - 90.f : away + 90.f;
		record->m_resolver_mode += "(side)";
	}
}



void Resolver::ResolveStand( AimPlayer* data, LagRecord* record ) {
	// for no-spread call a seperate resolver.
	if( g_menu.main.config.mode.get( ) == 1 ) {
		StandNS( data, record );
		return;
	}

	// get predicted away angle for the player.
	float away = GetAwayAngle( record );

	// pointer for easy access.
	LagRecord* move = &data->m_walk_record;

	// we have a valid moving record.
	if( move->m_sim_time > 0.f ) {
		vec3_t delta = move->m_origin - record->m_origin;

		// check if moving record is close.
		if( delta.length( ) <= 128.f ) {
			// indicate that we are using the moving lby.
			data->m_moved = true;
		}
	}

	bool activity = data->m_player->GetSequenceActivity(record->m_layers[3].m_sequence) == 980;
	float add_lby = activity  ? 0.f : 90.f;


	// a valid moving context was found
	if( data->m_moved ) {
		float diff = std::abs(math::AngleDiff( record->m_body, move->m_body ));
		float delta = record->m_anim_time - move->m_anim_time;

		// it has not been time for this first update yet.
		// activity: test
		if( (delta < 0.22f || activity) && diff < 35.f && data->m_body_index <= 0 && data->m_moving_index <= 0 ) {

			// set angles to current LBY.
			record->m_eye_angles.y = record->m_body;

			// set resolve mode.
			record->m_mode = Modes::RESOLVE_BODY;

			record->m_resolver_mode = tfm::format("pre-stand(%i:%s)", data->m_player->GetSequenceActivity(record->m_layers[3].m_sequence), std::round(delta * 100) / 100);

			// exit out of the resolver, thats it.
			return;
		}


		switch (data->m_stand_index % 5) {
		case 0:
			anti_freestand(data, record);
			break;
		case 1:
			record->m_resolver_mode = ("standing(1)");
			record->m_eye_angles.y = record->m_body + 90.f;
			break;
		case 2:
			record->m_resolver_mode = ("standing(2)");
			record->m_eye_angles.y = record->m_body - 90.f;
			break;
		case 3:  // rotated lby (+)
			record->m_resolver_mode = tfm::format("standing(3:%i)", data->m_player->GetSequenceActivity(record->m_layers[3].m_sequence));
			record->m_eye_angles.y = record->m_body - (add_lby + 45.f);
			break;
		case 4: // rotated lby (-)
			record->m_resolver_mode = tfm::format("standing(4:%i)", data->m_player->GetSequenceActivity(record->m_layers[3].m_sequence));
			record->m_eye_angles.y = record->m_body + (add_lby + 45.f);
			break;
		}

		return;
	}

	// stand2 -> no known last move.
	record->m_mode = Modes::RESOLVE_STAND2;

	switch( data->m_stand_index2 % 5 ) {

	case 0: // backward / freestand
		anti_freestand(data, record);
		break;

	case 1: // rotated lby (+)
		record->m_resolver_mode = "standing(1:1)";
		record->m_eye_angles.y = record->m_body - 90.f;
		break;

	case 2:  // rotated lby (-)
		record->m_resolver_mode = "standing(1:2)";
		record->m_eye_angles.y = record->m_body + 90.f;
		break;

	case 3:  // rotated lby (+)
		record->m_resolver_mode = tfm::format("standing(1:3:%i)", data->m_player->GetSequenceActivity(record->m_layers[3].m_sequence));
		record->m_eye_angles.y = record->m_body - (add_lby + 45.f);
		break;

	case 4: // rotated lby (-)
		record->m_resolver_mode = tfm::format("standing(1:4:%i)", data->m_player->GetSequenceActivity(record->m_layers[3].m_sequence));
		record->m_eye_angles.y = record->m_body + (add_lby + 45.f);
		break;
	}
}

void Resolver::StandNS( AimPlayer* data, LagRecord* record ) {
	// get away angles.
	float away = GetAwayAngle( record );

	switch( data->m_shots % 8 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 90.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 45.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 45.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 0.f;
		break;

	default:
		break;
	}

	// force LBY to not fuck any pose and do a true bruteforce.
	record->m_body = record->m_eye_angles.y;
}

void Resolver::ResolveAir( AimPlayer* data, LagRecord* record ) {
	// for no-spread call a seperate resolver.
	if( g_menu.main.config.mode.get( ) == 1 ) {
		AirNS( data, record );
		return;
	}

	// else run our matchmaking air resolver.

	// we have barely any speed. 
	// either we jumped in place or we just left the ground.
	// or someone is trying to fool our resolver.
	if( record->m_velocity.length_2d( ) < 60.f ) {
		// set this for completion.
		// so the shot parsing wont pick the hits / misses up.
		// and process them wrongly.
		record->m_mode = Modes::RESOLVE_STAND;

		// invoke our stand resolver.
		ResolveStand( data, record );

		record->m_resolver_mode = "air(low)";
		// we are done.
		return;
	}

	// try to predict the direction of the player based on his velocity direction.
	// this should be a rough estimation of where he is looking.
	float velyaw = math::rad_to_deg( std::atan2( record->m_velocity.y, record->m_velocity.x ) );

	record->m_resolver_mode = "air";
	switch( data->m_shots % 3 ) {
	case 0:
		record->m_eye_angles.y = velyaw + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = velyaw - 90.f;
		break;

	case 2:
		record->m_eye_angles.y = velyaw + 90.f;
		break;
	}
}

void Resolver::AirNS( AimPlayer* data, LagRecord* record ) {
	// get away angles.
	float away = GetAwayAngle( record );

	switch( data->m_shots % 9 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 150.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 150.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 165.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 165.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 8:
		record->m_eye_angles.y = away - 90.f;
		break;

	default:
		break;
	}
}

void Resolver::ResolvePoses( Player* player, LagRecord* record ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

	// only do this bs when in air.
	if( record->m_mode == Modes::RESOLVE_AIR ) {
		// ang = pose min + pose val x ( pose range )

		// lean_yaw
		player->m_flPoseParameter( )[ 2 ]  = g_csgo.RandomInt( 0, 4 ) * 0.25f;   

		// body_yaw
		player->m_flPoseParameter( )[ 11 ] = g_csgo.RandomInt( 1, 3 ) * 0.25f;
	}
}