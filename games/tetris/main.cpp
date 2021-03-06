#include <flame/serialize.h>
#include <flame/foundation/blueprint.h>
#include <flame/graphics/image.h>
#include <flame/network/network.h>
#include <flame/universe/ui/ui.h>
#include <flame/utils/app.h>
#include <flame/utils/fps.h>

#include "mino.h"
#include "key.h"
#include "score.h"

using namespace flame;

const auto board_width = 10U;
const auto board_height = 24U;
const auto DOWN_TICKS = 60U;
const auto CLEAR_TICKS = 15U;
const auto mino_col_decay = Vec4c(200, 200, 200, 255);

const auto sound_move_volumn = 1.f;
const auto sound_soft_drop_volumn = 0.7f;
const auto sound_hard_drop_volumn = 0.7f;
const auto sound_clear_volumn = 0.5f;
const auto sound_hold_volumn = 0.5f;

auto fx_volumn = 10U;

auto left_right_sensitiveness = 10U;
auto left_right_speed = 2U;
auto soft_drop_speed = 1U;

enum TileIndex
{
	TileGrid,
	TileMino1,
	TileMino2,
	TileMino3,
	TileMino4,
	TileMino5,
	TileMino6,
	TileMino7,
	TileGray,
	TileGhost
};

enum GameMode
{
	GameSingleMarathon,
	GameSingleRTA,
	GameSinglePractice,
	GameVS
};

struct Player
{
	void* id;
	std::wstring name;
	bool disconnected;
	bool ready;
	bool dead;

	Entity* e;
	cText* c_name;
	cTileMap* c_main;
	cTileMap* c_hold;
	cTileMap* c_next[6];
	Entity* e_count_down;
	cText* c_ready;
	cText* c_rank;
	Entity* e_garbage;
	Entity* e_kick;

	void reset()
	{
		id = nullptr;
		disconnected = false;
		ready = false;
		dead = false;
	}
};

struct Garbage
{
	uint time;
	uint lines;
};

struct MyApp : App
{
	graphics::ImageAtlas* atlas;

	sound::Buffer* sound_move_buf;
	sound::Buffer* sound_soft_drop_buf;
	sound::Buffer* sound_hard_drop_buf;
	sound::Buffer* sound_clear_buf;
	sound::Buffer* sound_hold_buf;
	sound::Source* sound_move_src;
	sound::Source* sound_soft_drop_src;
	sound::Source* sound_hard_drop_src;
	sound::Source* sound_clear_src;
	sound::Source* sound_hold_src;

	std::wstring my_name;
	std::wstring room_name;
	std::vector<Player> players;
	uint my_room_index;
	bool room_gaming;
	Server* server;
	uint room_max_people;
	Client* client;

	GameMode game_mode;

	cText* c_text_time;
	cText* c_text_level;
	cText* c_text_lines;
	cText* c_text_score;
	cText* c_text_special;
	Entity* e_start_or_ready;

	int left_frames;
	int right_frames;

	bool gaming;
	float play_time;
	uint level;
	uint lines;
	uint score;
	int clear_ticks;
	uint full_lines[4];
	uint combo;
	bool back_to_back;
	std::vector<Garbage> garbages;
	bool need_update_garbages_tip;
	Vec2i mino_pos;
	MinoType mino_type;
	MinoType mino_hold;
	bool mino_just_hold;
	uint mino_rotation;
	Vec2i mino_coords[3];
	int mino_reset_times;
	uint mino_bottom_dist;
	uint mino_ticks;
	Vec2u mino_pack_idx;
	MinoType mino_packs[2][MinoTypeCount];

	MyApp();
	~MyApp();
	void create();
	void create_home_scene();
	void create_player_controls(int player_index);
	void process_player_entered(int index);
	void process_player_disconnected(int index);
	void process_player_left(int index);
	void process_player_ready(int index);
	void process_game_start();
	void process_report_board(int index, const std::string& d);
	void process_attack(int index, int value);
	void process_dead(int index, int rank);
	void process_gameover();
	void join_room(const char* ip);
	void people_dead(int index);
	void create_lan_scene();
	void create_config_scene();
	void create_key_scene();
	void create_sound_scene();
	void create_sensitiveness_scene();
	void set_board_tiles(cTileMap* m);
	void create_game_scene();
	void do_game_logic();
	void begin_count_down();
	void update_status();
	void start_game();
	void shuffle_pack(uint idx);
	void draw_mino(cTileMap* board, int idx, const Vec2i& pos, uint offset_y, Vec2i* coords, const Vec4c& col = Vec4c(255));
	bool check_board(cTileMap* board, const Vec2i& p);
	bool check_board(cTileMap* board, Vec2i* in, const Vec2i& p);
	bool line_empty(cTileMap* board, uint l);
	bool line_full(cTileMap* board, uint l);
	uint get_rotation_idx(bool clockwise);
	bool super_rotation(cTileMap* board, bool clockwise, Vec2i* out_coord, Vec2i* offset);
	void quit_game();
}app;

struct MainForm : GraphicsWindow
{
	UI ui;

	MainForm();
	~MainForm() override;
};

MainForm* main_window = nullptr;

MainForm::MainForm() :
	GraphicsWindow(&app, true, true, "Tetris", Vec2u(800, 600), WindowFrame)
{
	main_window = this;

	setup_as_main_window();

	canvas->add_atlas(app.atlas);

	ui.init(world);

	ui.parents.push(root);
	{
		auto e = ui.e_text(L"");
		e->event_listeners.add([](Capture& c, EntityEvent e, void*) {
			if (e == EntityDestroyed)
				get_looper()->remove_event(c.thiz<void>());
			return true;
		}, Capture().set_thiz(add_fps_listener([](Capture& c, uint fps) {
			c.thiz<cText>()->set_text(std::to_wstring(fps).c_str());
		}, Capture().set_thiz(e->get_component(cText)))));
	}
	ui.c_aligner(AlignMin, AlignMax);
	ui.parents.pop();

	app.create_home_scene();
}

MainForm::~MainForm()
{
	main_window = nullptr;
}

MyApp::MyApp()
{
	players.resize(1);
	players[0].id = (void*)0xffff;
	my_room_index = 0;
	server = nullptr;
	client = nullptr;

	init_mino();
	init_key();
}

MyApp::~MyApp()
{
	std::ofstream user_data(L"user_data.ini");
	user_data << "name = " << w2s(my_name) << "\n";
	user_data << "\n[key]\n";
	auto key_info = find_enum(FLAME_CHASH("flame::Key"));
	for (auto i = 0; i < KEY_COUNT; i++)
		user_data << w2s(key_names[i]) << " = " << key_info->find_item(key_map[i])->name.str() << "\n";
	user_data << "\n[sound]\n";
	user_data << "fx_volumn = " << fx_volumn << "\n";
	user_data << "\n[sensitiveness]\n";
	user_data << "left_right_sensitiveness = " << left_right_sensitiveness << "\n";
	user_data << "left_right_speed = " << left_right_speed << "\n";
	user_data << "soft_drop_speed = " << soft_drop_speed << "\n";
	user_data.close();

	if (app.developing)
	{
		std::ofstream pack_desc(app.resource_path / L"package_description.ini");
		pack_desc << "src = \"\"\n";
		pack_desc << "dst = \"{c}\"\n\n";
		pack_desc << "[engine_items]\n";
		for (auto& p : app.used_files[0])
			pack_desc << "\"" << p.string() << "\"\n";
		pack_desc << "[items]\n";
		for (auto& p : app.used_files[1])
			pack_desc << "\"" << p.string() << "\"\n";
		pack_desc.close();
	}
}

void MyApp::create()
{
	App::create(false);

	auto user_data = parse_ini_file(L"user_data.ini");
	for (auto& e : user_data.get_section_entries(""))
	{
		if (e.key == "name")
			my_name = s2w(e.value);
	}
	auto key_info = find_enum(FLAME_CHASH("flame::Key"));
	for (auto& e : user_data.get_section_entries("key"))
	{
		for (auto i = 0; i < KEY_COUNT; i++)
		{
			if (key_names[i] == s2w(e.key))
				key_map[i] = (Key)key_info->find_item(e.value.c_str())->value;
		}
	}
	for (auto& e : user_data.get_section_entries("sound"))
	{
		if (e.key == "fx_volumn")
			fx_volumn = std::stoi(e.value);
	}
	for (auto& e : user_data.get_section_entries("sensitiveness"))
	{
		if (e.key == "left_right_sensitiveness")
			left_right_sensitiveness = std::stoi(e.value);
		else if (e.key == "left_right_speed")
			left_right_speed = std::stoi(e.value);
		else if (e.key == "soft_drop_speed")
			soft_drop_speed = std::stoi(e.value);
	}

	atlas = graphics::ImageAtlas::load(graphics_device, (resource_path / L"art/atlas/main.atlas").c_str());

	{
		sound_move_buf = sound::Buffer::create_from_file((resource_path / L"art/move.wav").c_str());
		sound_move_src = sound::Source::create(sound_move_buf);
		sound_move_src->set_volume(sound_move_volumn);
	}
	{
		sound_soft_drop_buf = sound::Buffer::create_from_file((resource_path / L"art/soft_drop.wav").c_str());
		sound_soft_drop_src = sound::Source::create(sound_soft_drop_buf);
		sound_soft_drop_src->set_volume(sound_soft_drop_volumn);
	}
	{
		sound_hard_drop_buf = sound::Buffer::create_from_file((resource_path / L"art/hard_drop.wav").c_str());
		sound_hard_drop_src = sound::Source::create(sound_hard_drop_buf);
		sound_hard_drop_src->set_volume(sound_hard_drop_volumn);
	}
	{
		sound_clear_buf = sound::Buffer::create_from_file((resource_path / L"art/clear.wav").c_str());
		sound_clear_src = sound::Source::create(sound_clear_buf);
		sound_clear_src->set_volume(sound_clear_volumn);
	}
	{
		sound_hold_buf = sound::Buffer::create_from_file((resource_path / L"art/hold.wav").c_str());
		sound_hold_src = sound::Source::create(sound_hold_buf);
		sound_hold_src->set_volume(sound_hold_volumn);
	}
}

