local caster = magic_get_caster()
local actor = select_actor_with_projectile(0x17f, caster)

if actor == nil then return end
if is_god_mode_enabled() and actor.in_party then
	return
end
print("\n")

hit_anim(actor.x, actor.y)
actor.paralyzed = true
local k_name = korean_translate(actor.name)
if is_korean_enabled() then
	print(k_name..korean_get_particle(k_name, "iga").." 마비되었다.\n")
else
	print(actor.name.." is paralyzed.\n")
end

if actor.in_party == true then
	party_update_leader()
end

