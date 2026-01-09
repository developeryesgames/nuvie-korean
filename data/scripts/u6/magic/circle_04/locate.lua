local caster = magic_get_caster()
magic_casting_fade_effect(caster)
magic_casting_effect()
local x,y,z = caster.x,caster.y,caster.z
x,y = math.floor(x / 8), math.floor(y / 8)
local lat,lon
if korean_mode then
	if x > 38 then x, lon = x - 38, "동" else x, lon = 38 - x, "서" end
	if y > 45 then y, lat = y - 45, "남" else y, lat = 45 - y, "북" end
	print("\n"..y.."° "..lat..", "..x.."° "..lon.."\n")
else
	if x > 38 then x, lon = x - 38, "E" else x, lon = 38 - x, "W" end
	if y > 45 then y, lat = y - 45, "S" else y, lat = 45 - y, "N" end
	print("\n"..y.."{"..lat..", "..x.."{"..lon.."\n")
end
