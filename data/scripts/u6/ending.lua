local g_img_tbl = {}
local START_YEAR = 161
local START_MONTH = 7
local START_DAY = 4

-- Korean localization support
local korean_mode = false
local ko_text_sprite = nil

local function init_korean()
	print("init_korean: checking is_korean_enabled...\n")
	if is_korean_enabled then
		print("init_korean: is_korean_enabled exists\n")
		local result = is_korean_enabled()
		print("init_korean: is_korean_enabled() returned: " .. tostring(result) .. "\n")
		if result then
			korean_mode = true
			print("init_korean: korean_mode set to true\n")
			if load_translation then
				load_translation("ending_ko.txt")
			end
		end
	else
		print("init_korean: is_korean_enabled does not exist!\n")
	end
end

local function wait_for_input()
	local input = nil
	while input == nil do
		canvas_update()
		input = input_poll()
		if input ~= nil then
			break
		end
	end
	
	return input
end

local g_pal_counter = 0

local star_field = nil

local function stones_rotate_palette()
	if g_pal_counter == 4 then
		canvas_rotate_palette(16, 16)
		g_pal_counter = 0
	else
		g_pal_counter = g_pal_counter + 1
	end
end

local function rotate_and_wait()
	local input = nil
	while input == nil do
		image_update_effect(star_field.image)
		stones_rotate_palette()
		canvas_update()
		input = input_poll()
		if input ~= nil then
			break
		end
	end
end

local function update_star_field_and_wait()
	local input = nil
	while input == nil do
		image_update_effect(star_field.image)
		canvas_update()
		input = input_poll()
		if input ~= nil then
			break
		end
	end
end

local function play()
	init_korean()
	canvas_set_opacity(0xff);
	mouse_cursor_visible(false)
	g_img_tbl = image_load_all("end.shp")
	music_play("brit.m")