void MyApp::create_home_scene()
{
	auto& ui = main_window->ui;

	ui.parents.push(main_window->root);
	ui.e_begin_layout(LayoutVertical, 8.f);
	ui.c_aligner(AlignMiddle, AlignMiddle);
	ui.push_style(FontSize, common(Vec1u(40)));
	ui.e_text(L"Tetris");
	ui.c_aligner(AlignMiddle, 0);
	ui.pop_style(FontSize);
	ui.push_style(FontSize, common(Vec1u(20)));
	ui.e_button(L"Marathon", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.game_mode = GameSingleMarathon;
			app.create_game_scene();
			app.start_game();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.e_button(L"RTA", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.game_mode = GameSingleRTA;
			app.create_game_scene();
			app.start_game();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.e_button(L"Practice", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.game_mode = GameSinglePractice;
			app.create_game_scene();
			app.start_game();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.e_button(L"LAN", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_lan_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.e_button(L"Config", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_config_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.pop_style(FontSize);
	ui.e_end_layout();
	ui.parents.pop();
}

void MyApp::create_player_controls(int player_index)
{
	auto& ui = main_window->ui;

	auto scale = (player_index == my_room_index || room_max_people == 2) ? 1.f : 0.5f;
	auto block_size = 24U * scale;

	auto pos = Vec2f(game_mode != GameVS ? 120.f : 0.f, 0.f);
	if (player_index != my_room_index)
	{
		switch (room_max_people)
		{
		case 2:
			pos = Vec2f(420.f, 0.f);
			break;
		case 7:
		{
			auto index = player_index;
			if (index > my_room_index)
				index--;
			pos = Vec2f(330.f + (index % 3) * 128.f, (index / 3) * 265.f);
		}
		break;
		}
	}

	auto& p = players[player_index];

	ui.next_element_pos = pos + Vec2f(80.f, 40.f) * scale;
	ui.e_begin_layout(LayoutHorizontal, 4.f);
	ui.push_style(FontSize, common(Vec1u(30 * scale)));
	p.c_name = ui.e_text([p]() {
		switch (app.game_mode)
		{
		case GameSingleMarathon:
			return L"Marathon";
		case GameSingleRTA:
			return L"RTA";
		case GameSinglePractice:
			return L"Practice";
		case GameVS:
			return p.name.c_str();
		}
	}())->get_component(cText);
	if (game_mode == GameVS && player_index == 0)
		p.c_name->color = Vec4c(91, 82, 119, 255);
	ui.pop_style(FontSize);

	if (my_room_index == 0 && player_index != my_room_index)
	{
		p.e_kick = ui.e_button(Icon_TIMES, [](Capture& c) {
			auto index = c.data<int>();

			app.process_player_left(index);

			{
				nlohmann::json rep;
				rep["action"] = "player_left";
				rep["index"] = index;
				auto str = rep.dump();
				for (auto i = 1; i < app.players.size(); i++)
				{
					if (i != index)
					{
						auto& p = app.players[i];
						if (p.id && !p.disconnected)
							app.server->send(p.id, str.data(), str.size(), false);
					}
				}
			}
		}, Capture().set_data(&player_index));
	}
	ui.e_end_layout();

	ui.e_empty();
	ui.next_element_pos = pos + Vec2f(85.f, 80.f) * scale;
	ui.next_element_size = Vec2f(block_size * board_width, block_size * (board_height - 3.8f));
	{
		auto ce = ui.c_element();
		ce->frame_thickness = 6.f * scale;
		ce->color = Vec4c(30, 30, 30, 255);
		ce->frame_color = Vec4c(255);
		ce->clip_flags = ClipChildren;;
	}

	ui.parents.push(ui.current_entity);
	ui.e_empty();
	ui.next_element_pos = Vec2f(0.f, -block_size * 3.8f);
	ui.next_element_size = Vec2f(block_size * board_width, block_size * board_height);
	ui.c_element();
	p.c_main = cTileMap::create();
	p.c_main->cell_size_ = Vec2f(block_size);
	p.c_main->set_size(Vec2u(board_width, board_height));
	p.c_main->clear_cells(TileGrid);
	set_board_tiles(p.c_main);
	ui.current_entity->add_component(p.c_main);
	ui.parents.pop();

	if (player_index == my_room_index)
	{
		block_size = 16U * scale;

		ui.next_element_pos = pos + Vec2f(22.f, 80.f);
		ui.e_text(L"Hold");

		ui.next_element_pos = pos + Vec2f(8.f, 100.f);
		ui.next_element_size = Vec2f(block_size * 4 + 8.f);
		ui.next_element_padding = 4.f;
		ui.next_element_color = Vec4c(30, 30, 30, 255);
		ui.e_element();
		{
			p.c_hold = cTileMap::create();
			p.c_hold->cell_size_ = Vec2f(block_size);
			p.c_hold->set_size(Vec2u(4, 3));
			set_board_tiles(p.c_hold);
			ui.current_entity->add_component(p.c_hold);
		}

		ui.next_element_pos = pos + Vec2f(350.f, 80.f);
		ui.e_text(L"Next");

		ui.e_empty();
		ui.next_element_pos = pos + Vec2f(330.f, 100.f);
		ui.next_element_size = Vec2f(block_size * 4 + 8.f, (block_size * 3.f + 4.f) * array_size(p.c_next) + 8.f - 45.f);
		{
			auto ce = ui.c_element();
			ce->color = Vec4c(30, 30, 30, 255);
		}
		auto create_next_board = [&](int i, int base, float y_off, float block_size) {
			ui.next_element_pos = pos + Vec2f(330.f, 100.f + y_off + (block_size * 3.f + 4.f) * (i - base));
			ui.next_element_size = Vec2f(block_size * 4 + 8.f);
			ui.next_element_padding = 4.f;
			ui.e_element();
			{
				p.c_next[i] = cTileMap::create();
				p.c_next[i]->cell_size_ = Vec2f(block_size);
				p.c_next[i]->set_size(Vec2u(4));
				set_board_tiles(p.c_next[i]);
				ui.current_entity->add_component(p.c_next[i]);
			}
		};
		for (auto i = 0; i < 1; i++)
			create_next_board(i, 0, 0.f, 16.f);
		for (auto i = 1; i < 3; i++)
			create_next_board(i, 1, 16.f * 3.f + 4.f, 14.f);
		for (auto i = 3; i < array_size(p.c_next); i++)
			create_next_board(i, 3, 16.f * 3.f + 4.f + (14.f * 3.f + 4.f) * 2, 12.f);

		ui.next_element_pos = pos + Vec2f(180.f, 250.f);
		ui.push_style(FontSize, common(Vec1u(80)));
		p.e_count_down = ui.e_text(L"");
		p.e_count_down->set_visible(false);
		ui.pop_style(FontSize);

		if (game_mode == GameVS)
		{
			ui.next_element_pos = pos + Vec2f(54.f, 546.f);
			p.e_garbage = ui.e_element();
		}
	}

	if (game_mode == GameVS)
	{
		ui.push_style(FontSize, common(Vec1u(60 * scale)));
		ui.next_element_pos = pos + Vec2f(150.f, 200.f) * scale;
		p.c_ready = ui.e_text(L"Ready")->get_component(cText);
		p.c_ready->entity->set_visible(false);
		ui.next_element_pos = pos + Vec2f(160.f, 150.f) * scale;
		p.c_rank = ui.e_text(L"Ready")->get_component(cText);
		p.c_rank->entity->set_visible(false);
		ui.pop_style(FontSize);
	}
}

void MyApp::process_player_entered(int index)
{
	get_looper()->add_event([](Capture& c) {
		auto& ui = main_window->ui;
		auto index = c.data<int>();
		auto& p = app.players[index];
		ui.parents.push(main_window->root);
		p.e = ui.e_element();
		ui.parents.push(p.e);
		app.create_player_controls(index);
		ui.parents.pop();
		ui.parents.pop();
	}, Capture().set_data(&index));
}

void MyApp::process_player_disconnected(int index)
{
	get_looper()->add_event([](Capture& c) {
		auto index = c.data<int>();
		auto& p = app.players[index];
		p.disconnected = true;
		p.c_name->set_text((p.name + L" " + Icon_BOLT).c_str());
	}, Capture().set_data(&index));
}

void MyApp::process_player_left(int index)
{
	get_looper()->add_event([](Capture& c) {
		auto index = c.data<int>();
		auto& p = app.players[index];
		p.reset();
		main_window->root->remove_child(p.e);
	}, Capture().set_data(&index));
}

void MyApp::process_player_ready(int index)
{
	get_looper()->add_event([](Capture& c) {
		auto& p = app.players[c.data<int>()];
		p.ready = true;
		p.c_ready->entity->set_visible(true);
	}, Capture().set_data(&index));
}

void MyApp::process_game_start()
{
	get_looper()->add_event([](Capture&) {
		app.room_gaming = true;
		app.start_game();
	}, Capture());
}

void MyApp::process_report_board(int index, const std::string& d)
{
	struct Capturing
	{
		cTileMap* b;
		char d[1024];
	}capture;
	capture.b = app.players[index].c_main;
	memcpy(capture.d, d.data(), d.size());
	get_looper()->add_event([](Capture& c) {
		auto& capture = c.data<Capturing>();
		for (auto y = 0; y < board_height; y++)
		{
			for (auto x = 0; x < board_width; x++)
			{
				auto id = capture.d[y * board_width + x] - '0';
				capture.b->set_cell(Vec2u(x, y), id, id == TileGrid ? Vec4c(255) : mino_col_decay);
			}
		}
	}, Capture().set_data(&capture));
}

void MyApp::process_attack(int index, int value)
{
	get_looper()->add_event([](Capture& c) {
		auto n = c.data<int>();
		Garbage g;
		g.time = 60;
		g.lines = n;
		app.garbages.push_back(g);
		app.need_update_garbages_tip = true;
	}, Capture().set_data(&value));
}

void MyApp::process_dead(int index, int rank)
{
	struct Capturing
	{
		int index;
		int rank;
	}capture;
	capture.index = index;
	capture.rank = rank;
	get_looper()->add_event([](Capture& c) {
		auto& capture = c.data<Capturing>();
		auto& p = app.players[capture.index];
		std::wstring str;
		switch (capture.rank)
		{
		case 1:
			str = L"1st";
			break;
		case 2:
			str = L"2nd";
			break;
		case 3:
			str = L"3rd";
			break;
		default:
			str = std::to_wstring(capture.rank) + L"th";
		}
		p.c_rank->set_text(str.c_str());
		p.c_rank->entity->set_visible(true);
	}, Capture().set_data(&capture));
}

void MyApp::process_gameover()
{
	get_looper()->add_event([](Capture&) {
		app.room_gaming = false;
		app.gaming = false;
		app.e_start_or_ready->set_visible(true);
	}, Capture());
}

void MyApp::join_room(const char* ip)
{
	auto& ui = main_window->ui;
	app.client = Client::create(SocketNormal, ip, 2434,
	[](Capture&, const char* msg, uint size) {
		auto req = nlohmann::json::parse(std::string(msg, size));
		auto action = req["action"].get<std::string>();
		if (action == "report_room")
		{
			app.room_name = s2w(req["room_name"].get<std::string>());
			app.room_max_people = req["max_people"].get<int>();
			app.players.resize(app.room_max_people);
			for (auto& p : app.players)
				p.reset();
			app.my_room_index = req["index"].get<int>();
			auto& me = app.players[app.my_room_index];
			me.id = (void*)0xffff;
			me.name = app.my_name;
			get_looper()->add_event([](Capture&) {
				main_window->root->remove_children(1, -1);
				app.game_mode = GameVS;
				app.create_game_scene();
			}, Capture());
		}
		else if (action == "player_entered")
		{
			auto index = req["index"].get<int>();
			auto& p = app.players[index];
			p.id = (void*)0xffff;
			p.name = s2w(req["name"].get<std::string>());

			app.process_player_entered(index);
		}
		else if (action == "player_disconnected")
			app.process_player_disconnected(req["index"].get<int>());
		else if (action == "player_left")
			app.process_player_left(req["index"].get<int>());
		else if (action == "player_ready")
			app.process_player_ready(req["index"].get<int>());
		else if (action == "game_start")
			app.process_game_start();
		else if (action == "report_board")
			app.process_report_board(req["index"].get<int>(), req["board"].get<std::string>());
		else if (action == "report_dead")
			app.process_dead(req["index"].get<int>(), req["rank"].get<int>());
		else if (action == "report_gameover")
			app.process_gameover();
		else if (action == "attack")
		{
			auto index = req["index"].get<int>();
			auto value = req["value"].get<int>();
			app.process_attack(index, value);
		}
	},
	[](Capture&) {
		get_looper()->add_event([](Capture&) {
			auto& ui = main_window->ui;
			ui.e_message_dialog(L"Host Has Disconnected")->event_listeners.add([](Capture&, EntityEvent e, void*) {
				if (e == EntityDestroyed)
				{
					get_looper()->add_event([](Capture&) {
						app.quit_game();
					}, Capture());
				}
				return true;
			}, Capture());
		}, Capture());
	}, Capture());
	if (app.client)
	{
		nlohmann::json req;
		req["action"] = "join_room";
		req["name"] = w2s(app.my_name);
		auto str = req.dump();
		app.client->send(str.data(), str.size());
	}
	else
		ui.e_message_dialog(L"Join Room Failed");
}

void MyApp::people_dead(int index)
{
	auto& p = players[index];
	p.dead = true;

	auto total_people = 0;
	auto dead_people = 0;
	auto last_people = 0;
	for (auto i = 0; i < app.players.size(); i++)
	{
		auto& p = app.players[i];
		if (p.id)
		{
			total_people++;
			if (p.dead)
				dead_people++;
			else
				last_people = i;
		}
	}
	auto rank = total_people - dead_people + 1;

	{
		nlohmann::json rep;
		rep["action"] = "report_dead";
		rep["index"] = index;
		rep["rank"] = rank;
		auto str = rep.dump();
		for (auto i = 1; i < app.players.size(); i++)
		{
			auto& p = app.players[i];
			if (p.id && !p.disconnected)
				app.server->send(p.id, str.data(), str.size(), false);
		}
	}

	process_dead(index, rank);

	if (total_people - dead_people == 1)
	{
		app.process_dead(last_people, 1);

		{
			nlohmann::json rep;
			rep["action"] = "report_dead";
			rep["index"] = last_people;
			rep["rank"] = 1;
			auto str = rep.dump();
			for (auto i = 1; i < app.players.size(); i++)
			{
				auto& p = app.players[i];
				if (p.id && !p.disconnected)
					app.server->send(p.id, str.data(), str.size(), false);
			}
		}

		for (auto i = 1; i < app.players.size(); i++)
		{
			auto& p = app.players[i];
			if (p.id)
				p.e_kick->set_visible(true);
		}
		app.process_gameover();

		{
			nlohmann::json rep;
			rep["action"] = "report_gameover";
			auto str = rep.dump();
			for (auto i = 1; i < app.players.size(); i++)
			{
				auto& p = app.players[i];
				if (p.id && !p.disconnected)
					app.server->send(p.id, str.data(), str.size(), false);
			}
		}
	}
}

void MyApp::create_lan_scene()
{
	auto& ui = main_window->ui;

	ui.parents.push(main_window->root);
	ui.next_element_size = Vec2f(500.f, 0.f);
	ui.next_element_padding = 8.f;
	ui.e_begin_layout(LayoutVertical, 8.f, false, false);
	ui.c_aligner(AlignMiddle, AlignMinMax);
	ui.push_style(FontSize, common(Vec1u(20)));
	ui.e_begin_layout(LayoutHorizontal, 8.f);
	ui.c_aligner(AlignMiddle, 0);
	ui.e_text(L"Your Name");
	ui.e_edit(300.f, app.my_name.c_str())->get_component(cText)->data_changed_listeners.add([](Capture& c, uint hash, void*) {
		if (hash == FLAME_CHASH("text"))
			app.my_name = c.current<cText>()->text.str();
		return true;
	}, Capture());
	ui.e_end_layout();
	ui.next_element_padding = 4.f;
	ui.next_element_frame_thickness = 2.f;
	ui.e_begin_scrollbar(ScrollbarVertical, true);
	auto e_room_list = ui.e_begin_list(true);
	ui.e_end_list();
	ui.e_end_scrollbar();
	ui.e_begin_layout(LayoutHorizontal, 8.f)->get_component(cLayout)->fence = -1;
	ui.c_aligner(AlignMinMax, 0);
	ui.e_button(Icon_REFRESH, [](Capture& c) {
		auto e_room_list = c.thiz<Entity>();
		get_looper()->add_event([](Capture& c) {
			auto e_room_list = c.thiz<Entity>();
			e_room_list->remove_children(0, -1);
			nlohmann::json req;
			req["action"] = "get_room";
			auto str = req.dump();
			board_cast(2434, str.data(), str.size(), 1, [](Capture& c, const char* ip, const char* msg, uint size) {
				auto& ui = main_window->ui;

				auto e_room_list = c.thiz<Entity>();
				auto rep = nlohmann::json::parse(std::string(msg, size));
				auto name = s2w(rep["name"].get<std::string>());
				auto host = s2w(rep["host"].get<std::string>());

				ui.parents.push(e_room_list);
				ui.e_list_item((L"Name:" + name + L" Host:" + host).c_str());
				ui.c_data_keeper()->set_string_item(FLAME_CHASH("ip"), ip);
				ui.parents.pop();
			}, Capture().set_thiz(e_room_list));
		}, Capture().set_thiz(e_room_list));
	}, Capture().set_thiz(e_room_list))->get_component(cEventReceiver)->on_mouse(KeyStateDown | KeyStateUp, Mouse_Null, Vec2i(0));
	ui.e_button(L"Create Room", [](Capture&) {
		auto& ui = main_window->ui;

		if (app.my_name.empty())
			ui.e_message_dialog(L"Your Name Cannot Not Be Empty");
		else
		{
			auto e_layer = ui.e_begin_dialog()->parent;
			ui.e_text(L"Room Name");
			ui.e_edit(100.f)->get_component(cText)->data_changed_listeners.add([](Capture& c, uint hash, void*) {
				if (hash == FLAME_CHASH("text"))
					app.room_name = c.current<cText>()->text.str();
				return true;
			}, Capture());
			ui.e_text(L"Max People");
			ui.e_begin_combobox()->get_component(cCombobox)->data_changed_listeners.add([](Capture& c, uint hash, void*) {
				if (hash == FLAME_CHASH("index"))
				{
					auto index = c.current<cCombobox>()->index;
					switch (index)
					{
					case 0:
						app.room_max_people = 2;
						break;
					case 1:
						app.room_max_people = 7;
						break;
					}
				}
				return true;
			}, Capture());
			ui.e_combobox_item(L"2");
			ui.e_combobox_item(L"7");
			ui.e_end_combobox(0);
			app.room_max_people = 2;
			ui.e_begin_layout(LayoutHorizontal, 4.f);
			ui.c_aligner(AlignMiddle, 0);
			ui.e_button(L"OK", [](Capture& c) {
				auto& ui = main_window->ui;

				remove_layer(c.thiz<Entity>());

				if (!app.room_name.empty())
				{
					app.players.resize(app.room_max_people);
					for (auto& p : app.players)
						p.reset();
					app.my_room_index = 0;
					{
						auto& me = app.players[0];
						me.id = (void*)0xffff;
						me.name = app.my_name;
					}
					app.server = Server::create(SocketNormal, 2434,
					[](Capture&, void* id, const char* msg, uint size) {
						auto req = nlohmann::json::parse(std::string(msg, size));
						if (req["action"] == "get_room")
						{
							nlohmann::json rep;
							rep["name"] = w2s(app.room_name);
							rep["host"] = w2s(app.my_name);
							auto str = rep.dump();
							app.server->send(id, str.data(), str.size(), true);
						}
					},
					[](Capture&, void* id) {
						if (!app.room_gaming)
						{
							for (auto i = 0; i < app.players.size(); i++)
							{
								auto& p = app.players[i];
								if (!p.id)
								{
									{
										nlohmann::json rep;
										rep["action"] = "report_room";
										rep["room_name"] = w2s(app.room_name);
										rep["max_people"] = app.room_max_people;
										rep["index"] = i;
										auto str = rep.dump();
										app.server->send(id, str.data(), str.size(), false);
									}
									{
										nlohmann::json rep;
										rep["action"] = "player_entered";
										rep["index"] = app.my_room_index;
										rep["name"] = w2s(app.my_name);
										auto str = rep.dump();
										app.server->send(id, str.data(), str.size(), false);
									}

									p.id = id;
									app.server->set_client(id,
										[](Capture& c, const char* msg, uint size) {
										auto index = c.data<int>();
										auto& p = app.players[index];
										auto req = nlohmann::json::parse(std::string(msg, size));
										auto action = req["action"].get<std::string>();
										if (action == "join_room")
										{
											p.name = s2w(req["name"].get<std::string>());

											app.process_player_entered(index);

											{
												nlohmann::json rep;
												rep["action"] = "player_entered";
												rep["index"] = index;
												rep["name"] = w2s(p.name);
												auto str = rep.dump();
												for (auto i = 1; i < app.players.size(); i++)
												{
													if (i != index)
													{
														auto& p = app.players[i];
														if (p.id && !p.disconnected)
															app.server->send(p.id, str.data(), str.size(), false);
													}
												}
											}
										}
										else if (action == "ready")
										{
											app.process_player_ready(index);

											{
												nlohmann::json rep;
												rep["action"] = "player_ready";
												rep["index"] = index;
												auto str = rep.dump();
												for (auto i = 1; i < app.players.size(); i++)
												{
													if (i != index)
													{
														auto& p = app.players[i];
														if (p.id && !p.disconnected)
															app.server->send(p.id, str.data(), str.size(), false);
													}
												}
											}
										}
										else if (action == "report_board")
										{
											auto d = req["board"].get<std::string>();
											app.process_report_board(req["index"].get<int>(), d);

											{
												nlohmann::json rep;
												rep["action"] = "report_board";
												rep["index"] = index;
												rep["board"] = d;
												auto str = rep.dump();
												for (auto i = 1; i < app.players.size(); i++)
												{
													if (i != index)
													{
														auto& p = app.players[i];
														if (p.id && !p.disconnected)
															app.server->send(p.id, str.data(), str.size(), false);
													}
												}
											}
										}
										else if (action == "report_dead")
											app.people_dead(index);
										else if (action == "attack")
										{
											auto target = req["target"].get<int>();
											auto value = req["value"].get<int>();
											if (target == app.my_room_index)
												app.process_attack(index, value);
											else
											{
												nlohmann::json rep;
												rep["action"] = "attack";
												rep["index"] = index;
												rep["value"] = value;
												auto str = rep.dump();
												auto& p = app.players[target];
												if (p.id && !p.disconnected)
													app.server->send(p.id, str.data(), str.size(), false);
											}
										}
									},
										[](Capture& c) {
										auto index = c.data<int>();

										app.process_player_disconnected(index);

										{
											nlohmann::json rep;
											rep["action"] = "player_disconnected";
											rep["index"] = index;
											auto str = rep.dump();
											for (auto i = 1; i < app.players.size(); i++)
											{
												if (i != index)
												{
													auto& p = app.players[i];
													if (p.id && !p.disconnected)
														app.server->send(p.id, str.data(), str.size(), false);
												}
											}
										}

										if (app.room_gaming)
											app.people_dead(index);
									}, Capture().set_data(&i));

									break;
								}
							}
						}
					}, Capture());
					app.room_gaming = false;
					get_looper()->add_event([](Capture&) {
						main_window->root->remove_children(1, -1);
						app.game_mode = GameVS;
						app.create_game_scene();
					}, Capture());
				}
			}, Capture().set_thiz(e_layer));
			ui.e_button(L"Cancel", [](Capture& c) {
				remove_layer(c.thiz<Entity>());
			}, Capture().set_thiz(e_layer));
			ui.e_end_layout();
			ui.e_end_dialog();
		}
	}, Capture());
	ui.e_button(L"Join Room", [](Capture& c) {
		auto& ui = main_window->ui;
		auto e_room_list = c.thiz<Entity>();
		auto selected = e_room_list->get_component(cList)->selected;
		if (selected)
		{
			if (app.my_name.empty())
				ui.e_message_dialog(L"Your Name Cannot Not Be Empty");
			else
				app.join_room(selected->get_component(cDataKeeper)->get_string_item(FLAME_CHASH("ip")));
		}
		else
			ui.e_message_dialog(L"You Need To Select A Room");
	}, Capture().set_thiz(e_room_list));
	ui.e_button(L"Direct Connect", [](Capture&) {
		auto& ui = main_window->ui;
		if (app.my_name.empty())
			ui.e_message_dialog(L"Your Name Cannot Not Be Empty");
		else
		{
			ui.e_input_dialog(L"IP", [](Capture&, bool ok, const wchar_t* text) {
				if (ok)
					app.join_room(w2s(text).c_str());
			}, Capture());
		}
	}, Capture());
	ui.e_button(L"Back", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_home_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMax, AlignMin);
	ui.e_end_layout();
	ui.pop_style(FontSize);
	ui.e_end_layout();
	ui.parents.pop();
}

void MyApp::create_config_scene()
{
	auto& ui = main_window->ui;
	ui.parents.push(main_window->root);
	ui.e_begin_layout(LayoutVertical, 8.f);
	ui.c_aligner(AlignMiddle, AlignMiddle);
	ui.push_style(FontSize, common(Vec1u(20)));
	ui.e_button(L"Key", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_key_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.e_button(L"Sound", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_sound_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.e_button(L"Sensitiveness", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_sensitiveness_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.e_button(L"Back", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_home_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.pop_style(FontSize);
	ui.e_end_layout();
	ui.parents.pop();
}

void MyApp::create_key_scene()
{
	auto& ui = main_window->ui;
	ui.parents.push(main_window->root);
	ui.e_begin_layout(LayoutVertical, 8.f);
	ui.c_aligner(AlignMiddle, AlignMiddle);
	ui.push_style(FontSize, common(Vec1u(20)));
	auto key_info = find_enum(FLAME_CHASH("flame::Key"));
	for (auto i = 0; i < KEY_COUNT; i++)
	{
		ui.e_begin_layout(LayoutHorizontal, 4.f);
		ui.e_text(key_names[i]);
		struct Capturing
		{
			cText* t;
			int i;
		}capture;
		auto e_edit = ui.e_edit(200.f, s2w(key_info->find_item(key_map[i])->name.str()).c_str());
		capture.t = e_edit->get_component(cText);
		capture.i = i;
		e_edit->get_component(cEventReceiver)->key_listeners.add([](Capture& c, KeyStateFlags action, int value) {
			if (action == KeyStateDown)
			{
				auto& capture = c.data<Capturing>();
				key_map[capture.i] = (Key)value;
				auto key_info = find_enum(FLAME_CHASH("flame::Key"));
				capture.t->set_text(s2w(key_info->find_item((Key)value)->name.str()).c_str());
			}
			return false;
		}, Capture().set_data(&capture), 0);
		ui.c_aligner(AlignMinMax | AlignGreedy, 0);
		ui.e_end_layout();
	}
	ui.e_button(L"Back", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_config_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.pop_style(FontSize);
	ui.e_end_layout();
	ui.parents.pop();
}

void MyApp::create_sound_scene()
{
	auto& ui = main_window->ui;
	ui.parents.push(main_window->root);
	ui.e_begin_layout(LayoutVertical, 8.f);
	ui.c_aligner(AlignMiddle, AlignMiddle);
	ui.push_style(FontSize, common(Vec1u(20)));
	struct Capturing
	{
		cText* t;
		int v;
	}capture;
	ui.e_begin_layout(LayoutHorizontal, 4.f);
	capture.t = ui.e_text(wfmt(L"FX %d", fx_volumn).c_str())->get_component(cText);
	auto change_fx_volumn = [](Capture& c) {
		auto& capture = c.data<Capturing>();
		auto v = fx_volumn + capture.v;
		if (v >= 0 && v <= 10)
		{
			fx_volumn = v;
			capture.t->set_text(wfmt(L"FX %d", v).c_str());

			auto f = v / 10.f;
			app.sound_move_src->set_volume(f * sound_move_volumn);
			app.sound_soft_drop_src->set_volume(f * sound_soft_drop_volumn);
			app.sound_hard_drop_src->set_volume(f * sound_hard_drop_volumn);
			app.sound_clear_src->set_volume(f * sound_clear_volumn);
			app.sound_hold_src->set_volume(f * sound_hold_volumn);
		}
	};
	capture.v = -1;
	ui.e_button(L"-", change_fx_volumn, Capture().set_data(&capture));
	capture.v = 1;
	ui.e_button(L"+", change_fx_volumn, Capture().set_data(&capture));
	ui.e_end_layout();
	ui.e_button(L"Back", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_config_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.pop_style(FontSize);
	ui.e_end_layout();
	ui.parents.pop();
}

void MyApp::create_sensitiveness_scene()
{
	auto& ui = main_window->ui;
	ui.parents.push(main_window->root);
	ui.e_begin_layout(LayoutVertical, 8.f);
	ui.c_aligner(AlignMiddle, AlignMiddle);
	ui.push_style(FontSize, common(Vec1u(20)));
	ui.e_text(L"Small Number Means More Sensitivity Or Faster");
	struct Capturing
	{
		cText* t;
		int v;
	}capture;
	ui.e_begin_layout(LayoutHorizontal, 4.f);
	capture.t = ui.e_text(wfmt(L"Left Right Sensitiveness %d",
		left_right_sensitiveness).c_str())->get_component(cText);
	auto change_lr_sens = [](Capture& c) {
		auto& capture = c.data<Capturing>();
		auto v = left_right_sensitiveness + capture.v;
		if (v >= 5 && v <= 30)
		{
			left_right_sensitiveness = v;
			capture.t->set_text(wfmt(L"Left Right Sensitiveness %d", v).c_str());
		}
	};
	capture.v = -1;
	ui.e_button(L"-", change_lr_sens, Capture().set_data(&capture));
	capture.v = 1;
	ui.e_button(L"+", change_lr_sens, Capture().set_data(&capture));
	ui.e_end_layout();
	ui.e_begin_layout(LayoutHorizontal, 4.f);
	capture.t = ui.e_text(wfmt(L"Left Right Speed %d",
		left_right_speed).c_str())->get_component(cText);
	auto change_lr_sp = [](Capture& c) {
		auto& capture = c.data<Capturing>();
		auto v = left_right_speed + capture.v;
		if (v >= 1 && v <= 10)
		{
			left_right_speed = v;
			capture.t->set_text(wfmt(L"Left Right Speed %d", v).c_str());
		}
	};
	capture.v = -1;
	ui.e_button(L"-", change_lr_sp, Capture().set_data(&capture));
	capture.v = 1;
	ui.e_button(L"+", change_lr_sp, Capture().set_data(&capture));
	ui.e_end_layout();
	ui.e_begin_layout(LayoutHorizontal, 4.f);
	capture.t = ui.e_text(wfmt(L"Soft Drop Speed %d",
		soft_drop_speed).c_str())->get_component(cText);
	auto change_sd_sp = [](Capture& c) {
		auto& capture = c.data<Capturing>();
		auto v = soft_drop_speed + capture.v;
		if (v >= 1 && v <= 10)
		{
			soft_drop_speed = v;
			capture.t->set_text(wfmt(L"Soft Drop Speed %d", v).c_str());
		}
	};
	capture.v = -1;
	ui.e_button(L"-", change_sd_sp, Capture().set_data(&capture));
	capture.v = 1;
	ui.e_button(L"+", change_sd_sp, Capture().set_data(&capture));
	ui.e_end_layout();
	ui.e_button(L"Back", [](Capture&) {
		get_looper()->add_event([](Capture&) {
			main_window->root->remove_children(1, -1);
			app.create_config_scene();
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMiddle, 0);
	ui.pop_style(FontSize);
	ui.e_end_layout();
	ui.parents.pop();
}

void MyApp::set_board_tiles(cTileMap* m)
{
	m->add_tile((atlas->canvas_slot_ << 16) + atlas->find_tile(FLAME_HASH("grid.png")));
	for (auto i = 1; i <= 7; i++)
		m->add_tile((atlas->canvas_slot_ << 16) + atlas->find_tile(FLAME_HASH((std::to_string(i) + ".png").c_str())));
	m->add_tile((atlas->canvas_slot_ << 16) + atlas->find_tile(FLAME_HASH("gray.png")));
	m->add_tile((atlas->canvas_slot_ << 16) + atlas->find_tile(FLAME_HASH("ghost.png")));
}

void MyApp::create_game_scene()
{
	auto& ui = main_window->ui;

	ui.parents.push(main_window->root);
	ui.push_style(FontSize, common(Vec1u(20)));

	ui.e_empty();
	{
		auto ev = get_looper()->add_event([](Capture& c) {
			c._current = INVALID_POINTER;
		}, Capture());
		ui.current_entity->event_listeners.add([](Capture& c, EntityEvent e, void*) {
			if (e == EntityRemoved)
				get_looper()->remove_event(c.data<void*>());
			return true;
		}, Capture().set_data(&ev));
	}

	if (game_mode == GameVS)
		ui.e_text(wfmt(L"Room: %s", room_name.c_str()).c_str());

	create_player_controls(my_room_index);

	if (game_mode != GameVS)
	{
		ui.next_element_pos = Vec2f(535.f, 150.f);
		ui.e_text(L"TIME")->get_component(cText)->color = Vec4c(40, 80, 200, 255);

		ui.next_element_pos = Vec2f(535.f, 210.f);
		ui.e_text(L"LEVEL")->get_component(cText)->color = Vec4c(40, 80, 200, 255);

		ui.next_element_pos = Vec2f(535.f, 270.f);
		ui.e_text(game_mode == GameSingleRTA ? L"LEFT" : L"LINES")->get_component(cText)->color = Vec4c(40, 80, 200, 255);

		ui.next_element_pos = Vec2f(535.f, 330.f);
		ui.e_text(L"SCORE")->get_component(cText)->color = Vec4c(40, 80, 200, 255);

		ui.push_style(FontSize, common(Vec1u(40)));
		ui.next_element_pos = Vec2f(535.f, 170.f);
		c_text_time = ui.e_text(L"")->get_component(cText);
		ui.next_element_pos = Vec2f(535.f, 230.f);
		c_text_level = ui.e_text(L"")->get_component(cText);
		ui.next_element_pos = Vec2f(535.f, 290.f);
		c_text_lines = ui.e_text(L"")->get_component(cText);
		ui.next_element_pos = Vec2f(535.f, 350.f);
		c_text_score = ui.e_text(L"")->get_component(cText);
		ui.pop_style(FontSize);
	}

	ui.push_style(FontSize, common(Vec1u(28)));
	ui.next_element_pos = Vec2f(8.f, 230.f);
	{
		auto e = ui.e_text(L"");
		e->set_visible(false);
		c_text_special = e->get_component(cText);
	}
	c_text_special->color = Vec4c(200, 80, 40, 255);
	ui.pop_style(FontSize);

	if (game_mode == GameVS)
	{
		ui.next_element_pos = Vec2f(4.f, 500.f);
		if (my_room_index == 0)
		{
			e_start_or_ready = ui.e_button(L"Start", [](Capture&) {
				if ([]() {
					auto n = 0;
						for (auto i = 1; i < app.players.size(); i++)
						{
							auto& p = app.players[i];
								if (!p.id)
									continue;
								if (!p.ready)
									return false;
								n++;
						}
					return n != 0;
				}())
				{
					for (auto i = 1; i < app.players.size(); i++)
					{
						auto& p = app.players[i];
						if (p.id && !p.disconnected)
							p.e_kick->set_visible(false);
					}
					app.process_game_start();

					{
						nlohmann::json req;
						req["action"] = "game_start";
						auto str = req.dump();
						for (auto i = 1; i < app.players.size(); i++)
						{
							auto& p = app.players[i];
							if (p.id && !p.disconnected)
								app.server->send(p.id, str.data(), str.size(), false);
						}
					}
				}
			}, Capture());
		}
		else
		{
			e_start_or_ready = ui.e_button(L"Ready", [](Capture&) {
				auto& me = app.players[app.my_room_index];
				if (!app.room_gaming && !me.ready)
				{
					me.ready = true;
					nlohmann::json req;
					req["action"] = "ready";
					auto str = req.dump();
					app.client->send(str.data(), str.size());

					app.process_player_ready(app.my_room_index);
				}
			}, Capture());
		}
	}

	ui.e_button(Icon_TIMES, [](Capture&) {
		auto& ui = main_window->ui;
		ui.e_confirm_dialog(L"Quit?", [](Capture&, bool yes) {
			if (yes)
			{
				get_looper()->add_event([](Capture&) {
					app.quit_game();
				}, Capture());
			}
		}, Capture());
	}, Capture());
	ui.c_aligner(AlignMax, AlignMin);

	ui.pop_style(FontSize);
	ui.parents.pop();
}

void MyApp::begin_count_down()
{
	auto e_count_down = players[my_room_index].e_count_down;
	e_count_down->set_visible(true);
	e_count_down->get_component(cText)->set_text(L"3");
	auto t = 3;
	get_looper()->remove_events(FLAME_CHASH("count_down"));
	get_looper()->add_event([](Capture& c) {
		auto& t = c.data<int>();
		t--;

		auto e = c.thiz<Entity>();
		if (t == 0)
		{
			e->set_visible(false);
			app.gaming = true;
		}
		else
		{
			c._current = INVALID_POINTER;
			e->get_component(cText)->set_text(std::to_wstring(t).c_str());
		}
	}, Capture().set_thiz(e_count_down).set_data(&t), 1.f, FLAME_CHASH("count_down"));
}

void MyApp::update_status()
{
	if (game_mode != GameVS)
	{
		c_text_time->set_text(wfmt(L"%02d:%02d.%02d", (int)play_time / 60, ((int)play_time) % 60, int(play_time * 100) % 100).c_str());
		c_text_level->set_text(wfmt(L"%02d", level).c_str());
		if (game_mode == GameSingleRTA)
			c_text_lines->set_text(wfmt(L"%02d", max(0, 40 - (int)lines)).c_str());
		else
			c_text_lines->set_text(wfmt(L"%04d", lines).c_str());
		c_text_score->set_text(wfmt(L"%09d", score).c_str());
	}
}

void MyApp::start_game()
{
	for (auto i = 0; i < players.size(); i++)
	{
		auto& p = app.players[i];
		if (p.id)
		{
			p.ready = false;
			p.dead = false;
			p.c_main->clear_cells(TileGrid);
			if (i == app.my_room_index)
			{
				p.c_hold->clear_cells(-1);
				for (auto j = 0; j < array_size(p.c_next); j++)
					p.c_next[j]->clear_cells(-1);
			}
			if (game_mode == GameVS)
			{
				p.c_ready->entity->set_visible(false);
				p.c_rank->entity->set_visible(false);
			}
		}
	}
	if (game_mode == GameVS)
		e_start_or_ready->set_visible(false);

	left_frames = -1;
	right_frames = -1;

	play_time = 0.f;
	level = 1;
	lines = 0;
	score = 0;
	clear_ticks = -1;
	combo = 0;
	back_to_back = false;
	garbages.clear();
	need_update_garbages_tip = true;
	mino_pos = Vec2i(0, -1);
	mino_type = MinoTypeCount;
	mino_hold = MinoTypeCount;
	mino_just_hold = false;
	mino_pack_idx = Vec2u(0, 0);
	mino_rotation = 0;
	mino_reset_times = -1;
	mino_bottom_dist = 0;
	mino_ticks = 0;
	{
		auto seed = ::time(0);
		if (server)
			seed++;
		else if (client)
			seed--;
		srand(seed);
	}
	for (auto i = 0; i < 2; i++)
		shuffle_pack(i);

	update_status();
	if (game_mode == GameVS)
		players[my_room_index].e_garbage->remove_children(0, -1);

	gaming = false;
	begin_count_down();
}

void MyApp::shuffle_pack(uint idx)
{
	auto& curr_pack = mino_packs[idx];
	for (auto i = 0; i < MinoTypeCount; i++)
		curr_pack[i] = (MinoType)i;
	for (auto i = 0; i < MinoTypeCount; i++)
		std::swap(curr_pack[i], curr_pack[rand() % MinoTypeCount]);
}

void MyApp::draw_mino(cTileMap* board, int idx, const Vec2i& pos, uint offset_y, Vec2i* coords, const Vec4c& col)
{
	board->set_cell(Vec2u(pos) + Vec2u(0, offset_y), idx, col);
	board->set_cell(Vec2u(pos + coords[0] + Vec2u(0, offset_y)), idx, col);
	board->set_cell(Vec2u(pos + coords[1] + Vec2u(0, offset_y)), idx, col);
	board->set_cell(Vec2u(pos + coords[2] + Vec2u(0, offset_y)), idx, col);
}

bool MyApp::check_board(cTileMap* board, const Vec2i& p)
{
	return
		board->cell(mino_pos + p) == TileGrid &&
		board->cell(mino_pos + p + mino_coords[0]) == TileGrid &&
		board->cell(mino_pos + p + mino_coords[1]) == TileGrid &&
		board->cell(mino_pos + p + mino_coords[2]) == TileGrid;
}

bool MyApp::check_board(cTileMap* board, Vec2i* in, const Vec2i& p)
{
	return
		board->cell(p) == TileGrid &&
		board->cell(in[0] + p) == TileGrid &&
		board->cell(in[1] + p) == TileGrid &&
		board->cell(in[2] + p) == TileGrid;
}

bool MyApp::line_empty(cTileMap* board, uint l)
{
	for (auto x = 0; x < board_width; x++)
	{
		if (board->cell(Vec2i(x, l)) != TileGrid)
			return false;
	}
	return true;
}

bool MyApp::line_full(cTileMap* board, uint l)
{
	for (auto x = 0; x < board_width; x++)
	{
		if (board->cell(Vec2i(x, l)) == TileGrid)
			return false;
	}
	return true;
}

uint MyApp::get_rotation_idx(bool clockwise)
{
	if (clockwise)
		return mino_rotation == 3 ? 0 : mino_rotation + 1;
	return mino_rotation == 0 ? 3 : mino_rotation - 1;
}

bool MyApp::super_rotation(cTileMap* board, bool clockwise, Vec2i* out_coord, Vec2i* offset)
{
	Mat2x2i mats[] = {
		Mat2x2i(Vec2i(0, -1), Vec2i(1, 0)),
		Mat2x2i(Vec2i(0, 1), Vec2i(-1, 0))
	};
	auto& mat = mats[clockwise ? 1 : 0];
	out_coord[0] = mat * mino_coords[0];
	out_coord[1] = mat * mino_coords[1];
	out_coord[2] = mat * mino_coords[2];
	auto offsets = g_mino_LTSZJ_offsets;
	if (mino_type == Mino_O)
		offsets = g_mino_O_offsets;
	else if (mino_type == Mino_I)
		offsets = g_mino_I_offsets;
	auto new_ridx = get_rotation_idx(clockwise);
	for (auto i = 0; i < 5; i++)
	{
		*offset = offsets[i][mino_rotation] - offsets[i][new_ridx];
		if (check_board(board, out_coord, mino_pos + *offset))
			return true;
	}
	return false;
}

void MyApp::quit_game()
{
	room_gaming = false;
	gaming = false;

	players.resize(1);
	players[0].id = (void*)0xffff;
	my_room_index = 0;
	if (server)
	{
		Server::destroy(server);
		server = nullptr;
	}
	if (client)
	{
		Client::destroy(client);
		client = nullptr;
	}

	get_looper()->add_event([](Capture&) {
		main_window->root->remove_children(1, -1);
		app.create_home_scene();
	}, Capture());
}

void MyApp::do_game_logic()
{
	auto& ui = main_window->ui;

	auto& key_states = main_window->s_event_dispatcher->key_states;

	if (game_mode != GameVS)
	{
		if (key_states[key_map[KEY_PAUSE]] == (KeyStateDown | KeyStateJust))
		{
			auto layer = main_window->root->last_child(FLAME_CHASH("layer_paused"));
			if (!layer)
			{
				gaming = false;
				players[my_room_index].e_count_down->set_visible(false);

				auto layer = ui.e_begin_dialog()->parent;
				layer->name = "layer_paused";
				ui.e_text(L"Paused");
				ui.c_aligner(AlignMiddle, 0);
				ui.e_button(L"Resume", [](Capture& c) {
					remove_layer(c.thiz<Entity>());
					app.begin_count_down();
				}, Capture().set_thiz(layer));
				ui.c_aligner(AlignMiddle, 0);
				ui.e_button(L"Restart", [](Capture& c) {
					remove_layer(c.thiz<Entity>());
					app.play_time = 0.f;
					app.start_game();
				}, Capture().set_thiz(layer));
				ui.c_aligner(AlignMiddle, 0);
				ui.e_button(L"Quit", [](Capture& c) {
					remove_layer(c.thiz<Entity>());
					app.quit_game();
				}, Capture().set_thiz(layer));
				ui.c_aligner(AlignMiddle, 0);
				ui.e_end_dialog();
			}
			else
			{
				remove_layer(layer);
				begin_count_down();
			}
		}
	}

	if (gaming)
	{
		main_window->s_2d_renderer->pending_update = true;

		play_time += get_looper()->delta_time;

		auto& p = players[my_room_index];
		auto& c_main = p.c_main;
		auto& c_hold = p.c_hold;
		auto& c_next = p.c_next;
		auto& e_garbage = p.e_garbage;

		auto dead = false;

		if (clear_ticks != -1)
		{
			clear_ticks--;
			if (clear_ticks <= 0)
			{
				for (auto i = 0; i < 4; i++)
				{
					auto l = full_lines[i];
					if (l != -1)
					{
						for (auto j = (int)l; j >= 0; j--)
						{
							if (j > 0)
							{
								for (auto x = 0; x < board_width; x++)
								{
									auto id = c_main->cell(Vec2i(x, j - 1));
									c_main->set_cell(Vec2u(x, j), id, id == TileGrid ? Vec4c(255) : mino_col_decay);
								}
							}
							else
							{
								for (auto x = 0; x < board_width; x++)
									c_main->set_cell(Vec2u(x, j), TileGrid);
							}
						}
					}
				}

				clear_ticks = -1;
			}
		}
		else
		{
			if (mino_pos.y() >= 0)
			{
				draw_mino(c_main, TileGrid, mino_pos, 0, mino_coords);
				if (mino_bottom_dist > 0)
					draw_mino(c_main, TileGrid, mino_pos, mino_bottom_dist, mino_coords);
			}

			if (mino_pos.y() < 0)
			{
				if (mino_pos.y() == -1 || mino_type == MinoTypeCount)
				{
					mino_type = mino_packs[mino_pack_idx.x()][mino_pack_idx.y()++];
					if (mino_pack_idx.y() >= MinoTypeCount)
					{
						shuffle_pack(mino_pack_idx.x());
						mino_pack_idx = Vec2i(1 - mino_pack_idx.x(), 0);
					}
					for (auto i = 0; i < array_size(c_next); i++)
					{
						c_next[i]->clear_cells();
						auto next_idx = mino_pack_idx;
						next_idx.y() += i;
						if (next_idx.y() >= MinoTypeCount)
						{
							next_idx.x() = 1 - next_idx.x();
							next_idx.y() %= MinoTypeCount;
						}
						auto t = mino_packs[next_idx.x()][next_idx.y()];
						Vec2i coords[3];
						for (auto j = 0; j < 3; j++)
							coords[j] = g_mino_coords[t][j];
						draw_mino(c_next[i], TileMino1 + t, Vec2i(1), 0, coords);
					}
				}
				if (mino_pos.y() == -2)
				{
					c_hold->clear_cells();
					if (mino_hold != MinoTypeCount)
					{
						Vec2i coords[3];
						for (auto i = 0; i < 3; i++)
							coords[i] = g_mino_coords[mino_hold][i];
						draw_mino(c_hold, TileMino1 + mino_hold, Vec2i(1), 0, coords);
					}
				}
				mino_pos = Vec2i(4, 5 - (mino_type == Mino_I ? 1 : 0));
				mino_rotation = 0;
				for (auto i = 0; i < 3; i++)
					mino_coords[i] = g_mino_coords[mino_type][i];
				mino_reset_times = -1;
				mino_ticks = 0;

				dead = !check_board(c_main, Vec2i(0));
				if (dead)
				{
					{
						auto pos = mino_pos;
						c_main->set_cell(Vec2u(pos), c_main->cell(pos) == TileGrid ? TileMino1 + mino_type : TileGray, mino_col_decay);
					}
					{
						auto pos = mino_pos + mino_coords[0];
						c_main->set_cell(Vec2u(pos), c_main->cell(pos) == TileGrid ? TileMino1 + mino_type : TileGray, mino_col_decay);
					}
					{
						auto pos = mino_pos + mino_coords[1];
						c_main->set_cell(Vec2u(pos), c_main->cell(pos) == TileGrid ? TileMino1 + mino_type : TileGray, mino_col_decay);
					}
					{
						auto pos = mino_pos + mino_coords[2];
						c_main->set_cell(Vec2u(pos), c_main->cell(pos) == TileGrid ? TileMino1 + mino_type : TileGray, mino_col_decay);
					}
				}
				if (dead || (game_mode == GameSingleRTA && lines >= 40))
				{
					gaming = false;

					if (game_mode != GameVS)
					{
						auto layer = ui.e_begin_dialog()->parent;
						ui.e_text(L"Game Over");
						ui.c_aligner(AlignMiddle, 0);
						ui.e_text((L"Time: " + wfmt(L"%02d:%02d", (int)play_time / 60, ((int)play_time) % 60)).c_str());
						ui.e_text((L"Level: " + wfmt(L"%d", level)).c_str());
						ui.e_text((L"Lines: " + wfmt(L"%d", lines)).c_str());
						ui.e_text((L"Score: " + wfmt(L"%d", score)).c_str());
						ui.e_button(L"Quit", [](Capture& c) {
							remove_layer(c.thiz<Entity>());
							app.quit_game();
						}, Capture().set_thiz(layer));
						ui.c_aligner(AlignMiddle, 0);
						ui.e_button(L"Restart", [](Capture& c) {
							remove_layer(c.thiz<Entity>());
							app.start_game();
						}, Capture().set_thiz(layer));
						ui.c_aligner(AlignMiddle, 0);
						ui.e_end_dialog();
					}
				}
			}

			if (!dead)
			{
				if (key_states[key_map[KEY_HOLD]] == (KeyStateDown | KeyStateJust) && (game_mode == GameSinglePractice || mino_just_hold == false))
				{
					mino_pos.y() = -2;
					std::swap(mino_hold, mino_type);
					mino_just_hold = true;

					sound_hold_src->play();
				}
				else
				{
					static auto last_is_rotate_action = false;
					static auto mini = false;

					auto rotated = false;

					auto r = 0;
					if (key_states[key_map[KEY_ROTATE_LEFT]] == (KeyStateDown | KeyStateJust))
						r--;
					if (key_states[key_map[KEY_ROTATE_RIGHT]] == (KeyStateDown | KeyStateJust))
						r++;
					if (r != 0)
					{
						Vec2i new_coords[3];
						Vec2i offset;
						if (super_rotation(c_main, r == 1, new_coords, &offset))
						{
							if (offset != 0)
								mini = true;
							else
								mini = false;
							mino_rotation = get_rotation_idx(r == 1);
							mino_pos += offset;
							for (auto i = 0; i < 3; i++)
								mino_coords[i] = new_coords[i];
							rotated = true;

							sound_move_src->play();
						}
					}

					auto moved = false;

					auto mx = 0;
					if (key_states[key_map[KEY_LEFT]] & KeyStateDown)
					{
						if (left_frames == -1)
							left_frames = 0;
						else
							left_frames++;
						if (left_frames == 0 || (left_frames >= left_right_sensitiveness
							&& left_frames % left_right_speed == 0))
							mx--;
					}
					else
						left_frames = -1;
					if (key_states[key_map[KEY_RIGHT]] & KeyStateDown)
					{
						if (right_frames == -1)
							right_frames = 0;
						else
							right_frames++;
						if (right_frames == 0 || (right_frames >= left_right_sensitiveness
							&& right_frames % left_right_speed == 0))
							mx++;
					}
					else
						right_frames = -1;
					if (mx != 0 && check_board(c_main, Vec2i(mx, 0)))
					{
						mino_pos.x() += mx;
						moved = true;

						sound_move_src->play();
					}

					if (!last_is_rotate_action)
						last_is_rotate_action = rotated && !moved;
					else
						last_is_rotate_action = !moved;

					mino_bottom_dist = 0;
					while (check_board(c_main, Vec2i(0, mino_bottom_dist + 1)))
						mino_bottom_dist++;
					if (rotated || moved)
					{
						if (game_mode == GameSinglePractice)
							mino_ticks = 0;
						else
						{
							if (mino_reset_times == -1 && mino_bottom_dist == 0)
								mino_reset_times = 0;
							if (mino_reset_times >= 0)
							{
								if (mino_reset_times >= 15)
									mino_ticks = DOWN_TICKS;
								else
									mino_ticks = 0;
								mino_reset_times++;
							}
						}
					}
					auto is_soft_drop = key_states[key_map[KEY_SOFT_DROP]] & KeyStateDown;
					auto down_ticks_final = DOWN_TICKS;
					if (game_mode == GameSinglePractice)
						down_ticks_final = 9999;
					else
						down_ticks_final = down_ticks_final - level + 1;
					if (mino_bottom_dist == 0)
						down_ticks_final = 30;
					else if (is_soft_drop)
						down_ticks_final = soft_drop_speed;
					auto hard_drop = key_states[key_map[KEY_HARD_DROP]] == (KeyStateDown | KeyStateJust);
					if (hard_drop || mino_ticks >= down_ticks_final)
					{
						if (mino_bottom_dist > 0)
							last_is_rotate_action = false;

						if (hard_drop || mino_bottom_dist == 0)
						{
							mino_pos.y() += mino_bottom_dist;
							if (hard_drop)
								score += mino_bottom_dist * 2;
							mino_bottom_dist = 0;
							draw_mino(c_main, TileMino1 + mino_type, mino_pos, 0, mino_coords, mino_col_decay);

							for (auto i = 0; i < 4; i++)
								full_lines[i] = -1;
							auto l = 0U;
							for (auto i = 0; i < board_height; i++)
							{
								if (line_full(c_main, i))
								{
									full_lines[l] = i;

									{
										auto cell_size = c_main->cell_size_;
										auto board_element = c_main->element;
										auto pos = board_element->global_pos + Vec2f(board_element->padding[0], board_element->padding[1]);
										pos.y() += i * cell_size.y();
										ui.parents.push(main_window->root);
										ui.next_element_pos = pos;
										ui.next_element_size = Vec2f(cell_size.x() * board_width, cell_size.y());
										ui.e_empty();
										auto element = ui.c_element();
										element->color = Vec4c(255);
										ui.parents.pop();

										struct Capturing
										{
											cElement* e;
											uint f;
										}capture;
										capture.e = element;
										capture.f = 5;
										get_looper()->add_event([](Capture& c) {
											auto& capture = c.data<Capturing>();
											capture.f--;
											if (capture.f > 0)
											{
												capture.e->pos.x() -= 10.f;
												capture.e->size.x() += 20.f;
												capture.e->pos.y() += 2.4f;
												capture.e->size.y() -= 4.8f;
												capture.e->color.a() = max(capture.e->color.a() - 30, 0);
												c._current = INVALID_POINTER;
											}
											else
											{
												auto e = capture.e->entity;
												e->parent->remove_child(e);
											}
										}, Capture().set_data(&capture), 0.f);
									}

									l++;
								}
							}
							lines += l;
							if (l > 0)
							{
								std::wstring special_str;

								auto attack = 0;

								get_combo_award(combo, attack, special_str);

								combo++;

								auto tspin = mino_type == Mino_T && last_is_rotate_action;
								if (tspin)
								{
									Vec2i judge_points[] = {
										Vec2i(-1, -1),
										Vec2i(+1, -1),
										Vec2i(-1, +1),
										Vec2i(+1, +1),
									};
									auto count = 0;
									for (auto i = 0; i < array_size(judge_points); i++)
									{
										auto p = mino_pos + judge_points[i];
										if (p.x() < 0 || p.x() >= board_width ||
											p.y() < 0 || p.y() >= board_height ||
											c_main->cell(p) != TileGrid)
											count++;
									}
									if (count < 3)
										tspin = false;
								}

								get_lines_award(l, tspin, mini, back_to_back, score, attack, special_str);

								auto cancel = max(attack, (int)l);
								if (!garbages.empty())
								{
									for (auto it = garbages.begin(); it != garbages.end();)
									{
										if (it->lines <= cancel)
										{
											attack -= it->lines;
											cancel -= it->lines;
											it = garbages.erase(it);
										}
										else
										{
											it->lines -= cancel;
											attack -= cancel;
											cancel = 0;
											break;
										}
									}
									need_update_garbages_tip = true;
								}

								if (!special_str.empty())
								{
									c_text_special->entity->set_visible(true);
									c_text_special->set_text(special_str.c_str());
									get_looper()->remove_events(FLAME_CHASH("special_text"));
									get_looper()->add_event([](Capture&) {
										app.c_text_special->entity->set_visible(false);
									}, Capture(), 1.f, FLAME_CHASH("special_text"));
								}

								if (game_mode == GameVS && attack > 0)
								{
									nlohmann::json req;
									req["action"] = "attack";
									req["value"] = attack;
									auto n = ::rand() % players.size() + 1;
									auto target = -1;
									while (n > 0)
									{
										target++;
										if (target == players.size())
											target = 0;
										if (target != my_room_index)
										{
											auto& p = players[target];
											if (p.id && !p.disconnected)
												n--;
										}
									}
									if (server)
									{
										req["index"] = my_room_index;
										auto str = req.dump();
										auto& p = players[target];
										if (p.id && !p.disconnected)
											server->send(p.id, str.data(), str.size(), false);
									}
									if (client)
									{
										req["target"] = target;
										auto str = req.dump();
										client->send(str.data(), str.size());
									}
								}

								for (auto i = 0; i < l; i++)
								{
									for (auto x = 0; x < board_width; x++)
										c_main->set_cell(Vec2u(x, full_lines[i]), TileGrid);
								}

								clear_ticks = CLEAR_TICKS;
								if (game_mode == GameSingleMarathon && lines % 5 == 0)
								{
									level++;
									level = min(DOWN_TICKS, level);
								}

								sound_clear_src->play();
							}
							else
							{
								for (auto it = garbages.begin(); it != garbages.end();)
								{
									if (it->time == 0)
									{
										auto n = it->lines;
										for (auto i = 0; i < board_height - n; i++)
										{
											for (auto x = 0; x < board_width; x++)
											{
												auto id = c_main->cell(Vec2i(x, i + n));
												c_main->set_cell(Vec2u(x, i), id, id == TileGrid ? Vec4c(255) : mino_col_decay);
											}
										}
										auto hole = ::rand() % board_width;
										for (auto i = 0; i < n; i++)
										{
											auto y = board_height - i - 1;
											for (auto x = 0; x < board_width; x++)
												c_main->set_cell(Vec2u(x, y), TileGray, mino_col_decay);
											c_main->set_cell(Vec2u(hole, y), TileGrid);
										}
										it = garbages.erase(it);
										need_update_garbages_tip = true;
									}
									else
										it++;
								}

								clear_ticks = 0;
								combo = 0;

								if (hard_drop)
									sound_hard_drop_src->play();
								else
									sound_soft_drop_src->play();
							}
							mino_pos.y() = -1;
							mino_just_hold = false;
						}
						else
						{
							mino_pos.y()++;
							mino_bottom_dist--;
							if (is_soft_drop)
							{
								score++;

								sound_move_src->play();
							}
						}
						mino_ticks = 0;
					}
					mino_ticks++;

					if (mino_pos.y() != -1)
					{
						if (mino_bottom_dist)
							draw_mino(c_main, TileGhost, mino_pos, mino_bottom_dist, mino_coords, g_mino_colors[mino_type]);
						draw_mino(c_main, TileMino1 + mino_type, mino_pos, 0, mino_coords);
					}
				}
			}

			if (game_mode == GameVS)
			{
				nlohmann::json req;
				req["action"] = "report_board";
				req["index"] = my_room_index;
				std::string d;
				d.resize(board_height * board_width);
				for (auto y = 0; y < board_height; y++)
				{
					for (auto x = 0; x < board_width; x++)
					{
						auto id = c_main->cell(Vec2i(x, y));
						d[y * board_width + x] = '0' + id;
					}
				}
				req["board"] = d;
				auto str = req.dump();
				if (server)
				{
					for (auto i = 1; i < players.size(); i++)
					{
						auto& p = players[i];
						if (p.id && !p.disconnected)
							server->send(p.id, str.data(), str.size(), false);
					}
				}
				if (client)
					client->send(str.data(), str.size());

				if (dead)
				{
					if (server)
						people_dead(my_room_index);
					if (client)
					{
						nlohmann::json req;
						req["action"] = "report_dead";
						auto str = req.dump();
						client->send(str.data(), str.size());
					}
				}
			}
		}

		update_status();
		if (game_mode == GameVS)
		{
			if (need_update_garbages_tip)
			{
				e_garbage->remove_children(0, -1);
				ui.parents.push(e_garbage);
				auto idx = 0;
				for (auto i = 0; i < garbages.size(); i++)
				{
					auto& g = garbages[i];
					for (auto j = 0; j < g.lines; j++)
					{
						ui.next_element_pos = Vec2f(0.f, -idx * 24.f - i * 4.f);
						ui.next_element_size = Vec2f(24.f);
						ui.e_image((atlas->canvas_slot_ << 16) + atlas->find_tile(FLAME_HASH("gray.png")));
						idx++;
					}
				}
				ui.parents.pop();
				need_update_garbages_tip = false;
			}
			auto idx = 0;
			for (auto i = 0; i < garbages.size(); i++)
			{
				auto& g = garbages[i];
				if (g.time > 0)
					g.time--;
				if (g.time == 0)
				{
					for (auto j = 0; j < g.lines; j++)
						e_garbage->children[idx + j]->get_component(cImage)->color = Vec4c(255, 0, 0, 255);
				}
				idx += g.lines;
			}
		}
	}
}

int main(int argc, char **args)
{
	app.create();

	new MainForm();

	get_looper()->loop([](Capture&) {
		app.run();
	}, Capture());

	return 0;
}
