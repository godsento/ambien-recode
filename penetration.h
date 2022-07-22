#pragma once

namespace penetration {
	struct PenetrationInput_t {
		Player* m_from;
		Player* m_target;
		vec3_t  m_pos;
		float	m_damage;
		float   m_damage_pen;
		bool	m_can_pen;
		int penetration_count;
	};

	struct PenetrationOutput_t {
		Player* m_target;
		float   m_damage;
		int     m_hitgroup;
		bool    m_pen;

        __forceinline PenetrationOutput_t() : m_target{ nullptr }, m_damage{ 0.f }, m_hitgroup{ -1 }, m_pen{ false } {}
	};

	bool IsArmored(Player* player, int nHitgroup);

	float scale( Player* player, float damage, float armor_ratio, int hitgroup );
	void TraceLine(const vec3_t& start, const vec3_t& end, uint32_t mask, ITraceFilter* ignore, CGameTrace* ptr);
	void ClipTraceToPlayer(const vec3_t vecAbsStart, const vec3_t& vecAbsEnd, uint32_t iMask, ITraceFilter* pFilter, CGameTrace* pGameTrace, Player* player);
	void ClipTraceToPlayers(const vec3_t& vecAbsStart, const vec3_t& vecAbsEnd, uint32_t iMask, ITraceFilter* pFilter, CGameTrace* pGameTrace, float flMaxRange, float flMinRange);
	bool TraceToExit2(CGameTrace* pEnterTrace, vec3_t vecStartPos, vec3_t vecDirection, CGameTrace* pExitTrace);
    bool  run( PenetrationInput_t* in, PenetrationOutput_t* out );
}