--[ [
	canvas_set_palette("endpal.lbm", 0)
	canvas_set_update_interval(25)
	sprite_new(g_img_tbl[7], 0, 0, true)--bg
	
	image_set_transparency_colour(g_img_tbl[0], 0)
	star_field = sprite_new(image_new_starfield(114,94), 106, 0, false)
	star_field.clip_x = 106
	star_field.clip_y = 0
	star_field.clip_w = 107
	star_field.clip_h = 84
	image_update_effect(star_field.image)
	local codex = sprite_new(g_img_tbl[1], 0x97, 0x1d, false)
	local codex_opened = sprite_new(g_img_tbl[2], 0x85, 0x17, false)
	local wall_transparent = sprite_new(g_img_tbl[0], 106, 0, false)--wall
	local wall = sprite_new(g_img_tbl[4], 106, 0, true)--wall
	moongate = sprite_new(g_img_tbl[3], 9, 0x7d, true)--moongate
	moongate.clip_x = 0
	moongate.clip_y = 0
	moongate.clip_w = 320
	moongate.clip_h = 0x7e
	
	local characters = sprite_new(g_img_tbl[8], 0, 0, false)
	characters.clip_x = 0
	characters.clip_y = 0
	characters.clip_w = 160
	characters.clip_h = 200
		
	local characters1 = sprite_new(g_img_tbl[9], 0, 0, false)
	characters1.clip_x = 0
	characters1.clip_y = 0
	characters1.clip_w = 160
	characters1.clip_h = 200
		
	local blue_lens = sprite_new(g_img_tbl[5], 0x49, 0x57, true)--lens
	local red_lens = sprite_new(g_img_tbl[6], 0xda, 0x57, true)--lens
	
	local scroll_img = image_load("end.shp", 0xb)

	if not korean_mode then
		image_print(scroll_img, "A glowing portal springs from the floor!", 7, 303, 34, 13, 0x3e)
	end
	local scroll = sprite_new(scroll_img, 1, 0xa0, true)

	-- Korean text sprite must be created AFTER scroll sprite so it renders on top
	if korean_mode then
		ko_text_sprite = sprite_new(nil, 34, 0xa0 + 13, true)
		ko_text_sprite.text = "바닥에서 빛나는 관문이 솟아오른다!"
		ko_text_sprite.text_color = 0x3e
		ko_text_sprite.text_align = 2
	end
	
	local input
	local i
	
	for i=0x7d,-0xc,-1  do
		stones_rotate_palette()
		moongate.y = i
		canvas_update()
		input = input_poll()
		if input ~= nil then
			moongate.y = -0xc
			break
		end
	end
	
	rotate_and_wait()
	
	characters.visible = true
	
	scroll_img = image_load("end.shp", 0xa)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x85 + 13, true)
		ko_text_sprite.text = "그 진홍빛 심연 속에서 마법사 니스툴을 대동한 로드 브리티시가 나타난다. 왕실 예언자의 얼굴에는 고뇌와 불신이 가득하지만, 로드 브리티시는 그대에게 엄격한 시선을 고정하며 마치 길 잃은 아이를 타이르듯 말한다."
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "From its crimson depths Lord British emerges, trailed by the mage Nystul. Anguish and disbelief prevail on the royal seer's face, but Lord British directs his stony gaze at you and speaks as if to a wayward child.", 8, 303, 7, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x85
	
	rotate_and_wait()
	
	scroll_img = image_load("end.shp", 0xc)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x97 + 12, true)
		ko_text_sprite.text = "\"그대가 우리의 코덱스를 훔칠 만한 정당한 이유가 있었기를 바라오.\" 폐하께서 말씀하신다. \"하지만 미덕의 이름으로 묻겠소..."
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "\"Thou didst have just cause to burgle our Codex, I trust\127 His Majesty says. \"But for Virtue's sake...", 8, 303, 7, 12, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x97
	
	for i=-0xc,0x7d,1  do
		stones_rotate_palette()
		moongate.y = i
		canvas_update()
		input = input_poll()
		if input ~= nil then
			moongate.y = 0x7d
			break
		end
	end
	
	moongate.visible = false
	wait_for_input()
		
	scroll_img = image_load("end.shp", 0xb)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 63, 0xa0 + 13, true)
		ko_text_sprite.text = "\"도대체 그것으로 무슨 짓을 한 것이오?\""
		ko_text_sprite.text_color = 0x3e
		ko_text_sprite.text_align = 2
	else
		image_print(scroll_img, "\"WHAT HAST THOU DONE WITH IT?\127", 8, 303, 63, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x0
	scroll.y = 0xa0


	wait_for_input()

	scroll_img = image_load("end.shp", 0xa)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x85 + 8, true)
		ko_text_sprite.text = "그대는 오목 렌즈를 집어 들어 왕에게 건넨다. \"그 책이 진정 우리의 것이었던 적이 있었습니까, 폐하? 그것이 오직 브리타니아만을 위해 쓰였던 것입니까? 그대는 더 이상 코덱스를 소유하지 않지만, 그 지혜가 진정으로 사라졌다고 생각하십니까? 소용돌이 속을 보십시오. 코덱스가 스스로 대답하게 하십시오!\""
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "You pick up the concave lens and pass it to the King. \"Was the book ever truly ours, Your Majesty? Was it written for Britannia alone? Thou dost no longer hold the Codex, but is its wisdom indeed lost? Look into the Vortex, and let the Codex answer for itself!\127", 8, 303, 7, 8, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x85
	
	wait_for_input()
	
	blue_lens.visible = false
	
	characters.clip_x = 160
	characters.clip_w = 160
	characters.visible = false
	
	characters1.visible = true
	 
	wait_for_input()
	
	scroll_img = image_load("end.shp", 0xc)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x97 + 12, true)
		ko_text_sprite.text = "로드 브리티시가 벽 앞에 렌즈를 비추자, 소용돌이치는 수많은 별들을 배경으로 궁극의 지혜의 코덱스가 서서히 그 모습을 드러낸다!"
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "As Lord British holds the glass before the wall, the Codex of Ultimate Wisdom wavers into view against a myriad of swimming stars!", 8, 303, 7, 12, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x97
	
	star_field.visible = true
	wall_transparent.visible = true

	for i=0xff,0,-3  do
		wall.dissolve = i
		image_update_effect(star_field.image)
		canvas_update()
		input = input_poll()
		if input ~= nil then
			wall.dissolve = 0
			break
		end
	end

	codex.dissolve = 0
	codex.visible = true

	for i=0,0xff,3  do
		codex.dissolve = i
		image_update_effect(star_field.image)
		canvas_update()
		input = input_poll()
		if input ~= nil then
			codex.dissolve = 0xff
			break
		end
	end

	update_star_field_and_wait()
	
	scroll_img = image_load("end.shp", 0xb)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 70, 0xa0 + 13, true)
		ko_text_sprite.text = "그러나 책은 여전히 닫혀 있다."
		ko_text_sprite.text_color = 0x3e
		ko_text_sprite.text_align = 2
	else
		image_print(scroll_img, "Yet the book remains closed.", 8, 303, 70, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x0
	scroll.y = 0xa0

	update_star_field_and_wait()

	music_play("gargoyle.m")
	scroll_img = image_load("end.shp", 0xb)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x98 + 9, true)
		ko_text_sprite.text = "공기 중에 아지랑이가 피어오르며, 또 다른 붉은 관문의 탄생을 알린다!"
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "And waves of heat shimmer in the air, heralding the birth of another red gate!", 8, 303, 7, 9, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x0
	scroll.y = 0x98
	
	moongate.x = 0xe6
	moongate.y = 0x7d
	moongate.visible = true
	
	for i=0x7d,-0xc,-1  do
		image_update_effect(star_field.image)
		stones_rotate_palette()
		moongate.y = i
		canvas_update()
		input = input_poll()
		if input ~= nil then
			moongate.y = -0xc
			break
		end
	end
	
	rotate_and_wait()
	
	characters.visible = true
	
	scroll_img = image_load("end.shp", 0xa)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x85 + 8, true)
		ko_text_sprite.text = "가고일의 왕 드락시누솜이 날개 없는 시종들을 거느리고 성큼성큼 다가온다. 로드 브리티시와 마찬가지로 그 또한 초인적인 의지로 분노를 억누르고 있는 듯하다. 그의 비늘 돋은 손이 그대의 어깨를 움켜쥐자, 그대의 굴복의 아뮬렛이 뜨겁게 달아오른다."
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "King Draxinusom of the Gargoyles strides forward, flanked by a small army of wingless attendants. Like Lord British, he seems to suppress his rage only through a heroic effort of will. His scaly hand grasps your shoulder, and your Amulet of Submission grows very warm.", 8, 303, 7, 8, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x85

	rotate_and_wait()

	scroll_img = image_load("end.shp", 0xb)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 46, 0xa0 + 13, true)
		ko_text_sprite.text = "\"때가 되었다, 도둑놈아,\" 그가 말한다."
		ko_text_sprite.text_color = 0x3e
		ko_text_sprite.text_align = 2
	else
		image_print(scroll_img, "\"Thy time hath come, Thief,\127 he says.", 8, 303, 46, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x0
	scroll.y = 0xa0
	
	for i=-0xc,0x7d,1  do
		image_update_effect(star_field.image)
		stones_rotate_palette()
		moongate.y = i
		canvas_update()
		input = input_poll()
		if input ~= nil then
			moongate.y = 0x7d
			break
		end
	end

	moongate.visible = false
	
	update_star_field_and_wait()
	
	scroll_img = image_load("end.shp", 0xb)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 5, 0x98 + 13, true)
		ko_text_sprite.text = "그대는 재빨리 손을 뻗어 볼록 렌즈를 움켜쥐고..."
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "Quickly you reach down to seize the convex lens...", 8, 310, 5, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x0
	scroll.y = 0x98

	update_star_field_and_wait()

	scroll_img = image_load("end.shp", 0xc)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x97 + 12, true)
		ko_text_sprite.text = "...거대한 가고일 왕의 손에 그것을 쥐여주며 그의 움푹 팬 눈을 응시한다. \"나의 주군과 함께 평화를 찾는 일에 동참해주십시오, 간청합니다.\""
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "...and you press it into the hand of the towering Gargoyle king, meeting his sunken eyes. \"Join my Lord in his search for peace, I beg thee.\127", 8, 303, 7, 12, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x97

	update_star_field_and_wait()
	
	characters.visible = false
	characters1.clip_w = 320
	red_lens.visible = false
		
	scroll_img = image_load("end.shp", 0xa)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x85 + 13, true)
		ko_text_sprite.text = "그대의 간곡한 요청에, 드락시누솜 왕은 마지못해 렌즈를 들어 빛을 모은다. 로드 브리티시 또한 자신의 렌즈를 치켜들자, 방 안의 인간과 가고일 모두의 시선이 벽 위에 빛나는 코덱스의 형상에 고정된다."
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "At your urging, King Draxinusom reluctantly raises his lens to catch the light. As Lord British holds up his own lens, every eye in the room, human and Gargoyle alike, fixes upon the image of the Codex which shines upon the wall.", 8, 303, 7, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x85

	update_star_field_and_wait()

	music_play("end.m")
	scroll_img = image_load("end.shp", 0xa)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x85 + 13, true)
		ko_text_sprite.text = "고대의 책이 펼쳐진다. 두 왕은 홀린 듯 침묵 속에 책장을 응시하고, 각자의 언어로 계시된 궁극의 지혜의 웅변이 울려 퍼진다. 그대 또한 코덱스가 주는 해답을 읽을 수 있었다..."
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "The ancient book opens. Both kings gaze upon its pages in spellbound silence, as the eloquence of Ultimate Wisdom is revealed in the tongues of each lord's domain. You, too, can read the answers the Codex gives...", 8, 303, 7, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x85
	
	codex_opened.dissolve = 0
	codex_opened.visible = true

	for i=0,0xff,3  do
		codex_opened.dissolve = i
		image_update_effect(star_field.image)
		canvas_update()
		input = input_poll()
		if input ~= nil then
			codex_opened.dissolve = 0xff
			break
		end
	end
	codex.visible = false
	
	update_star_field_and_wait()
	
	scroll_img = image_load("end.shp", 0xa)
	if korean_mode then
		if ko_text_sprite then ko_text_sprite.visible = false end
		ko_text_sprite = sprite_new(nil, 7, 0x85 + 13, true)
		ko_text_sprite.text = "...지혜가 모두 전해지고, 로드 브리티시와 드락시누솜 왕이 더 이상의 증오와 공포 없이 서로를 친구로서 마주했을 때, 그대는 브리타니아에서의 그대의 임무가 마침내 끝났음을 깨닫는다."
		ko_text_sprite.text_color = 0x3e
	else
		image_print(scroll_img, "...and when its wisdom is gleaned, when Lord British and King Draxinusom turn to each other as friends, hating no longer, fearing no more, you know that your mission in Britannia has ended at last.", 8, 303, 7, 13, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x85

	update_star_field_and_wait()

	scroll.visible = false
	if ko_text_sprite then ko_text_sprite.visible = false end
	
	for i=0xff,0,-3  do
		codex_opened.dissolve = i
		image_update_effect(star_field.image)
		canvas_update()
		input = input_poll()
		if input ~= nil then
			codex_opened.dissolve = 0
			break
		end
	end

	codex_opened.visible = false

	wall.dissolve = 0
	wall.visible = true

	for i=0,0xff,3  do
		wall.dissolve = i
		image_update_effect(star_field.image)
		canvas_update()
		input = input_poll()
		if input ~= nil then
			wall.dissolve = 0xff
			break
		end
	end
	
	star_field.visible=false
	wait_for_input()
	

	for i=0xff,0,-3 do
		canvas_set_opacity(i)
		canvas_update()
	end
	
	canvas_hide_all_sprites()
	canvas_set_opacity(0xff)

	local num_string = {"zero",
	"one",
	"two",
	"three",
	"four",
	"five",
	"six",
	"seven",
	"eight",
	"nine",
	"ten",
	"eleven",
	"twelve",
	"thirteen",
	"fourteen",
	"fifteen",
	"sixteen",
	"seventeen",
	"eighteen",
	"nineteen",
	"twenty",
	"twenty-one",
	"twenty-two",
	"twenty-three",
	"twenty-four",
	"twenty-five",
	"twenty-six",
	"twenty-seven",
	"twenty-eight",
	"twenty-nine"
	}

	local year = clock_get_year()
	if year == nil then year = 161 end
	local month = clock_get_month()
	if month == nil then month = 7 end
	local day = clock_get_day()
	if day == nil then day = 4 end

	local current_days = year * 365 + month * 30 + day
	local start_days = START_YEAR * 365 + START_MONTH * 30 + START_DAY
	local time = current_days - start_days
	
	local years = math.floor(time / 365)
	local months = math.floor((time - years * 365) / 30)
	local days = time - (years * 365 + months * 30)
	
	local line1 = ""
	
	if years > 0 then
		if years > 29 then
			line1 = line1..years.." year"
		else
			line1 = line1..num_string[years+1].." year"
		end
		
		if years > 1 then
			line1 = line1.."s"
		end
	end
	
	if months > 0 then
		if line1 ~= "" then
			line1 = line1..", "
		end
		line1 = line1..num_string[months+1].." month"
		if months > 1 then
			line1 = line1.."s"
		end
	end

	local line2 = ""
	if days > 0 or line1 == "" then
		if line1 ~= "" then
			line1 = line1..","
			line2 = "and "
		end
		
		line2 = line2..num_string[days+1].." day"
		if days ~= 1 then
			line2 = line2.."s"
		end
	end
	
	if line1 == "" then
		line1 = "Prophet, in"
	else
		line1 = "Prophet, in "..line1
	end
	
	if string.len(line1.." "..line2) <= 38 and line2 ~= "" then
		line1 = line1.." "..line2
		line2 = ""
	end
	
	if line2 == "" then
		line1 = line1.."."
	else
		line2 = line2.."."
	end
	
	local x, y
	y=9
	scroll_img = image_load("end.shp", 0xa)
	if korean_mode then
		-- Korean congratulations message using sprite text
		ko_text_sprite = sprite_new(nil, 1, 0x44 + 9, true)
		ko_text_sprite.text = "축하합니다"
		ko_text_sprite.text_color = 0x3e
		ko_text_sprite.text_align = 2

		local ko_text_sprite2 = sprite_new(nil, 1, 0x44 + 25, true)
		ko_text_sprite2.text = "그대는 울티마 VI: 거짓 예언자를 완수하였습니다."
		ko_text_sprite2.text_color = 0x3e
		ko_text_sprite2.text_align = 2

		local ko_text_sprite3 = sprite_new(nil, 1, 0x44 + 41, true)
		ko_text_sprite3.text = "오리진 시스템즈의 로드 브리티시에게 그대의 업적을 보고하십시오!"
		ko_text_sprite3.text_color = 0x3e
		ko_text_sprite3.text_align = 2
	else
		image_print(scroll_img, "CONGRATULATIONS", 8, 303, 107, y, 0x3e)
		y=y+8
		image_print(scroll_img, "Thou hast completed Ultima VI: The False", 8, 303, 34, y, 0x3e)
		y=y+8
		image_print(scroll_img, line1, 8, 303, math.floor((318-canvas_string_length(line1)) / 2), y, 0x3e)
		y=y+8
		if line2 ~= "" then
			image_print(scroll_img, line2, 8, 303, math.floor((318-canvas_string_length(line2)) / 2), y, 0x3e)
			y=y+8
		end
		image_print(scroll_img, "Report thy feat unto Lord British at Origin", 8, 303, 23, y, 0x3e)
		y=y+8
		image_print(scroll_img, "Systems!", 8, 303, 132, y, 0x3e)
	end
	scroll.image = scroll_img
	scroll.x = 0x1
	scroll.y = 0x44

	scroll.opacity = 0
	scroll.visible = true
	
	for i=0,0xff,3  do
		scroll.opacity = i
		canvas_update()
		input = input_poll()
		if input ~= nil then
			scroll.opacity = 0xff
			break
		end
	end
	
	wait_for_input()

end

play()
